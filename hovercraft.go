package main

import (
	"github.com/Jragonmiris/mathgl"
	gl "github.com/chsc/gogl/gl21"
	glfw "github.com/go-gl/glfw3"
)

type Hovercraft struct {
	x float64
	y float64
	z float64

	r float64
}

func (this *Hovercraft) Render() {
	gl.PushMatrix()
	defer gl.PopMatrix()

	gl.Translated(gl.Double(this.x), gl.Double(this.y), gl.Double(this.z))
	gl.Rotated(gl.Double(this.r), 0, 0, -1)

	gl.Begin(gl.TRIANGLE_FAN)
	{
		gl.Color3f(0, 1, 0)
		const size = 3
		gl.Vertex2i(2*size, 0)
		gl.Vertex2i(-size, +size)
		gl.Vertex2i(-size, -size)
	}
	gl.End()
}

func (this *Hovercraft) Input(window *glfw.Window) {
	if (window.GetKey(glfw.KeyLeft) != glfw.Release) && !(window.GetKey(glfw.KeyRight) != glfw.Release) {
		this.r -= 3
	} else if (window.GetKey(glfw.KeyRight) != glfw.Release) && !(window.GetKey(glfw.KeyLeft) != glfw.Release) {
		this.r += 3
	}

	var direction mathgl.Vec2d
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

	// Physics update.
	if direction.Len() != 0 {
		rotM := mathgl.Rotate2Dd(-this.r)
		direction = rotM.Mul2x1(direction)

		direction = direction.Normalize().Mul(1)

		this.x += direction[0]
		this.y += direction[1]
	}
}
