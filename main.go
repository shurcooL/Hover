package main

import (
	"fmt"
	"image"
	_ "image/png"
	"log"
	"math"
	"os"
	"runtime"
	"time"

	"github.com/go-gl/glow/gl/2.1/gl"
	"github.com/go-gl/mathgl/mgl64"
	glfw "github.com/shurcooL/glfw3"

	"github.com/shurcooL/go-goon"
)

var startedProcess = time.Now()
var windowSize [2]int

var wireframe bool

func main() {
	runtime.LockOSThread()
	runtime.GOMAXPROCS(runtime.NumCPU())

	if err := glfw.Init(); err != nil {
		log.Panicln("glfw.Init():", err)
	}
	defer glfw.Terminate()

	//glfw.WindowHint(glfw.Samples, 32) // Anti-aliasing
	window, err := glfw.CreateWindow(640, 480, "Hover", nil, nil)
	if err != nil {
		panic(err)
	}
	window.MakeContextCurrent()

	if err := gl.Init(); nil != err {
		log.Print(err)
	}

	glfw.SwapInterval(1) // Vsync

	LoadTexture("./dirt.png")

	framebufferSizeCallback := func(w *glfw.Window, framebufferSize0, framebufferSize1 int) {
		gl.Viewport(0, 0, int32(framebufferSize0), int32(framebufferSize1))

		windowSize[0], windowSize[1], _ = w.GetSize()
	}
	{
		var framebufferSize [2]int
		framebufferSize[0], framebufferSize[1], _ = window.GetFramebufferSize()
		framebufferSizeCallback(window, framebufferSize[0], framebufferSize[1])
	}
	window.SetFramebufferSizeCallback(framebufferSizeCallback)

	var lastMousePos mgl64.Vec2
	lastMousePos[0], lastMousePos[1], _ = window.GetCursorPosition()
	mousePos := func(w *glfw.Window, x, y float64) {
		sliders := []float64{x - lastMousePos[0], y - lastMousePos[1]}
		//axes := []float64{x, y}

		lastMousePos[0] = x
		lastMousePos[1] = y

		{
			isButtonPressed := [2]bool{
				mustAction(window.GetMouseButton(glfw.MouseButton1)) != glfw.Release,
				mustAction(window.GetMouseButton(glfw.MouseButton2)) != glfw.Release,
			}

			var moveSpeed = 1.0
			var rotateSpeed = 0.3

			if mustAction(window.GetKey(glfw.KeyLeftShift)) != glfw.Release || mustAction(window.GetKey(glfw.KeyRightShift)) != glfw.Release {
				moveSpeed *= 0.01
				rotateSpeed *= 0.1
			}

			if isButtonPressed[0] && !isButtonPressed[1] {
				camera.rh += rotateSpeed * sliders[0]
			} else if isButtonPressed[0] && isButtonPressed[1] {
				camera.x += moveSpeed * sliders[0] * math.Cos(mgl64.DegToRad(camera.rh))
				camera.y += -moveSpeed * sliders[0] * math.Sin(mgl64.DegToRad(camera.rh))
			} else if !isButtonPressed[0] && isButtonPressed[1] {
				camera.rh += rotateSpeed * sliders[0]
			}
			if isButtonPressed[0] && !isButtonPressed[1] {
				camera.x -= moveSpeed * sliders[1] * math.Sin(mgl64.DegToRad(camera.rh))
				camera.y -= moveSpeed * sliders[1] * math.Cos(mgl64.DegToRad(camera.rh))
			} else if isButtonPressed[0] && isButtonPressed[1] {
				camera.z -= moveSpeed * sliders[1]
			} else if !isButtonPressed[0] && isButtonPressed[1] {
				camera.rv -= rotateSpeed * sliders[1]
			}
			for camera.rh < 0 {
				camera.rh += 360
			}
			for camera.rh >= 360 {
				camera.rh -= 360
			}
			if camera.rv > 90 {
				camera.rv = 90
			}
			if camera.rv < -90 {
				camera.rv = -90
			}
			//fmt.Printf("Cam rot h = %v, v = %v\n", camera.rh, camera.rv)
		}
	}
	window.SetCursorPositionCallback(mousePos)

	window.SetKeyCallback(func(w *glfw.Window, key glfw.Key, scancode int, action glfw.Action, mods glfw.ModifierKey) {
		if action == glfw.Press || action == glfw.Repeat {
			switch key {
			case glfw.KeyEscape:
				window.SetShouldClose(true)
			case glfw.KeyF1:
				wireframe = !wireframe
				goon.DumpExpr(wireframe)
			case glfw.KeyF2:
				cameraIndex = (cameraIndex + 1) % len(cameras)
			}
		}
	})

	gl.ClearColor(0.85, 0.85, 0.85, 1)
	//gl.Enable(gl.CULL_FACE)
	gl.Enable(gl.DEPTH_TEST)

	fpsWidget := NewFpsWidget(mgl64.Vec2{0, 60})

	track = newTrack("./track1.dat")

	fmt.Printf("Loaded in %v ms.\n", time.Since(startedProcess).Seconds()*1000)

	// ---

	firstFrame := true
	for !mustBool(window.ShouldClose()) {
		frameStartTime := time.Now()

		glfw.PollEvents()

		// Input
		player.Input(window)

		player.Physics()

		gl.Clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)

		Set3DProjection()
		cameras[cameraIndex].Apply()
		track.Render()
		player.Render()

		Set2DProjection()
		fpsWidget.Render()

		fpsWidget.PushTimeToRender(time.Since(frameStartTime).Seconds() * 1000)

		window.SwapBuffers()
		runtime.Gosched()

		fpsWidget.PushTimeTotal(time.Since(frameStartTime).Seconds() * 1000)

		if firstFrame {
			fmt.Printf("First frame in %v ms.\n", time.Since(startedProcess).Seconds()*1000)
			firstFrame = false
		}
	}

	goon.DumpExpr(camera)
}

// ---

func Set2DProjection() {
	// Update the projection matrix
	gl.MatrixMode(gl.PROJECTION)
	gl.LoadIdentity()
	gl.Ortho(0, float64(windowSize[0]), float64(windowSize[1]), 0, -1, 1)
	gl.MatrixMode(gl.MODELVIEW)
	gl.LoadIdentity()
}

func Set3DProjection() {
	// Update the projection matrix
	gl.MatrixMode(gl.PROJECTION)
	gl.LoadIdentity()

	var projectionMatrix [16]float64
	perspMatrix := mgl64.Perspective(45, float64(windowSize[0])/float64(windowSize[1]), 0.1, 1000)
	for i := 0; i < 16; i++ {
		projectionMatrix[i] = float64(perspMatrix[i])
	}
	gl.MultMatrixd(&projectionMatrix[0])

	gl.MatrixMode(gl.MODELVIEW)
	gl.LoadIdentity()
}

// ---

func mustAction(action glfw.Action, err error) glfw.Action {
	if err != nil {
		panic(err)
	}
	return action
}

func mustBool(b bool, err error) bool {
	if err != nil {
		panic(err)
	}
	return b
}

// ---

func CheckGLError() {
	errorCode := gl.GetError()
	if errorCode != 0 {
		log.Panicln("GL Error:", errorCode)
	}
}

func LoadTexture(path string) {
	//fmt.Printf("Trying to load texture %q: ", path)

	// Open the file
	file, err := os.Open(path)
	if err != nil {
		fmt.Println(os.Getwd())
		log.Fatal(err)
	}
	defer file.Close()

	// Decode the image
	img, _, err := image.Decode(file)
	if err != nil {
		log.Fatal(err)
	}

	bounds := img.Bounds()
	//fmt.Printf("loaded %vx%v texture.\n", bounds.Dx(), bounds.Dy())

	var pixPointer *uint8
	switch img := img.(type) {
	case *image.RGBA:
		pixPointer = &img.Pix[0]
	case *image.NRGBA:
		pixPointer = &img.Pix[0]
	default:
		panic("Unsupported type.")
	}

	var texture uint32
	gl.GenTextures(1, &texture)
	gl.BindTexture(gl.TEXTURE_2D, texture)
	gl.TexParameteri(gl.TEXTURE_2D, gl.GENERATE_MIPMAP, gl.TRUE)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_LINEAR)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR)
	gl.TexImage2D(gl.TEXTURE_2D, 0, gl.RGBA, int32(bounds.Dx()), int32(bounds.Dy()), 0, gl.RGBA, gl.UNSIGNED_BYTE, gl.Ptr(pixPointer))
	CheckGLError()
}

// ---

// TODO: Import the stuff below instead of copy-pasting it.

// ---

func DrawBorderlessBox(pos, size mgl64.Vec2, backgroundColor mgl64.Vec3) {
	gl.Color3dv((*float64)(&backgroundColor[0]))
	gl.Rectd(float64(pos[0]), float64(pos[1]), float64(pos.Add(size)[0]), float64(pos.Add(size)[1]))
}

// ---

type Widget struct {
	pos  mgl64.Vec2
	size mgl64.Vec2
}

func NewWidget(pos, size mgl64.Vec2) Widget {
	return Widget{pos: pos, size: size}
}

// ---

type fpsSample struct{ Render, Total float64 }

type FpsWidget struct {
	Widget
	samples []fpsSample
}

func NewFpsWidget(pos mgl64.Vec2) *FpsWidget {
	return &FpsWidget{Widget: NewWidget(pos, mgl64.Vec2{})}
}

func (w *FpsWidget) Render() {
	gl.PushMatrix()
	defer gl.PopMatrix()
	gl.Translated(float64(w.pos[0]), float64(w.pos[1]), 0)
	gl.Begin(gl.LINES)
	gl.Color3d(1, 0, 0)
	gl.Vertex2d(float64(0), float64(-1000.0/60))
	gl.Vertex2d(float64(30), float64(-1000.0/60))
	gl.End()
	for index, sample := range w.samples {
		var color mgl64.Vec3
		if sample.Render <= 1000.0/60*1.25 {
			color = mgl64.Vec3{0, 0, 0}
		} else {
			color = mgl64.Vec3{1, 0, 0}
		}
		DrawBorderlessBox(mgl64.Vec2{float64(30 - len(w.samples) + index), -sample.Render}, mgl64.Vec2{1, sample.Render}, color)
		DrawBorderlessBox(mgl64.Vec2{float64(30 - len(w.samples) + index), -sample.Total}, mgl64.Vec2{1, sample.Total - sample.Render}, mgl64.Vec3{0.65, 0.65, 0.65})
	}
}

func (w *FpsWidget) PushTimeToRender(sample float64) {
	w.samples = append(w.samples, fpsSample{Render: sample})
	if len(w.samples) > 30 {
		w.samples = w.samples[len(w.samples)-30:]
	}
}
func (w *FpsWidget) PushTimeTotal(sample float64) {
	w.samples[len(w.samples)-1].Total = sample
}
