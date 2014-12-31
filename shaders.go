package main

import (
	"fmt"

	"github.com/go-gl/glow/gl/2.1/gl"
	"github.com/go-gl/mathgl/mgl32"
)

const (
	vertexSource = `#version 120

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
` + "\x00"
	fragmentSource = `#version 120

//precision lowp float;

uniform sampler2D texUnit;
uniform sampler2D texUnit2;

varying vec3 vPixelColor;
varying vec2 vTexCoord;
varying float vTerrType;

void main() {
	vec3 tex = mix(texture2D(texUnit, vTexCoord).rgb, texture2D(texUnit2, vTexCoord).rgb, vTerrType);
	gl_FragColor = vec4(vPixelColor * tex, 1.0);
}
` + "\x00"
)

var program uint32
var pMatrixUniform int32
var mvMatrixUniform int32
var texUnit int32
var texUnit2 int32

var mvMatrix mgl32.Mat4
var pMatrix mgl32.Mat4

func initShaders() error {
	vertexShader := gl.CreateShader(gl.VERTEX_SHADER)
	vertexSourceStr := gl.Str(vertexSource)
	gl.ShaderSource(vertexShader, 1, &vertexSourceStr, nil)
	gl.CompileShader(vertexShader)
	defer gl.DeleteShader(vertexShader)

	fragmentShader := gl.CreateShader(gl.FRAGMENT_SHADER)
	fragmentSourceStr := gl.Str(fragmentSource)
	gl.ShaderSource(fragmentShader, 1, &fragmentSourceStr, nil)
	gl.CompileShader(fragmentShader)
	defer gl.DeleteShader(fragmentShader)

	program = gl.CreateProgram()
	gl.AttachShader(program, vertexShader)
	gl.AttachShader(program, fragmentShader)
	gl.LinkProgram(program)

	/*if !gl.GetProgramParameterb(program, gl.LINK_STATUS) {
		return errors.New("LINK_STATUS")
	}*/

	gl.ValidateProgram(program)
	/*if !gl.GetProgramParameterb(program, gl.VALIDATE_STATUS) {
		return errors.New("VALIDATE_STATUS")
	}*/

	gl.UseProgram(program)

	pMatrixUniform = gl.GetUniformLocation(program, gl.Str("uPMatrix\x00"))
	mvMatrixUniform = gl.GetUniformLocation(program, gl.Str("uMVMatrix\x00"))
	texUnit = gl.GetUniformLocation(program, gl.Str("texUnit\x00"))
	texUnit2 = gl.GetUniformLocation(program, gl.Str("texUnit2\x00"))

	if glError := gl.GetError(); glError != 0 {
		return fmt.Errorf("gl.GetError: %v", glError)
	}

	return nil
}
