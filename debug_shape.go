package main

import (
	"errors"
	"fmt"

	"github.com/shurcooL/gogl"
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

var program3 *gogl.Program
var pMatrixUniform3 *gogl.UniformLocation
var mvMatrixUniform3 *gogl.UniformLocation
var colorUniform3 *gogl.UniformLocation
var vertexVbo3 *gogl.Buffer

func loadDebugShape() error {
	vertexShader := gl.CreateShader(gl.VERTEX_SHADER)
	gl.ShaderSource(vertexShader, vertexSource3)
	gl.CompileShader(vertexShader)
	defer gl.DeleteShader(vertexShader)

	fragmentShader := gl.CreateShader(gl.FRAGMENT_SHADER)
	gl.ShaderSource(fragmentShader, fragmentSource3)
	gl.CompileShader(fragmentShader)
	defer gl.DeleteShader(fragmentShader)

	program3 = gl.CreateProgram()
	gl.AttachShader(program3, vertexShader)
	gl.AttachShader(program3, fragmentShader)

	gl.LinkProgram(program3)
	if !gl.GetProgramParameterb(program3, gl.LINK_STATUS) {
		return errors.New("LINK_STATUS: " + gl.GetProgramInfoLog(program3))
	}

	gl.ValidateProgram(program3)
	if !gl.GetProgramParameterb(program3, gl.VALIDATE_STATUS) {
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
