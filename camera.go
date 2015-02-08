package main

import "github.com/go-gl/mathgl/mgl32"

type CameraI interface {
	Apply() mgl32.Mat4
}

var cameraIndex int
var cameras = []CameraI{&camera, &camera2}

// ---

var camera = Camera{x: 160.12941888695732, y: 685.2641404161014, z: 600, rh: 115.50000000000003, rv: -14.999999999999998}

//var camera = Camera{x: 651.067403141426, y: 604.5361059479138, z: 527.1199999999999, rh: 175.50000000000017, rv: -33.600000000000044}

type Camera struct {
	x float64
	y float64
	z float64

	rh float64 // Degrees (for now, should change to radians too).
	rv float64
}

func (this *Camera) Apply() mgl32.Mat4 {
	mat := mgl32.Ident4()
	mat = mat.Mul4(mgl32.HomogRotate3D(mgl32.DegToRad(float32(this.rv+90)), mgl32.Vec3{-1, 0, 0})) // The 90 degree offset is necessary to make Z axis the up-vector in OpenGL (normally it's the in/out-of-screen vector).
	mat = mat.Mul4(mgl32.HomogRotate3D(mgl32.DegToRad(float32(this.rh)), mgl32.Vec3{0, 0, 1}))
	mat = mat.Mul4(mgl32.Translate3D(float32(-this.x), float32(-this.y), float32(-this.z)))
	return mat
}

// ---

var camera2 = Camera2{player: &player}

type Camera2 struct {
	player *Hovercraft
}

func (this *Camera2) Apply() mgl32.Mat4 {
	mat := mgl32.Ident4()
	mat = mat.Mul4(mgl32.HomogRotate3D(mgl32.DegToRad(float32(-20+90)), mgl32.Vec3{-1, 0, 0})) // The 90 degree offset is necessary to make Z axis the up-vector in OpenGL (normally it's the in/out-of-screen vector).
	mat = mat.Mul4(mgl32.Translate3D(float32(0), float32(25), float32(-20)))
	mat = mat.Mul4(mgl32.HomogRotate3D(float32(this.player.r+Tau/4), mgl32.Vec3{0, 0, 1}))
	mat = mat.Mul4(mgl32.Translate3D(float32(-this.player.x), float32(-this.player.y), float32(-this.player.z)))
	return mat
}
