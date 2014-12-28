package main

import (
	"encoding/binary"
	"fmt"
	"os"
	"unsafe"

	"github.com/go-gl/glow/gl/2.1/gl"
)

var track *Track

const TRIGROUP_NUM_BITS_USED = 510
const TRIGROUP_NUM_DWORDS = (TRIGROUP_NUM_BITS_USED + 2) / 32
const TRIGROUP_WIDTHSHIFT = 4
const TERR_HEIGHT_SCALE = 1.0 / 32
const TERR_TEXTURE_SCALE = 1.0 / 20

type TerrTypeNode struct {
	Type       uint8
	NextStartX uint16
	Next       uint16
	_          uint8
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

	vertexVbo       uint32
	colorVbo        uint32
	textureCoordVbo uint32
}

func newTrack(path string) *Track {
	file, err := os.Open(path)
	if err != nil {
		panic(err)
	}
	defer file.Close()

	var track Track

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

	fi, err := file.Stat()
	if err != nil {
		panic(err)
	}
	fileOffset, err := file.Seek(0, os.SEEK_CUR)
	if err != nil {
		panic(err)
	}
	fmt.Printf("Read %v of %v bytes.\n", fileOffset, fi.Size())

	{
		rowCount := uint64(track.Depth) - 1
		rowLength := uint64(track.Width)

		vertexData := make([][3]float32, 2*rowLength*rowCount)
		colorData := make([][3]byte, 2*rowLength*rowCount)
		textureCoordData := make([][2]float32, 2*rowLength*rowCount)

		var index uint64
		for y := uint16(1); y < track.Depth; y++ {
			for x := uint16(0); x < track.Width; x++ {
				for i := uint16(0); i < 2; i++ {
					yy := y - i

					terrCoord := track.TerrCoords[uint64(yy)*uint64(track.Width)+uint64(x)]
					height := float64(terrCoord.Height) * TERR_HEIGHT_SCALE
					lightIntensity := byte(terrCoord.LightIntensity)

					vertexData[index] = [3]float32{float32(x), float32(yy), float32(height)}
					colorData[index] = [3]byte{lightIntensity, lightIntensity, lightIntensity}
					textureCoordData[index] = [2]float32{float32(float32(x) * TERR_TEXTURE_SCALE), float32(float32(yy) * TERR_TEXTURE_SCALE)}
					index++
				}
			}
		}

		track.vertexVbo = createVbo3Float(vertexData)
		track.colorVbo = createVbo3Ubyte(colorData)
		track.textureCoordVbo = createVbo2Float(textureCoordData)
	}

	return &track
}

func (track *Track) getHeightAt(x, y float64) float64 {
	// TODO: Interpolate between 4 points.
	return track.getHeightAtPoint(uint64(x), uint64(y))
}

func (track *Track) getHeightAtPoint(x, y uint64) float64 {
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

func (track *Track) Render() {
	gl.PushMatrix()
	defer gl.PopMatrix()

	gl.Color3f(1, 1, 1)

	if wireframe {
		gl.PolygonMode(gl.FRONT_AND_BACK, gl.LINE)
	}

	gl.Enable(gl.TEXTURE_2D)
	gl.Begin(gl.TRIANGLE_FAN)
	{
		gl.TexCoord2i(0, 0)
		gl.Vertex2i(0, 0)
		gl.TexCoord2i(1, 0)
		gl.Vertex2i(int32(track.Width), 0)
		gl.TexCoord2i(1, 1)
		gl.Vertex2i(int32(track.Width), int32(track.Depth))
		gl.TexCoord2i(0, 1)
		gl.Vertex2i(0, int32(track.Depth))
	}
	gl.End()

	{
		rowCount := uint64(track.Depth) - 1
		rowLength := uint64(track.Width)

		gl.EnableClientState(gl.VERTEX_ARRAY)
		gl.BindBuffer(gl.ARRAY_BUFFER, track.vertexVbo)
		gl.VertexPointer(3, gl.FLOAT, 0, nil)

		gl.EnableClientState(gl.COLOR_ARRAY)
		gl.BindBuffer(gl.ARRAY_BUFFER, track.colorVbo)
		gl.ColorPointer(3, gl.UNSIGNED_BYTE, 0, nil)

		gl.EnableClientState(gl.TEXTURE_COORD_ARRAY)
		gl.BindBuffer(gl.ARRAY_BUFFER, track.textureCoordVbo)
		gl.TexCoordPointer(2, gl.FLOAT, 0, nil)

		for row := uint64(0); row < rowCount; row++ {
			gl.DrawArrays(gl.TRIANGLE_STRIP, int32(row*2*rowLength), int32(2*rowLength))
		}

		gl.DisableClientState(gl.VERTEX_ARRAY)
		gl.DisableClientState(gl.COLOR_ARRAY)
		gl.DisableClientState(gl.TEXTURE_COORD_ARRAY)
	}
	gl.Disable(gl.TEXTURE_2D)

	if wireframe {
		gl.PolygonMode(gl.FRONT_AND_BACK, gl.FILL)
	}
}

// ---

func createVbo3Float(vertices [][3]float32) uint32 {
	var vbo uint32
	gl.GenBuffers(1, &vbo)
	gl.BindBuffer(gl.ARRAY_BUFFER, vbo)
	defer gl.BindBuffer(gl.ARRAY_BUFFER, 0)

	gl.BufferData(gl.ARRAY_BUFFER, int(unsafe.Sizeof(vertices[0]))*len(vertices), gl.Ptr(vertices), gl.STATIC_DRAW)

	return vbo
}

func createVbo2Float(vertices [][2]float32) uint32 {
	var vbo uint32
	gl.GenBuffers(1, &vbo)
	gl.BindBuffer(gl.ARRAY_BUFFER, vbo)
	defer gl.BindBuffer(gl.ARRAY_BUFFER, 0)

	gl.BufferData(gl.ARRAY_BUFFER, int(unsafe.Sizeof(vertices[0]))*len(vertices), gl.Ptr(vertices), gl.STATIC_DRAW)

	return vbo
}

func createVbo3Ubyte(vertices [][3]byte) uint32 {
	var vbo uint32
	gl.GenBuffers(1, &vbo)
	gl.BindBuffer(gl.ARRAY_BUFFER, vbo)
	defer gl.BindBuffer(gl.ARRAY_BUFFER, 0)

	gl.BufferData(gl.ARRAY_BUFFER, int(unsafe.Sizeof(vertices[0]))*len(vertices), gl.Ptr(vertices), gl.STATIC_DRAW)

	return vbo
}

func cStringToGoString(cString []byte) string {
	n := 0
	for i, b := range cString {
		if b == 0 {
			break
		}
		n = i + 1
	}
	return string(cString[:n])
}
