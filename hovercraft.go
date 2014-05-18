package main

import gl "github.com/chsc/gogl/gl21"

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
