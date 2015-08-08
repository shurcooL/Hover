// Hover is a work-in-progress port of Hover, a game originally created by Eric Undersander in 2000.
package main

import (
	"flag"
	"fmt"
	"image"
	_ "image/png"
	"log"
	"math"
	"runtime"
	"time"

	"github.com/go-gl/mathgl/mgl32"
	"github.com/go-gl/mathgl/mgl64"
	"github.com/goxjs/gl"
	"github.com/goxjs/glfw"
)

var stateFileFlag = flag.String("state-file", "Hover.state", "File to save/load state.")

var startedProcess = time.Now()
var windowSize [2]int

var wireframe bool

func init() {
	runtime.LockOSThread()
	runtime.GOMAXPROCS(runtime.NumCPU())
}

func main() {
	flag.Parse()

	if *stateFileFlag != "" {
		err := loadState(*stateFileFlag)
		fmt.Println("loadState:", err)
	}

	err := glfw.Init(gl.ContextWatcher)
	if err != nil {
		log.Panicln("glfw.Init:", err)
	}
	defer glfw.Terminate()

	//glfw.WindowHint(glfw.Samples, 8) // Anti-aliasing.

	window, err := glfw.CreateWindow(1024, 800, "Hover", nil, nil)
	if err != nil {
		panic(err)
	}
	window.MakeContextCurrent()

	fmt.Printf("OpenGL: %s %s %s; %v samples.\n", gl.GetString(gl.VENDOR), gl.GetString(gl.RENDERER), gl.GetString(gl.VERSION), gl.GetInteger(gl.SAMPLES))
	fmt.Printf("GLSL: %s.\n", gl.GetString(gl.SHADING_LANGUAGE_VERSION))

	glfw.SwapInterval(1) // Vsync.

	framebufferSizeCallback := func(w *glfw.Window, framebufferSize0, framebufferSize1 int) {
		gl.Viewport(0, 0, framebufferSize0, framebufferSize1)

		windowSize[0], windowSize[1] = w.GetSize()
	}
	{
		var framebufferSize [2]int
		framebufferSize[0], framebufferSize[1] = window.GetFramebufferSize()
		framebufferSizeCallback(window, framebufferSize[0], framebufferSize[1])
	}
	window.SetFramebufferSizeCallback(framebufferSizeCallback)

	mouseMovement := func(_ *glfw.Window, xpos, ypos, xdelta, ydelta float64) {
		sliders := []float64{xdelta, ydelta}

		{
			isButtonPressed := [2]bool{
				window.GetMouseButton(glfw.MouseButton1) != glfw.Release,
				window.GetMouseButton(glfw.MouseButton2) != glfw.Release,
			}

			var moveSpeed = 1.0
			const rotateSpeed = 0.3

			if window.GetKey(glfw.KeyLeftShift) != glfw.Release || window.GetKey(glfw.KeyRightShift) != glfw.Release {
				moveSpeed *= 0.01
			}

			if isButtonPressed[0] && !isButtonPressed[1] {
				camera.RH += rotateSpeed * sliders[0]
			} else if isButtonPressed[0] && isButtonPressed[1] {
				camera.X += moveSpeed * sliders[0] * math.Cos(mgl64.DegToRad(camera.RH))
				camera.Y += -moveSpeed * sliders[0] * math.Sin(mgl64.DegToRad(camera.RH))
			} else if !isButtonPressed[0] && isButtonPressed[1] {
				camera.RH += rotateSpeed * sliders[0]
			}
			if isButtonPressed[0] && !isButtonPressed[1] {
				camera.X -= moveSpeed * sliders[1] * math.Sin(mgl64.DegToRad(camera.RH))
				camera.Y -= moveSpeed * sliders[1] * math.Cos(mgl64.DegToRad(camera.RH))
			} else if isButtonPressed[0] && isButtonPressed[1] {
				camera.Z -= moveSpeed * sliders[1]
			} else if !isButtonPressed[0] && isButtonPressed[1] {
				camera.RV -= rotateSpeed * sliders[1]
			}
			for camera.RH < 0 {
				camera.RH += 360
			}
			for camera.RH >= 360 {
				camera.RH -= 360
			}
			if camera.RV > 90 {
				camera.RV = 90
			}
			if camera.RV < -90 {
				camera.RV = -90
			}
		}
	}
	window.SetMouseMovementCallback(mouseMovement)

	window.SetMouseButtonCallback(func(_ *glfw.Window, button glfw.MouseButton, action glfw.Action, mods glfw.ModifierKey) {
		isButtonPressed := [2]bool{
			window.GetMouseButton(glfw.MouseButton1) != glfw.Release,
			window.GetMouseButton(glfw.MouseButton2) != glfw.Release,
		}

		if isButtonPressed[0] || isButtonPressed[1] {
			window.SetInputMode(glfw.CursorMode, glfw.CursorDisabled)
		} else {
			window.SetInputMode(glfw.CursorMode, glfw.CursorNormal)
		}
	})

	window.SetKeyCallback(func(w *glfw.Window, key glfw.Key, scancode int, action glfw.Action, mods glfw.ModifierKey) {
		if action == glfw.Press || action == glfw.Repeat {
			switch key {
			case glfw.KeyEscape:
				window.SetShouldClose(true)
			case glfw.KeyF1:
				wireframe = !wireframe
				fmt.Println("wireframe:", wireframe) //goon.DumpExpr(wireframe)
			case glfw.KeyF2:
				cameraIndex = (cameraIndex + 1) % len(cameras)
				switch cameraIndex {
				case 0:
					window.SetInputMode(glfw.CursorMode, glfw.CursorNormal)
				case 1:
					window.SetInputMode(glfw.CursorMode, glfw.CursorHidden)
				}
			}
		}
	})

	fpsWidget := NewFpsWidget(mgl64.Vec2{0, 60})

	track, err = newTrack("./track1.dat")
	if err != nil {
		panic(err)
	}

	err = loadModel()
	if err != nil {
		panic(err)
	}

	err = loadDebugShape()
	if err != nil {
		panic(err)
	}

	gl.ClearColor(0.85, 0.85, 0.85, 1)
	gl.Clear(gl.COLOR_BUFFER_BIT)
	//gl.Enable(gl.CULL_FACE)
	gl.Enable(gl.DEPTH_TEST)

	fmt.Printf("Loaded in %v ms.\n", time.Since(startedProcess).Seconds()*1000)

	// ---

	firstFrame := true
	for !window.ShouldClose() {
		frameStartTime := time.Now()

		glfw.PollEvents()

		// Input
		player.Input(window)

		player.Physics()

		gl.Clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)

		pMatrix = Set3DProjection()
		mvMatrix = cameras[cameraIndex].Apply()

		track.Render()

		player.Render()

		//Set2DProjection()
		//fpsWidget.Render()

		fpsWidget.PushTimeToRender(time.Since(frameStartTime).Seconds() * 1000)

		window.SwapBuffers()
		runtime.Gosched()

		fpsWidget.PushTimeTotal(time.Since(frameStartTime).Seconds() * 1000)

		if firstFrame {
			fmt.Printf("First frame in %v ms.\n", time.Since(startedProcess).Seconds()*1000)
			firstFrame = false
		}
	}

	if *stateFileFlag != "" {
		err := saveState(*stateFileFlag)
		fmt.Println("saveState:", err)
	}
}

// ---

/*func Set2DProjection() mgl32.Mat4 {
	// Update the projection matrix
	gl.MatrixMode(gl.PROJECTION)
	gl.LoadIdentity()
	gl.Ortho(0, float64(windowSize[0]), float64(windowSize[1]), 0, -1, 1)
	gl.MatrixMode(gl.MODELVIEW)
	gl.LoadIdentity()
}*/

func Set3DProjection() mgl32.Mat4 {
	return mgl32.Perspective(mgl32.DegToRad(45), float32(windowSize[0])/float32(windowSize[1]), 0.1, 1000)
}

// ---

func CheckGLError() {
	errorCode := gl.GetError()
	if errorCode != 0 {
		log.Panicln("GL Error:", errorCode)
	}
}

func loadTexture(path string) (gl.Texture, error) {
	fmt.Printf("Trying to load texture %q: ", path)
	started := time.Now()
	defer func() {
		fmt.Println("taken:", time.Since(started))
	}()

	// Open the file
	file, err := glfw.Open(path)
	if err != nil {
		return gl.Texture{}, err
	}
	defer file.Close()

	// Decode the image
	img, _, err := image.Decode(file)
	if err != nil {
		return gl.Texture{}, err
	}

	bounds := img.Bounds()
	fmt.Printf("loaded %vx%v texture.\n", bounds.Dx(), bounds.Dy())

	var pix []byte
	switch img := img.(type) {
	case *image.RGBA:
		pix = img.Pix
	case *image.NRGBA:
		pix = img.Pix
	default:
		panic("Unsupported image type.")
	}

	texture := gl.CreateTexture()
	gl.BindTexture(gl.TEXTURE_2D, texture)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_LINEAR)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR)
	gl.TexImage2D(gl.TEXTURE_2D, 0, bounds.Dx(), bounds.Dy(), gl.RGBA, gl.UNSIGNED_BYTE, pix)
	gl.GenerateMipmap(gl.TEXTURE_2D)

	if glError := gl.GetError(); glError != 0 {
		return gl.Texture{}, fmt.Errorf("gl.GetError: %v", glError)
	}

	return texture, nil
}

// ---

// TODO: Import the stuff below instead of copy-pasting it.

// ---

/*func DrawBorderlessBox(pos, size mgl64.Vec2, backgroundColor mgl64.Vec3) {
	gl.Color3dv((*float64)(&backgroundColor[0]))
	gl.Rectd(float64(pos[0]), float64(pos[1]), float64(pos.Add(size)[0]), float64(pos.Add(size)[1]))
}*/

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
	/*gl.PushMatrix()
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
	}*/
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
