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
const Tau = 2 * math.Pi

var player = Hovercraft{x: 250.8339829707148, y: 630.3799668664172, z: 565, r: 0}

type Hovercraft struct {
	x float64
	y float64
	z float64

	r float64 // Radians.
}

func (this *Hovercraft) Render() {
	gl.UseProgram(program3)
	{
		mat := mvMatrix
		mat = mat.Mul4(mgl32.Translate3D(float32(player.x), float32(player.y), float32(player.z)))
		mat = mat.Mul4(mgl32.HomogRotate3D(float32(player.r), mgl32.Vec3{0, 0, -1}))

		gl.UniformMatrix4fv(pMatrixUniform3, pMatrix[:])
		gl.UniformMatrix4fv(mvMatrixUniform3, mat[:])

		gl.BindBuffer(gl.ARRAY_BUFFER, vertexVbo3)
		vertexPositionAttribute := gl.GetAttribLocation(program3, "aVertexPosition")
		gl.EnableVertexAttribArray(vertexPositionAttribute)
		gl.VertexAttribPointer(vertexPositionAttribute, 3, gl.FLOAT, false, 0, 0)

		gl.Uniform3f(colorUniform3, 0, 1, 0)
		gl.DrawArrays(gl.TRIANGLES, 0, 3*1)
		gl.Uniform3f(colorUniform3, 1, 0, 0)
		gl.DrawArrays(gl.TRIANGLES, 3*1, 3*1)
	}

	gl.UseProgram(program2)
	{
		mat := mvMatrix
		mat = mat.Mul4(mgl32.Translate3D(float32(player.x), float32(player.y), float32(player.z+3)))
		mat = mat.Mul4(mgl32.HomogRotate3D(float32(player.r), mgl32.Vec3{0, 0, -1}))
		mat = mat.Mul4(mgl32.HomogRotate3D(Tau/4, mgl32.Vec3{0, 0, -1}))
		mat = mat.Mul4(mgl32.Scale3D(0.15, 0.15, 0.15))

		gl.UniformMatrix4fv(pMatrixUniform2, pMatrix[:])
		gl.UniformMatrix4fv(mvMatrixUniform2, mat[:])
		gl.Uniform3f(uCameraPosition, float32(-(camera.y - player.y)), float32(camera.x-player.x), float32(camera.z-(player.z+3))) // HACK: Calculate this properly.

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

func (this *Hovercraft) Input(window *glfw.Window) {
	if (window.GetKey(glfw.KeyLeft) != glfw.Release) && !(window.GetKey(glfw.KeyRight) != glfw.Release) {
		this.r -= mgl64.DegToRad(3)
	} else if (window.GetKey(glfw.KeyRight) != glfw.Release) && !(window.GetKey(glfw.KeyLeft) != glfw.Release) {
		this.r += mgl64.DegToRad(3)
	}

	var direction mgl64.Vec2
	if (window.GetKey(glfw.KeyA) != glfw.Release) && !(window.GetKey(glfw.KeyD) != glfw.Release) {
		direction[1] = +1
	} else if (window.GetKey(glfw.KeyD) != glfw.Release) && !(window.GetKey(glfw.KeyA) != glfw.Release) {
		direction[1] = -1
	}
	if (window.GetKey(glfw.KeyW) != glfw.Release) && !(window.GetKey(glfw.KeyS) != glfw.Release) {
		direction[0] = +1
	} else if (window.GetKey(glfw.KeyS) != glfw.Release) && !(window.GetKey(glfw.KeyW) != glfw.Release) {
		direction[0] = -1
	}
	if (window.GetKey(glfw.KeyQ) != glfw.Release) && !(window.GetKey(glfw.KeyE) != glfw.Release) {
		camera.Z -= 1
	} else if (window.GetKey(glfw.KeyE) != glfw.Release) && !(window.GetKey(glfw.KeyQ) != glfw.Release) {
		camera.Z += 1
	}

	// Physics update.
	if direction.Len() != 0 {
		rotM := mgl64.Rotate2D(-this.r)
		direction = rotM.Mul2x1(direction)

		direction = direction.Normalize().Mul(1)

		if window.GetKey(glfw.KeyLeftShift) != glfw.Release || window.GetKey(glfw.KeyRightShift) != glfw.Release {
			direction = direction.Mul(0.01)
		} else if window.GetKey(glfw.KeySpace) != glfw.Release {
			direction = direction.Mul(5)
		}

		this.x += direction[0]
		this.y += direction[1]
	}
}

// Update physics.
func (this *Hovercraft) Physics() {
	// TEST: Check terrain height calculations.
	{
		this.z = track.getHeightAt(this.x, this.y)
	}
}

const (
	vertexSource2 = `//#version 120 // OpenGL 2.1.
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
	fragmentSource2 = `//#version 120 // OpenGL 2.1.
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

var program2 gl.Program
var pMatrixUniform2 gl.Uniform
var mvMatrixUniform2 gl.Uniform
var uCameraPosition gl.Uniform

func initShaders2() error {
	var err error
	program2, err = glutil.CreateProgram(vertexSource2, fragmentSource2)
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
