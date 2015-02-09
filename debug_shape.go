package main

import (
	"errors"
	"fmt"
	"math"

	"github.com/go-gl/mathgl/mgl32"
	"github.com/goxjs/gl"
	"github.com/goxjs/gl/glutil"
)

const (
	RACER_LIFTTHRUST_CONE = 3.0 // Height of cone (base radius 1).
)

var liftThrusterPositions = []mgl32.Vec3{
	{1, 0, 0},
	{math.Sqrt2 / 2, math.Sqrt2 / 2, 0},
	{0, 1, 0},
	{-math.Sqrt2 / 2, math.Sqrt2 / 2, 0},
	{-1, 0, 0},
	{-math.Sqrt2 / 2, -math.Sqrt2 / 2, 0},
	{0, -1, 0},
	{math.Sqrt2 / 2, -math.Sqrt2 / 2, 0},
	{0, 0, 0},
}

const (
	vertexSource3 = `//#version 120 // OpenGL 2.1.
//#version 100 // WebGL.

uniform mat4 uMVMatrix;
uniform mat4 uPMatrix;

attribute vec3 aVertexPosition;

void main() {
	gl_Position = uPMatrix * uMVMatrix * vec4(aVertexPosition, 1.0);
}
`
	fragmentSource3 = `//#version 120 // OpenGL 2.1.
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

var program3 gl.Program
var pMatrixUniform3 gl.Uniform
var mvMatrixUniform3 gl.Uniform
var colorUniform3 gl.Uniform
var vertexVbo3 gl.Buffer

func loadDebugShape() error {
	var err error
	program3, err = glutil.CreateProgram(vertexSource3, fragmentSource3)
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

	const size = 1
	var vertices = []float32{
		0, 0, -debugHeight,
		0, +size, 3*size - debugHeight,
		0, -size, 3*size - debugHeight,
		0, 0, -debugHeight,
		0, +size, -3*size - debugHeight,
		0, -size, -3*size - debugHeight,
	}

	// Lift thrusters visualized as lines.
	thrusterOrigin := mgl32.Vec3{0, 0, RACER_LIFTTHRUST_CONE}
	for _, p0 := range liftThrusterPositions {
		vertices = append(vertices, p0.X(), p0.Y(), p0.Z())
		p1 := p0.Add(p0.Sub(thrusterOrigin).Mul(5))
		vertices = append(vertices, p1.X(), p1.Y(), p1.Z())
	}

	vertexVbo3 = createVbo3Float(vertices)

	if glError := gl.GetError(); glError != 0 {
		return fmt.Errorf("gl.GetError: %v", glError)
	}

	return nil
}
