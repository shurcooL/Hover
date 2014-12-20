package main

import "github.com/go-gl/glow/gl/2.1/gl"

type CameraI interface {
	Apply()
}

var cameraIndex int
var cameras = []CameraI{&camera, &camera2}

// ---

var camera = Camera{x: 160.12941888695732, y: 685.2641404161014, z: 600, rh: 115.50000000000003, rv: -14.999999999999998}

type Camera struct {
	x float64
	y float64
	z float64

	rh float64
	rv float64
}

func (this *Camera) Apply() {
	gl.Rotated(float64(this.rv+90), -1, 0, 0) // The 90 degree offset is necessary to make Z axis the up-vector in OpenGL (normally it's the in/out-of-screen vector)
	gl.Rotated(float64(this.rh), 0, 0, 1)
	gl.Translated(float64(-this.x), float64(-this.y), float64(-this.z))
}

// ---

var camera2 = Camera2{player: &player}

type Camera2 struct {
	player *Hovercraft
}

func (this *Camera2) Apply() {
	gl.Rotated(float64(-20+90), -1, 0, 0) // The 90 degree offset is necessary to make Z axis the up-vector in OpenGL (normally it's the in/out-of-screen vector)
	gl.Translated(0, 25, -20)
	gl.Rotated(float64(this.player.r+90), 0, 0, 1)
	gl.Translated(float64(-this.player.x), float64(-this.player.y), float64(-this.player.z))
}
