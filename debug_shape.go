package main

import (
	"encoding/binary"
	"errors"
	"fmt"
	"math"

	"golang.org/x/mobile/exp/f32"

	"github.com/go-gl/mathgl/mgl64"
	"github.com/goxjs/gl"
	"github.com/goxjs/gl/glutil"
)

const (
	RACER_LIFTTHRUST_CONE = 3.0 // Height of cone (base radius 1).
)

var liftThrusterPositions = [...]mgl64.Vec3{
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
var liftThrusterDirections,
	liftThrusterRollEffect,
	liftThrusterPitchEffect,
	liftThrusterVelEffect = func() ([]mgl64.Vec3, []float64, []float64, []float64) {

	var (
		liftThrusterDirections  = make([]mgl64.Vec3, len(liftThrusterPositions))
		liftThrusterRollEffect  = make([]float64, len(liftThrusterPositions))
		liftThrusterPitchEffect = make([]float64, len(liftThrusterPositions))
		liftThrusterVelEffect   = make([]float64, len(liftThrusterPositions))
	)

	var liftThrusterOrigin = mgl64.Vec3{0, 0, RACER_LIFTTHRUST_CONE}
	for i := range liftThrusterPositions {
		liftThrusterDirections[i] = liftThrusterPositions[i].Sub(liftThrusterOrigin).Normalize()
		liftThrusterRollEffect[i] = RACER_LIFTTHRUST_MAXPITCHROLLACCEL * -liftThrusterPositions[i].Y() // TODO: Verify.
		liftThrusterPitchEffect[i] = RACER_LIFTTHRUST_MAXPITCHROLLACCEL * liftThrusterPositions[i].X() // TODO: Verify.
		liftThrusterVelEffect[i] = 1.0
	}

	return liftThrusterDirections, liftThrusterRollEffect, liftThrusterPitchEffect, liftThrusterVelEffect
}()

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

func calcThrusterDistances() []float64 {
	var dists []float64
	for i := range liftThrusterPositions {
		mat := mgl64.Ident4()
		mat = mat.Mul4(mgl64.HomogRotate3D(float64(player.R), mgl64.Vec3{0, 0, -1}))
		mat = mat.Mul4(mgl64.HomogRotate3D(float64(player.Roll), mgl64.Vec3{1, 0, 0}))
		mat = mat.Mul4(mgl64.HomogRotate3D(float64(player.Pitch), mgl64.Vec3{0, 1, 0}))

		pos := mat.Mul4x1(liftThrusterPositions[i].Vec4(1)).Vec3()
		pos = pos.Add(mgl64.Vec3{float64(player.X), float64(player.Y), float64(player.Z)})

		dir := mat.Mul4x1(liftThrusterDirections[i].Vec4(1)).Vec3()

		dists = append(dists, track.distToTerrain(pos, dir, RACER_LIFTTHRUST_MAXDIST))
	}
	return dists
}

var iterations uint64

func debugShapeRender() {
	gl.BindBuffer(gl.ARRAY_BUFFER, vertexVbo3)
	vertexPositionAttribute := gl.GetAttribLocation(program3, "aVertexPosition")
	gl.EnableVertexAttribArray(vertexPositionAttribute)
	gl.VertexAttribPointer(vertexPositionAttribute, 3, gl.FLOAT, false, 0, 0)

	//t := time.Now()
	iterations = 0
	thrusterDistances := calcThrusterDistances()
	//fmt.Println("calcThrusterDistances:", iterations, time.Since(t).Seconds()*1000, "ms")

	// Lift thrusters visualized as lines.
	var vertices []float32
	for i, p0 := range liftThrusterPositions {
		vertices = append(vertices, float32(p0.X()), float32(p0.Y()), float32(p0.Z()))
		p1 := p0.Add(liftThrusterDirections[i].Mul(thrusterDistances[i]))
		vertices = append(vertices, float32(p1.X()), float32(p1.Y()), float32(p1.Z()))
	}
	for i, p0 := range liftThrusterPositions {
		p1 := p0.Add(liftThrusterDirections[i].Mul(thrusterDistances[i]))
		vertices = append(vertices, float32(p1.X()), float32(p1.Y()), float32(p1.Z()))
		p2 := p0.Add(liftThrusterDirections[i].Mul(50))
		vertices = append(vertices, float32(p2.X()), float32(p2.Y()), float32(p2.Z()))
	}
	for i, p0 := range liftThrusterPositions {
		p1 := p0.Add(liftThrusterDirections[i].Mul(thrusterDistances[i]))
		vertices = append(vertices, float32(p1.X()), float32(p1.Y()), float32(p1.Z()))
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
