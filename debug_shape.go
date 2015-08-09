package main

import (
	"encoding/binary"
	"errors"
	"fmt"

	"golang.org/x/mobile/exp/f32"

	"github.com/go-gl/mathgl/mgl32"
	"github.com/goxjs/gl"
	"github.com/goxjs/gl/glutil"
)

const (
	RACER_LIFTTHRUST_CONE = 3.0 // Height of cone (base radius 1).
)

var liftThrusterOrigin = mgl32.Vec3{0, 0, RACER_LIFTTHRUST_CONE}
var liftThrusterPositions = [...]mgl32.Vec3{
	/*{1, 0, 0},
	{math.Sqrt2 / 2, math.Sqrt2 / 2, 0},
	{0, 1, 0},
	{-math.Sqrt2 / 2, math.Sqrt2 / 2, 0},
	{-1, 0, 0},
	{-math.Sqrt2 / 2, -math.Sqrt2 / 2, 0},
	{0, -1, 0},
	{math.Sqrt2 / 2, -math.Sqrt2 / 2, 0},*/
	{0, 0, 0},
}

var program3 gl.Program
var pMatrixUniform3 gl.Uniform
var mvMatrixUniform3 gl.Uniform
var colorUniform3 gl.Uniform
var vertexVbo3 gl.Buffer

func loadDebugShape() error {
	const (
		vertexSource = `//#version 120 // OpenGL 2.1.
//#version 100 // WebGL.

uniform mat4 uMVMatrix;
uniform mat4 uPMatrix;

attribute vec3 aVertexPosition;

void main() {
	gl_PointSize = 5.0;
	gl_Position = uPMatrix * uMVMatrix * vec4(aVertexPosition, 1.0);
}
`
		fragmentSource = `//#version 120 // OpenGL 2.1.
//#version 100 // WebGL.

#ifdef GL_ES
	precision lowp float;
#endif

uniform vec3 uColor;

void main() {
	gl_FragColor = vec4(uColor, 1.0);
}
`
	)

	var err error
	program3, err = glutil.CreateProgram(vertexSource, fragmentSource)
	if err != nil {
		return err
	}

	gl.ValidateProgram(program3)
	if gl.GetProgrami(program3, gl.VALIDATE_STATUS) != gl.TRUE {
		return errors.New("VALIDATE_STATUS: " + gl.GetProgramInfoLog(program3))
	}

	gl.UseProgram(program3)

	pMatrixUniform3 = gl.GetUniformLocation(program3, "uPMatrix")
	mvMatrixUniform3 = gl.GetUniformLocation(program3, "uMVMatrix")
	colorUniform3 = gl.GetUniformLocation(program3, "uColor")

	vertexVbo3 = gl.CreateBuffer()

	if glError := gl.GetError(); glError != 0 {
		return fmt.Errorf("gl.GetError: %v", glError)
	}

	return nil
}

func calcThrusterDistances() []float32 {
	var dists []float32
	for _, liftThrusterPosition := range liftThrusterPositions {
		mat := mgl32.Ident4()
		mat = mat.Mul4(mgl32.HomogRotate3D(float32(player.R), mgl32.Vec3{0, 0, -1}))
		mat = mat.Mul4(mgl32.HomogRotate3D(float32(player.Roll), mgl32.Vec3{1, 0, 0}))
		mat = mat.Mul4(mgl32.HomogRotate3D(float32(player.Pitch), mgl32.Vec3{0, 1, 0}))

		pos := mat.Mul4x1(liftThrusterPosition.Vec4(1)).Vec3()
		pos = pos.Add(mgl32.Vec3{float32(player.X), float32(player.Y), float32(player.Z)})

		dir := mat.Mul4x1(liftThrusterPosition.Sub(liftThrusterOrigin).Normalize().Mul(30).Vec4(1)).Vec3()

		dists = append(dists, track.distToTerrain(pos, dir))
	}
	return dists
}

func debugShapeRender() {
	gl.BindBuffer(gl.ARRAY_BUFFER, vertexVbo3)
	vertexPositionAttribute := gl.GetAttribLocation(program3, "aVertexPosition")
	gl.EnableVertexAttribArray(vertexPositionAttribute)
	gl.VertexAttribPointer(vertexPositionAttribute, 3, gl.FLOAT, false, 0, 0)

	thrusterDistances := calcThrusterDistances()

	// Lift thrusters visualized as lines.
	var vertices []float32
	for i, p0 := range liftThrusterPositions {
		vertices = append(vertices, p0.X(), p0.Y(), p0.Z())
		p1 := p0.Add(p0.Sub(liftThrusterOrigin).Normalize().Mul(thrusterDistances[i]))
		vertices = append(vertices, p1.X(), p1.Y(), p1.Z())
	}
	for i, p0 := range liftThrusterPositions {
		p1 := p0.Add(p0.Sub(liftThrusterOrigin).Normalize().Mul(thrusterDistances[i]))
		vertices = append(vertices, p1.X(), p1.Y(), p1.Z())
		p2 := p0.Add(p0.Sub(liftThrusterOrigin).Mul(50))
		vertices = append(vertices, p2.X(), p2.Y(), p2.Z())
	}
	for i, p0 := range liftThrusterPositions {
		p1 := p0.Add(p0.Sub(liftThrusterOrigin).Normalize().Mul(thrusterDistances[i]))
		vertices = append(vertices, p1.X(), p1.Y(), p1.Z())
	}

	gl.BufferData(gl.ARRAY_BUFFER, f32.Bytes(binary.LittleEndian, vertices...), gl.STATIC_DRAW)

	// Lift thrusters visualized as lines.
	gl.Uniform3f(colorUniform3, 0, 1, 0)
	gl.DrawArrays(gl.LINES, 0, 2*len(liftThrusterPositions))
	gl.Uniform3f(colorUniform3, 1, 0, 0)
	gl.DrawArrays(gl.LINES, 2*len(liftThrusterPositions), 2*len(liftThrusterPositions))
	gl.Uniform3f(colorUniform3, 0, 0, 1)
	gl.DrawArrays(gl.POINTS, 2*2*len(liftThrusterPositions), len(liftThrusterPositions))
}
