package main

import (
	"errors"
	"fmt"

	"github.com/goxjs/gl"
	"github.com/goxjs/gl/glutil"
)

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
		0, 0, 0,
		0, +size, 3 * size,
		0, -size, 3 * size,
		0, 0, 0,
		0, +size, -3 * size,
		0, -size, -3 * size,
	}

	vertexVbo3 = createVbo3Float(vertices)

	if glError := gl.GetError(); glError != 0 {
		return fmt.Errorf("gl.GetError: %v", glError)
	}

	return nil
}
