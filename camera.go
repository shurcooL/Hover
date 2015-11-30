package main

import (
	"github.com/go-gl/mathgl/mgl32"
	"github.com/go-gl/mathgl/mgl64"
	"github.com/goxjs/glfw"
)

type CameraI interface {
	Apply() mgl32.Mat4
	Input(*glfw.Window)
}

var cameraIndex int = 1
var cameras = []CameraI{&camera, &camera2}

// ---

var camera = Camera{X: 245.332877917019, Y: 619.2630811586736, Z: 567, RH: 25.2000000000001, RV: -12.300000000000002}

type Camera struct {
	X float64
	Y float64
	Z float64

	RH float64 // Degrees (for now, should change to radians too).
	RV float64
}

func (c *Camera) Apply() mgl32.Mat4 {
	mat := mgl32.Ident4()
	mat = mat.Mul4(mgl32.HomogRotate3D(mgl32.DegToRad(float32(c.RV+90)), mgl32.Vec3{-1, 0, 0})) // The 90 degree offset is necessary to make Z axis the up-vector in OpenGL (normally it's the in/out-of-screen vector).
	mat = mat.Mul4(mgl32.HomogRotate3D(mgl32.DegToRad(float32(c.RH)), mgl32.Vec3{0, 0, 1}))
	mat = mat.Mul4(mgl32.Translate3D(float32(-c.X), float32(-c.Y), float32(-c.Z)))
	return mat
}

func (c *Camera) Input(window *glfw.Window) {
	if (window.GetKey(glfw.KeyZ) != glfw.Release) && !(window.GetKey(glfw.KeyC) != glfw.Release) {
		c.Z -= 1
	} else if (window.GetKey(glfw.KeyC) != glfw.Release) && !(window.GetKey(glfw.KeyZ) != glfw.Release) {
		c.Z += 1
	}
}

// ---

var camera2 = Camera2{player: &player}

type Camera2 struct {
	player *Hovercraft
}

func (c *Camera2) Apply() mgl32.Mat4 {
	// offset is the camera offset relative to the player center.
	var offset = mgl64.Vec3{0, -25, 15}
	{
		mat := mgl64.Ident4()
		mat = mat.Mul4(mgl64.HomogRotate3D(player.R, mgl64.Vec3{0, 0, -1}))

		offset = mat.Mul4x1(offset.Vec4(1)).Vec3()
		offset = offset.Normalize()

		dist := track.distToTerrain(mgl64.Vec3{c.player.X, c.player.Y, c.player.Z}, offset, 30)
		dist *= 0.9 // HACK: Underestimate so that corners don't get clipped often. TODO: Calculate distance to 4 camera corners, use min.
		offset = offset.Mul(dist)
	}

	mat := mgl32.Ident4()
	mat = mat.Mul4(mgl32.HomogRotate3D(mgl32.DegToRad(float32(-20+90)), mgl32.Vec3{-1, 0, 0})) // The 90 degree offset is necessary to make Z axis the up-vector in OpenGL (normally it's the in/out-of-screen vector).
	mat = mat.Mul4(mgl32.HomogRotate3D(float32(c.player.R), mgl32.Vec3{0, 0, 1}))
	mat = mat.Mul4(mgl32.Translate3D(float32(-offset.X()), float32(-offset.Y()), float32(-offset.Z())))
	mat = mat.Mul4(mgl32.Translate3D(float32(-c.player.X), float32(-c.player.Y), float32(-c.player.Z)))
	return mat
}

func (c *Camera2) Input(*glfw.Window) {}
