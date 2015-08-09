package main

import (
	"errors"
	"fmt"
	"math"
	"strings"

	"github.com/GlenKelley/go-collada"
	"github.com/bradfitz/iter"
	"github.com/go-gl/mathgl/mgl32"
	"github.com/go-gl/mathgl/mgl64"
	"github.com/goxjs/gl"
	"github.com/goxjs/gl/glutil"
	"github.com/goxjs/glfw"
)

// Tau is the constant τ, which equals to 6.283185... or 2π.
// Reference: https://oeis.org/A019692
const Tau = 2 * math.Pi

var player = Hovercraft{X: 250.8339829707148, Y: 630.3799668664172, Z: 565, R: 0}

var debugHeight = 3.0

type Hovercraft struct {
	X float64
	Y float64
	Z float64

	// Radians.
	Pitch float64
	Roll  float64
	R     float64
}

func (this *Hovercraft) Render() {
	gl.UseProgram(program3)
	{
		mat := mvMatrix
		mat = mat.Mul4(mgl32.Translate3D(float32(player.X), float32(player.Y), float32(player.Z)))
		mat = mat.Mul4(mgl32.HomogRotate3D(float32(player.R), mgl32.Vec3{0, 0, -1}))
		mat = mat.Mul4(mgl32.HomogRotate3D(float32(player.Roll), mgl32.Vec3{1, 0, 0}))
		mat = mat.Mul4(mgl32.HomogRotate3D(float32(player.Pitch), mgl32.Vec3{0, 1, 0}))

		gl.UniformMatrix4fv(pMatrixUniform3, pMatrix[:])
		gl.UniformMatrix4fv(mvMatrixUniform3, mat[:])

		debugShapeRender()
	}

	gl.UseProgram(program2)
	{
		mat := mvMatrix
		mat = mat.Mul4(mgl32.Translate3D(float32(player.X), float32(player.Y), float32(player.Z)))
		mat = mat.Mul4(mgl32.HomogRotate3D(float32(player.R), mgl32.Vec3{0, 0, -1}))
		mat = mat.Mul4(mgl32.HomogRotate3D(float32(player.Roll), mgl32.Vec3{1, 0, 0}))
		mat = mat.Mul4(mgl32.HomogRotate3D(float32(player.Pitch), mgl32.Vec3{0, 1, 0}))

		mat = mat.Mul4(mgl32.HomogRotate3D(Tau/4, mgl32.Vec3{0, 0, -1}))
		mat = mat.Mul4(mgl32.Scale3D(0.12, 0.12, 0.12))

		gl.UniformMatrix4fv(pMatrixUniform2, pMatrix[:])
		gl.UniformMatrix4fv(mvMatrixUniform2, mat[:])
		gl.Uniform3f(uCameraPosition, float32(-(camera.Y - player.Y)), float32(camera.X-player.X), float32(camera.Z-(player.Z-debugHeight))) // HACK: Calculate this properly.

		gl.BindBuffer(gl.ARRAY_BUFFER, vertexVbo)
		vertexPositionAttribute := gl.GetAttribLocation(program2, "aVertexPosition")
		gl.EnableVertexAttribArray(vertexPositionAttribute)
		gl.VertexAttribPointer(vertexPositionAttribute, 3, gl.FLOAT, false, 0, 0)

		gl.BindBuffer(gl.ARRAY_BUFFER, normalVbo)
		normalAttribute := gl.GetAttribLocation(program2, "aNormal")
		gl.EnableVertexAttribArray(normalAttribute)
		gl.VertexAttribPointer(normalAttribute, 3, gl.FLOAT, false, 0, 0)

		gl.DrawArrays(gl.TRIANGLES, 0, 3*m_TriangleCount)
	}
	gl.UseProgram(gl.Program{})
}

const deltaTime = 1.0 / 60

const (
	RACER_MAXTURNRATE  = math.Pi
	RACER_MAXPITCHRATE = 0.75 * math.Pi
	RACER_MAXROLLRATE  = 0.75 * math.Pi
)

func (this *Hovercraft) Input(window *glfw.Window) {
	var rot mgl64.Vec3
	if (window.GetKey(glfw.KeyLeft) != glfw.Release) && !(window.GetKey(glfw.KeyRight) != glfw.Release) {
		rot[0] = -RACER_MAXTURNRATE * deltaTime
	} else if (window.GetKey(glfw.KeyRight) != glfw.Release) && !(window.GetKey(glfw.KeyLeft) != glfw.Release) {
		rot[0] = RACER_MAXTURNRATE * deltaTime
	}
	if (window.GetKey(glfw.KeyDown) != glfw.Release) && !(window.GetKey(glfw.KeyUp) != glfw.Release) {
		rot[1] = -RACER_MAXPITCHRATE * deltaTime
	} else if (window.GetKey(glfw.KeyUp) != glfw.Release) && !(window.GetKey(glfw.KeyDown) != glfw.Release) {
		rot[1] = RACER_MAXPITCHRATE * deltaTime
	}
	if (window.GetKey(glfw.KeyA) != glfw.Release) && !(window.GetKey(glfw.KeyD) != glfw.Release) {
		rot[2] = -RACER_MAXROLLRATE * deltaTime
	} else if (window.GetKey(glfw.KeyD) != glfw.Release) && !(window.GetKey(glfw.KeyA) != glfw.Release) {
		rot[2] = RACER_MAXROLLRATE * deltaTime
	}
	if window.GetKey(glfw.KeyLeftShift) != glfw.Release || window.GetKey(glfw.KeyRightShift) != glfw.Release {
		rot = rot.Mul(0.05)
	}
	this.R += rot[0]
	this.Pitch += rot[1]
	this.Roll += rot[2]

	var direction mgl64.Vec2
	if (window.GetKey(glfw.KeyW) != glfw.Release) && !(window.GetKey(glfw.KeyS) != glfw.Release) {
		direction[0] = +1
	} else if (window.GetKey(glfw.KeyS) != glfw.Release) && !(window.GetKey(glfw.KeyW) != glfw.Release) {
		direction[0] = -1
	}
	if (window.GetKey(glfw.KeyQ) != glfw.Release) && !(window.GetKey(glfw.KeyE) != glfw.Release) {
		debugHeight -= 0.1
	} else if (window.GetKey(glfw.KeyE) != glfw.Release) && !(window.GetKey(glfw.KeyQ) != glfw.Release) {
		debugHeight += 0.1
	}

	// Physics update.
	if direction.Len() != 0 {
		rotM := mgl64.Rotate2D(-this.R)
		direction = rotM.Mul2x1(direction)

		direction = direction.Normalize()

		if window.GetKey(glfw.KeyLeftShift) != glfw.Release || window.GetKey(glfw.KeyRightShift) != glfw.Release {
			direction = direction.Mul(0.01)
		} else if window.GetKey(glfw.KeySpace) != glfw.Release {
			direction = direction.Mul(5)
		}

		this.X += direction[0]
		this.Y += direction[1]
	}
}

// Update physics.
func (this *Hovercraft) Physics() {
	// TEST: Check terrain height calculations.
	{
		this.Z = track.getHeightAt(this.X, this.Y) + debugHeight
	}
}

var program2 gl.Program
var pMatrixUniform2 gl.Uniform
var mvMatrixUniform2 gl.Uniform
var uCameraPosition gl.Uniform

func initShaders2() error {
	const (
		vertexSource = `//#version 120 // OpenGL 2.1.
//#version 100 // WebGL.

attribute vec3 aVertexPosition;
attribute vec3 aNormal;

uniform mat4 uMVMatrix;
uniform mat4 uPMatrix;

varying vec3 vPosition;
varying vec3 vNormal;

void main() {
	vNormal = normalize(aNormal);
	vPosition = aVertexPosition.xyz;
	gl_Position = uPMatrix * uMVMatrix * vec4(aVertexPosition, 1.0);
}
`
		fragmentSource = `//#version 120 // OpenGL 2.1.
//#version 100 // WebGL.

#ifdef GL_ES
	precision lowp float;
#endif

uniform vec3 uCameraPosition;

varying vec3 vPosition;
varying vec3 vNormal;

void main() {
	vec3 vNormal = normalize(vNormal);

	// Diffuse lighting.
	vec3 toLight = normalize(vec3(1.0, 1.0, 3.0));
	float diffuse = dot(vNormal, toLight);
	diffuse = clamp(diffuse, 0.0, 1.0);

	// Specular highlights.
	vec3 posToCamera = normalize(uCameraPosition - vPosition);
	vec3 halfV = normalize(toLight + posToCamera);
	float intSpec = max(dot(halfV, vNormal), 0.0);
	vec3 spec = 0.5 * vec3(1.0, 1.0, 1.0) * pow(intSpec, 32.0);

	vec3 PixelColor = (0.2 + 0.8 * diffuse) * vec3(0.75, 0.75, 0.75) + spec;

	gl_FragColor = vec4(PixelColor, 1.0);
}
`
	)

	var err error
	program2, err = glutil.CreateProgram(vertexSource, fragmentSource)
	if err != nil {
		return err
	}

	gl.ValidateProgram(program2)
	if gl.GetProgrami(program2, gl.VALIDATE_STATUS) != gl.TRUE {
		return errors.New("VALIDATE_STATUS: " + gl.GetProgramInfoLog(program2))
	}

	gl.UseProgram(program2)

	pMatrixUniform2 = gl.GetUniformLocation(program2, "uPMatrix")
	mvMatrixUniform2 = gl.GetUniformLocation(program2, "uMVMatrix")
	uCameraPosition = gl.GetUniformLocation(program2, "uCameraPosition")

	if glError := gl.GetError(); glError != 0 {
		return fmt.Errorf("gl.GetError: %v", glError)
	}

	return nil
}

var doc *collada.Collada
var m_TriangleCount, m_LineCount int
var vertexVbo gl.Buffer
var normalVbo gl.Buffer

func loadModel() error {
	err := initShaders2()
	if err != nil {
		return err
	}

	file, err := glfw.Open("./vehicle0.dae")
	if err != nil {
		return err
	}
	defer file.Close()

	doc, err = collada.LoadDocumentFromReader(file)
	if err != nil {
		return err
	}

	// Calculate the total triangle and line counts.
	for _, geometry := range doc.LibraryGeometries[0].Geometry {
		for _, triangle := range geometry.Mesh.Triangles {
			m_TriangleCount += triangle.HasCount.Count
		}
	}

	fmt.Printf("m_TriangleCount = %v, m_LineCount = %v\n", m_TriangleCount, m_LineCount)

	// ---

	//goon.DumpExpr(doc.LibraryGeometries[0].Geometry)

	var scale float32 = 1.0
	if strings.Contains(doc.HasAsset.Asset.Contributor[0].AuthoringTool, "Google SketchUp 8") {
		scale = 0.4
	}

	vertices := make([]float32, 3*3*m_TriangleCount)
	normals := make([]float32, 3*3*m_TriangleCount)

	nTriangleNumber := 0
	for _, geometry := range doc.LibraryGeometries[0].Geometry {
		if len(geometry.Mesh.Triangles) == 0 {
			continue
		}

		// HACK. 0 seems to be position, 1 is normal, but need to not hardcode this.
		pVertexData := geometry.Mesh.Source[0].FloatArray.F32()
		pNormalData := geometry.Mesh.Source[1].FloatArray.F32()

		//goon.DumpExpr(len(pVertexData))
		//goon.DumpExpr(len(pNormalData))

		unsharedCount := len(geometry.Mesh.Vertices.Input)

		for _, triangles := range geometry.Mesh.Triangles {
			sharedIndicies := triangles.HasP.P.I()
			sharedCount := len(triangles.HasSharedInput.Input)

			//goon.DumpExpr(len(sharedIndicies))
			//goon.DumpExpr(sharedCount)

			for nTriangle := range iter.N(triangles.HasCount.Count) {
				offset := 0 // HACK. 0 seems to be position, 1 is normal, but need to not hardcode this.
				vertices[3*3*nTriangleNumber+0] = pVertexData[3*sharedIndicies[(3*nTriangle+0)*sharedCount+offset]+0] * scale
				vertices[3*3*nTriangleNumber+1] = pVertexData[3*sharedIndicies[(3*nTriangle+0)*sharedCount+offset]+1] * scale
				vertices[3*3*nTriangleNumber+2] = pVertexData[3*sharedIndicies[(3*nTriangle+0)*sharedCount+offset]+2] * scale
				vertices[3*3*nTriangleNumber+3] = pVertexData[3*sharedIndicies[(3*nTriangle+1)*sharedCount+offset]+0] * scale
				vertices[3*3*nTriangleNumber+4] = pVertexData[3*sharedIndicies[(3*nTriangle+1)*sharedCount+offset]+1] * scale
				vertices[3*3*nTriangleNumber+5] = pVertexData[3*sharedIndicies[(3*nTriangle+1)*sharedCount+offset]+2] * scale
				vertices[3*3*nTriangleNumber+6] = pVertexData[3*sharedIndicies[(3*nTriangle+2)*sharedCount+offset]+0] * scale
				vertices[3*3*nTriangleNumber+7] = pVertexData[3*sharedIndicies[(3*nTriangle+2)*sharedCount+offset]+1] * scale
				vertices[3*3*nTriangleNumber+8] = pVertexData[3*sharedIndicies[(3*nTriangle+2)*sharedCount+offset]+2] * scale

				if unsharedCount*sharedCount == 2 {
					offset = sharedCount - 1 // HACK. 0 seems to be position, 1 is normal, but need to not hardcode this.
					normals[3*3*nTriangleNumber+0] = pNormalData[3*sharedIndicies[(3*nTriangle+0)*sharedCount+offset]+0]
					normals[3*3*nTriangleNumber+1] = pNormalData[3*sharedIndicies[(3*nTriangle+0)*sharedCount+offset]+1]
					normals[3*3*nTriangleNumber+2] = pNormalData[3*sharedIndicies[(3*nTriangle+0)*sharedCount+offset]+2]
					normals[3*3*nTriangleNumber+3] = pNormalData[3*sharedIndicies[(3*nTriangle+1)*sharedCount+offset]+0]
					normals[3*3*nTriangleNumber+4] = pNormalData[3*sharedIndicies[(3*nTriangle+1)*sharedCount+offset]+1]
					normals[3*3*nTriangleNumber+5] = pNormalData[3*sharedIndicies[(3*nTriangle+1)*sharedCount+offset]+2]
					normals[3*3*nTriangleNumber+6] = pNormalData[3*sharedIndicies[(3*nTriangle+2)*sharedCount+offset]+0]
					normals[3*3*nTriangleNumber+7] = pNormalData[3*sharedIndicies[(3*nTriangle+2)*sharedCount+offset]+1]
					normals[3*3*nTriangleNumber+8] = pNormalData[3*sharedIndicies[(3*nTriangle+2)*sharedCount+offset]+2]
				}

				nTriangleNumber++
			}
		}
	}

	// ---

	vertexVbo = createVbo3Float(vertices)
	normalVbo = createVbo3Float(normals)

	if glError := gl.GetError(); glError != 0 {
		return fmt.Errorf("gl.GetError: %v", glError)
	}

	return nil
}
