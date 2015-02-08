package main

import (
	"errors"
	"fmt"
	"strings"

	"github.com/GlenKelley/go-collada"
	"github.com/bradfitz/iter"
	"github.com/go-gl/mathgl/mgl64"
	"github.com/shurcooL/gogl"
	glfw "github.com/shurcooL/goglfw"
)

var player = Hovercraft{x: 250.8339829707148, y: 630.3799668664172, z: 565, r: 0}

type Hovercraft struct {
	x float64
	y float64
	z float64

	r float64
}

func (this *Hovercraft) Render() {
	/*gl.PushMatrix()
	defer gl.PopMatrix()

	gl.Translated(float64(this.x), float64(this.y), float64(this.z))
	gl.Rotated(float64(this.r), 0, 0, -1)

	gl.Begin(gl.TRIANGLES)
	{
		const size = 1
		gl.Color3f(0, 1, 0)
		gl.Vertex3i(0, 0, 0)
		gl.Vertex3i(0, +size, 3*size)
		gl.Vertex3i(0, -size, 3*size)
		gl.Color3f(1, 0, 0)
		gl.Vertex3i(0, 0, 0)
		gl.Vertex3i(0, +size, -3*size)
		gl.Vertex3i(0, -size, -3*size)
	}
	gl.End()*/

	gl.UseProgram(program2)
	{
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
	gl.UseProgram(nil)
}

func (this *Hovercraft) Input(window *glfw.Window) {
	if (window.GetKey(glfw.KeyLeft) != glfw.Release) && !(window.GetKey(glfw.KeyRight) != glfw.Release) {
		this.r -= 3
	} else if (window.GetKey(glfw.KeyRight) != glfw.Release) && !(window.GetKey(glfw.KeyLeft) != glfw.Release) {
		this.r += 3
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
		this.z -= 1
	} else if (window.GetKey(glfw.KeyE) != glfw.Release) && !(window.GetKey(glfw.KeyQ) != glfw.Release) {
		this.z += 1
	}

	// Physics update.
	if direction.Len() != 0 {
		rotM := mgl64.Rotate2D(mgl64.DegToRad(-this.r))
		direction = rotM.Mul2x1(direction)

		direction = direction.Normalize().Mul(1)

		if window.GetKey(glfw.KeyLeftShift) != glfw.Release || window.GetKey(glfw.KeyRightShift) != glfw.Release {
			direction = direction.Mul(0.001)
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

var program2 *gogl.Program
var pMatrixUniform2 *gogl.UniformLocation
var mvMatrixUniform2 *gogl.UniformLocation
var uCameraPosition *gogl.UniformLocation

func initShaders2() error {
	vertexShader := gl.CreateShader(gl.VERTEX_SHADER)
	gl.ShaderSource(vertexShader, vertexSource2)
	gl.CompileShader(vertexShader)
	defer gl.DeleteShader(vertexShader)

	fragmentShader := gl.CreateShader(gl.FRAGMENT_SHADER)
	gl.ShaderSource(fragmentShader, fragmentSource2)
	gl.CompileShader(fragmentShader)
	defer gl.DeleteShader(fragmentShader)

	program2 = gl.CreateProgram()
	gl.AttachShader(program2, vertexShader)
	gl.AttachShader(program2, fragmentShader)
	gl.LinkProgram(program2)

	if !gl.GetProgramParameterb(program2, gl.LINK_STATUS) {
		return errors.New("LINK_STATUS: " + gl.GetProgramInfoLog(program2))
	}

	gl.ValidateProgram(program2)
	if !gl.GetProgramParameterb(program2, gl.VALIDATE_STATUS) {
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
var vertexVbo *gogl.Buffer
var normalVbo *gogl.Buffer

func loadModel() error {
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
