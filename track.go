package main

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
	"math"
	"os"
	"time"

	"github.com/go-gl/mathgl/mgl32"
	"github.com/go-gl/mathgl/mgl64"
	"github.com/goxjs/gl"
	"github.com/goxjs/gl/glutil"
	"github.com/goxjs/glfw"
	"golang.org/x/mobile/exp/f32"
)

var track *Track

const TRIGROUP_NUM_BITS_USED = 510
const TRIGROUP_NUM_DWORDS = (TRIGROUP_NUM_BITS_USED + 2) / 32
const TRIGROUP_WIDTHSHIFT = 4
const TERR_HEIGHT_SCALE = 1.0 / 32

type TerrTypeNode struct {
	Type       uint8
	_          uint8
	NextStartX uint16
	Next       uint16
}

type NavCoord struct {
	X, Z             uint16
	DistToStartCoord uint16 // Decider at forks, and determines racers' rank/place.
	Next             uint16
	Alt              uint16
}

type NavCoordLookupNode struct {
	NavCoord   uint16
	NextStartX uint16
	Next       uint16
}

type TerrCoord struct {
	Height         uint16
	LightIntensity uint8
}

type TriGroup struct {
	Data [TRIGROUP_NUM_DWORDS]uint32
}

type TrackFileHeader struct {
	SunlightDirection, SunlightPitch float32
	RacerStartPositions              [8][3]float32
	NumTerrTypes                     uint16
	NumTerrTypeNodes                 uint16
	NumNavCoords                     uint16
	NumNavCoordLookupNodes           uint16
	Width, Depth                     uint16
}

type Track struct {
	TrackFileHeader
	NumTerrCoords  uint32
	TriGroupsWidth uint32
	TriGroupsDepth uint32
	NumTriGroups   uint32

	TerrTypeTextureFilenames []string

	TerrTypeRuns  []TerrTypeNode
	TerrTypeNodes []TerrTypeNode

	NavCoords           []NavCoord
	NavCoordLookupRuns  []NavCoordLookupNode
	NavCoordLookupNodes []NavCoordLookupNode

	TerrCoords []TerrCoord
	TriGroups  []TriGroup

	vertexVbo   gl.Buffer
	colorVbo    gl.Buffer
	terrTypeVbo gl.Buffer

	textures [2]gl.Texture
}

func newTrack(path string) (*Track, error) {
	var track Track

	err := initShaders()
	if err != nil {
		panic(err)
	}
	track.textures[0], err = loadTexture("./dirt.png")
	if err != nil {
		panic(err)
	}
	track.textures[1], err = loadTexture("./sand.png")
	if err != nil {
		panic(err)
	}

	started := time.Now()

	file, err := glfw.Open(path)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	binary.Read(file, binary.LittleEndian, &track.TrackFileHeader)

	// Stuff derived from header info.
	track.NumTerrCoords = uint32(track.Width) * uint32(track.Depth)
	track.TriGroupsWidth = (uint32(track.Width) - 1) >> TRIGROUP_WIDTHSHIFT
	track.TriGroupsDepth = (uint32(track.Depth) - 1) >> TRIGROUP_WIDTHSHIFT
	track.NumTriGroups = track.TriGroupsWidth * track.TriGroupsDepth

	track.TerrTypeTextureFilenames = make([]string, track.NumTerrTypes)
	for i := uint16(0); i < track.NumTerrTypes; i++ {
		var terrTypeTextureFilename [32]byte
		binary.Read(file, binary.LittleEndian, &terrTypeTextureFilename)
		track.TerrTypeTextureFilenames[i] = cStringToGoString(terrTypeTextureFilename[:])
	}

	track.TerrTypeRuns = make([]TerrTypeNode, track.Depth)
	binary.Read(file, binary.LittleEndian, &track.TerrTypeRuns)

	track.TerrTypeNodes = make([]TerrTypeNode, track.NumTerrTypeNodes)
	binary.Read(file, binary.LittleEndian, &track.TerrTypeNodes)

	track.NavCoords = make([]NavCoord, track.NumNavCoords)
	binary.Read(file, binary.LittleEndian, &track.NavCoords)

	track.NavCoordLookupRuns = make([]NavCoordLookupNode, track.Depth)
	binary.Read(file, binary.LittleEndian, &track.NavCoordLookupRuns)

	track.NavCoordLookupNodes = make([]NavCoordLookupNode, track.NumNavCoordLookupNodes)
	binary.Read(file, binary.LittleEndian, &track.NavCoordLookupNodes)

	track.TerrCoords = make([]TerrCoord, track.NumTerrCoords)
	binary.Read(file, binary.LittleEndian, &track.TerrCoords)

	track.TriGroups = make([]TriGroup, track.NumTriGroups)
	binary.Read(file, binary.LittleEndian, &track.TriGroups)

	fileOffset, err := file.Seek(0, os.SEEK_CUR)
	if err != nil {
		return nil, err
	}
	fileSize, err := file.Seek(0, os.SEEK_END)
	if err != nil {
		return nil, err
	}
	fmt.Printf("Read %v of %v bytes.\n", fileOffset, fileSize)

	{
		rowCount := int(track.Depth) - 1
		rowLength := int(track.Width)

		terrTypeMap := make([]uint8, int(track.Width)*int(track.Depth))
		for y := 0; y < int(track.Depth); y++ {
			pCurrNode := &track.TerrTypeRuns[y]

			for x := 0; x < int(track.Width); x++ {
				if x >= int(pCurrNode.NextStartX) {
					pCurrNode = &track.TerrTypeNodes[pCurrNode.Next]
				}
				terrTypeMap[y*int(track.Width)+x] = pCurrNode.Type
			}
		}

		vertexData := make([]float32, 3*2*rowLength*rowCount)
		colorData := make([]uint8, 3*2*rowLength*rowCount)
		terrTypeData := make([]float32, 2*rowLength*rowCount)

		var index int
		for y := 1; y < int(track.Depth); y++ {
			for x := 0; x < int(track.Width); x++ {
				for i := 0; i < 2; i++ {
					yy := y - i

					terrCoord := &track.TerrCoords[yy*int(track.Width)+x]
					height := float64(terrCoord.Height) * TERR_HEIGHT_SCALE
					lightIntensity := terrCoord.LightIntensity

					vertexData[3*index+0], vertexData[3*index+1], vertexData[3*index+2] = float32(x), float32(yy), float32(height)
					colorData[3*index+0], colorData[3*index+1], colorData[3*index+2] = lightIntensity, lightIntensity, lightIntensity
					if terrTypeMap[yy*int(track.Width)+x] == 0 {
						terrTypeData[index] = 0
					} else {
						terrTypeData[index] = 1
					}
					index++
				}
			}
		}

		track.vertexVbo = createVbo3Float(vertexData)
		track.colorVbo = createVbo3Ubyte(colorData)
		track.terrTypeVbo = createVbo3Float(terrTypeData)
	}

	fmt.Println("Done loading track in:", time.Since(started))

	return &track, nil
}

func intersectRayTriangle(u, v, O, rayO, rayD mgl64.Vec3) (float64, bool) {
	N := u.Cross(v).Normalize()

	if mgl64.FloatEqual(N.Dot(rayD), 0) {
		// Parallel.
		// TODO.
		return 0, false
	}

	St := N.Dot(O.Sub(rayO))
	Sb := N.Dot(rayD)
	S := St / Sb
	if S < 0 {
		// Ray goes away from triangle, no hit.
		return 0, false
	}

	I := rayO.Add(rayD.Mul(S))

	uu := u.Dot(u)
	uv := u.Dot(v)
	vv := v.Dot(v)
	w := I.Sub(O)
	wu := w.Dot(u)
	wv := w.Dot(v)
	D := uv*uv - uu*vv

	// Get and test parametric coordinates.
	s := (uv*wv - vv*wu) / D
	if s < 0 || s > 1 {
		// I is outside T.
		return 0, false
	}
	t := (uv*wu - uu*wv) / D
	if t < 0 || (s+t) > 1 {
		// I is outside T.
		return 0, false
	}
	// I is in T.
	return S, true
}

// vDir must be normalized.
// maxDist should be positive.
func (track *Track) distToTerrain(vPosition mgl64.Vec3, vDir mgl64.Vec3, maxDist float64) float64 {
	iterations++

	trackZ := track.getHeightAt(vPosition.X(), vPosition.Y())
	if trackZ >= vPosition.Z() {
		// We're underground.
		return vPosition.Z() - trackZ
	}

	D2 := vDir.Vec2()
	if mgl64.FloatEqual(D2.Len(), 0) {
		if vDir.Z() >= 0 {
			// Direction is straight up.
			return maxDist
		} else {
			// Direction is straight down, return vertical diff.
			dist := vPosition.Z() - trackZ
			if dist > maxDist {
				dist = maxDist
			}
			return dist
		}
	}

	x, y := uint64(vPosition.X()), uint64(vPosition.Y())
	distance := float64(0)
	var xf, yf float64
	var a, b, c, d, u, v mgl64.Vec3
	var nearestCellDist float64
	var nearestCell neighborCell
	for {
		iterations++
		xf, yf = float64(x), float64(y)
		a = mgl64.Vec3{xf, yf, track.getHeightAtPoint(x, y)}
		b = mgl64.Vec3{xf + 1, yf, track.getHeightAtPoint(x+1, y)}
		c = mgl64.Vec3{xf, yf + 1, track.getHeightAtPoint(x, y+1)}
		d = mgl64.Vec3{xf + 1, yf + 1, track.getHeightAtPoint(x+1, y+1)}

		u = a.Sub(b)
		v = d.Sub(b)
		if dist, ok := intersectRayTriangle(u, v, b, vPosition, vDir); ok {
			dist += distance
			if dist > maxDist {
				dist = maxDist
			}
			return dist
		}

		u = a.Sub(c)
		v = d.Sub(c)
		if dist, ok := intersectRayTriangle(u, v, c, vPosition, vDir); ok {
			dist += distance
			if dist > maxDist {
				dist = maxDist
			}
			return dist
		}

		// Find nearest neighbor cell to visit next.
		if D2.X() >= 0 {
			// +x cell.
			nearestCellDist = (xf + 1 - vPosition.X()) / D2.X()
			nearestCell = positiveX
		} else {
			// -x cell.
			nearestCellDist = (xf - vPosition.X()) / D2.X()
			nearestCell = negativeX
		}
		if D2.Y() >= 0 {
			// +y cell.
			if dist := (yf + 1 - vPosition.Y()) / D2.Y(); dist < nearestCellDist {
				nearestCellDist = dist
				nearestCell = positiveY
			}
		} else {
			// -y cell.
			if dist := (yf - vPosition.Y()) / D2.Y(); dist < nearestCellDist {
				nearestCellDist = dist
				nearestCell = negativeY
			}
		}
		vPosition = vPosition.Add(vDir.Mul(nearestCellDist))
		switch nearestCell {
		case positiveX:
			x++
		case negativeX:
			x--
		case positiveY:
			y++
		case negativeY:
			y--
		}
		distance += nearestCellDist
		if distance > maxDist {
			return maxDist
		}
	}
	panic("unreachable")
}

func (track *Track) getHeightAt(x, y float64) float64 {
	s, t := x-math.Floor(x), y-math.Floor(y)

	//   c--d
	//   | /|
	// ^ |/ |
	// t a--b
	//   s >

	a := track.getHeightAtPoint(uint64(x), uint64(y))
	d := track.getHeightAtPoint(uint64(x)+1, uint64(y)+1)

	if s >= t { // Lower triangle abd.
		b := track.getHeightAtPoint(uint64(x)+1, uint64(y))
		return a + s*(b-a) + t*(d-b)
	} else { // Upper triangle acd.
		c := track.getHeightAtPoint(uint64(x), uint64(y)+1)
		return a + s*(d-c) + t*(c-a)
	}
}

func (track *Track) getHeightAtPoint(x, y uint64) float64 {
	// TODO: Factor out the bounds checking to a higher level (where it's less expensive).
	if x > uint64(track.Width)-1 {
		x = uint64(track.Width) - 1
	}
	if y > uint64(track.Depth)-1 {
		y = uint64(track.Depth) - 1
	}

	terrCoord := track.TerrCoords[y*uint64(track.Width)+x]
	height := float64(terrCoord.Height) * TERR_HEIGHT_SCALE
	return height
}

func (track *Track) normalOfCell(x, y uint64) mgl64.Vec3 {
	// c--d
	// |\/|
	// |/\|
	// a--b
	a := track.getHeightAtPoint(x, y)
	b := track.getHeightAtPoint(x+1, y)
	c := track.getHeightAtPoint(x, y+1)
	d := track.getHeightAtPoint(x+1, y+1)

	slope1 := d - a
	slope2 := c - b
	vDiag1 := mgl64.Vec3{1, 1, slope1}
	vDiag2 := mgl64.Vec3{-1, 1, slope2}

	return vDiag1.Cross(vDiag2).Normalize()
}

func (track *Track) Render() {
	gl.UseProgram(program)
	{
		gl.UniformMatrix4fv(pMatrixUniform, pMatrix[:])
		gl.UniformMatrix4fv(mvMatrixUniform, mvMatrix[:])

		if wireframe {
			gl.Uniform1i(wireframeUniform, gl.TRUE)
		} else {
			gl.Uniform1i(wireframeUniform, gl.FALSE)
		}

		gl.Uniform1i(texUnit, 0)
		gl.ActiveTexture(gl.TEXTURE0)
		gl.BindTexture(gl.TEXTURE_2D, track.textures[0])
		gl.Uniform1i(texUnit2, 1)
		gl.ActiveTexture(gl.TEXTURE1)
		gl.BindTexture(gl.TEXTURE_2D, track.textures[1])

		gl.BindBuffer(gl.ARRAY_BUFFER, track.vertexVbo)
		vertexPositionAttribute := gl.GetAttribLocation(program, "aVertexPosition")
		gl.EnableVertexAttribArray(vertexPositionAttribute)
		gl.VertexAttribPointer(vertexPositionAttribute, 3, gl.FLOAT, false, 0, 0)

		gl.BindBuffer(gl.ARRAY_BUFFER, track.colorVbo)
		vertexColorAttribute := gl.GetAttribLocation(program, "aVertexColor")
		gl.EnableVertexAttribArray(vertexColorAttribute)
		gl.VertexAttribPointer(vertexColorAttribute, 3, gl.UNSIGNED_BYTE, true, 0, 0)

		gl.BindBuffer(gl.ARRAY_BUFFER, track.terrTypeVbo)
		vertexTerrTypeAttribute := gl.GetAttribLocation(program, "aVertexTerrType")
		gl.EnableVertexAttribArray(vertexTerrTypeAttribute)
		gl.VertexAttribPointer(vertexTerrTypeAttribute, 1, gl.FLOAT, false, 0, 0)

		rowCount := int(track.Depth) - 1
		rowLength := int(track.Width)

		for row := 0; row < rowCount; row++ {
			gl.DrawArrays(gl.TRIANGLE_STRIP, row*2*rowLength, 2*rowLength)
		}
	}
	gl.UseProgram(gl.Program{})
}

// ---

var program gl.Program
var pMatrixUniform gl.Uniform
var mvMatrixUniform gl.Uniform
var wireframeUniform gl.Uniform
var texUnit gl.Uniform
var texUnit2 gl.Uniform

var mvMatrix mgl32.Mat4
var pMatrix mgl32.Mat4

func initShaders() error {
	const (
		vertexSource = `//#version 120 // OpenGL 2.1.
//#version 100 // WebGL.

const float TERR_TEXTURE_SCALE = 1.0 / 20.0; // From track.h rather than terrain.h.

attribute vec3 aVertexPosition;
attribute vec3 aVertexColor;
attribute float aVertexTerrType;

uniform mat4 uMVMatrix;
uniform mat4 uPMatrix;

varying vec3 vPixelColor;
varying vec2 vTexCoord;
varying float vTerrType;

void main() {
	vPixelColor = aVertexColor;
	vTexCoord = aVertexPosition.xy * TERR_TEXTURE_SCALE;
	vTerrType = aVertexTerrType;
	gl_Position = uPMatrix * uMVMatrix * vec4(aVertexPosition, 1.0);
}
`
		fragmentSource = `//#version 120 // OpenGL 2.1.
//#version 100 // WebGL.

#ifdef GL_ES
	precision lowp float;
#endif

const float TERR_TEXTURE_SCALE = 1.0 / 20.0; // From track.h rather than terrain.h.

uniform bool wireframe;

uniform sampler2D texUnit;
uniform sampler2D texUnit2;

varying vec3 vPixelColor;
varying vec2 vTexCoord;
varying float vTerrType;

void main() {
	if (wireframe) {
		vec2 unit;
		unit.x = mod(vTexCoord.x, TERR_TEXTURE_SCALE) / TERR_TEXTURE_SCALE;
		unit.y = mod(vTexCoord.y, TERR_TEXTURE_SCALE) / TERR_TEXTURE_SCALE;
		if (unit.x <= 0.02 || unit.x >= 0.98 || unit.y <= 0.02 || unit.y >= 0.98 ||
			(-0.02 <= unit.x - unit.y && unit.x - unit.y <= 0.02)) {

			gl_FragColor = vec4(unit.x, unit.y, 0.0, 1.0);
			return;
		};
	}

	vec3 tex = mix(texture2D(texUnit, vTexCoord).rgb, texture2D(texUnit2, vTexCoord).rgb, vTerrType);
	gl_FragColor = vec4(vPixelColor * tex, 1.0);
}
`
	)

	var err error
	program, err = glutil.CreateProgram(vertexSource, fragmentSource)
	if err != nil {
		return err
	}

	gl.ValidateProgram(program)
	if gl.GetProgrami(program, gl.VALIDATE_STATUS) != gl.TRUE {
		return errors.New("VALIDATE_STATUS: " + gl.GetProgramInfoLog(program))
	}

	gl.UseProgram(program)

	pMatrixUniform = gl.GetUniformLocation(program, "uPMatrix")
	mvMatrixUniform = gl.GetUniformLocation(program, "uMVMatrix")
	wireframeUniform = gl.GetUniformLocation(program, "wireframe")
	texUnit = gl.GetUniformLocation(program, "texUnit")
	texUnit2 = gl.GetUniformLocation(program, "texUnit2")

	if glError := gl.GetError(); glError != 0 {
		return fmt.Errorf("gl.GetError: %v", glError)
	}

	return nil
}

// ---

type neighborCell uint8

const (
	positiveX neighborCell = iota
	negativeX
	positiveY
	negativeY
)

// ---

func createVbo3Float(vertices []float32) gl.Buffer {
	vbo := gl.CreateBuffer()
	gl.BindBuffer(gl.ARRAY_BUFFER, vbo)
	gl.BufferData(gl.ARRAY_BUFFER, f32.Bytes(binary.LittleEndian, vertices...), gl.STATIC_DRAW)
	return vbo
}

func createVbo3Ubyte(vertices []uint8) gl.Buffer {
	vbo := gl.CreateBuffer()
	gl.BindBuffer(gl.ARRAY_BUFFER, vbo)
	gl.BufferData(gl.ARRAY_BUFFER, vertices, gl.STATIC_DRAW)
	return vbo
}

func cStringToGoString(cString []byte) string {
	i := bytes.IndexByte(cString, 0)
	if i < 0 {
		return ""
	}
	return string(cString[:i])
}
