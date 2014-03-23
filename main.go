package main

import (
	"encoding/binary"
	"fmt"
	"image"
	_ "image/png"
	"log"
	"os"
	"runtime"
	"time"

	"github.com/Jragonmiris/mathgl"
	gl "github.com/chsc/gogl/gl21"
	glfw "github.com/go-gl/glfw3"
	"github.com/shurcooL/go-goon"

	. "gist.github.com/5286084.git"
)

type TrackFileHeader struct {
	SunlightDirection, SunlightPitch float32
	RacerStartPositions              [8]mathgl.Vec3f
	NumTerrTypes                     uint16
	NumTerrTypeNodes                 uint16
	NumNavCoords                     uint16
	NumNavCoordLookupNodes           uint16
	Width, Depth                     uint16
}

const MAX_REAL_TERRAIN_TYPES = 8
const NUM_ED_TYPES = 4
const MAX_TERRAIN_TYPES = (MAX_REAL_TERRAIN_TYPES + NUM_ED_TYPES)

type TerrTypeNode struct {
	Type       uint8
	NextStartX uint16
	Next       uint16
}

type Track struct {
	TerrTypeTextureFilenames [MAX_TERRAIN_TYPES]string

	TerrTypeRuns  []TerrTypeNode
	TerrTypeNodes []TerrTypeNode
}

func loadTrack() {
	path := "./Hover/track1.dat"

	file, err := os.Open(path)
	CheckError(err)
	defer file.Close()

	var header TrackFileHeader
	binary.Read(file, binary.LittleEndian, &header)
	goon.DumpExpr(header)

	var track Track

	for i := uint16(0); i < header.NumTerrTypes; i++ {
		var terrTypeTextureFilename [32]byte
		binary.Read(file, binary.LittleEndian, &terrTypeTextureFilename)
		track.TerrTypeTextureFilenames[i] = CToGoString(terrTypeTextureFilename[:])
	}

	track.TerrTypeRuns = make([]TerrTypeNode, header.Depth)
	binary.Read(file, binary.LittleEndian, &track.TerrTypeRuns)

	track.TerrTypeRuns = make([]TerrTypeNode, header.NumTerrTypeNodes)
	binary.Read(file, binary.LittleEndian, &track.TerrTypeRuns)

	//goon.Dump(track)

	fi, err := file.Stat()
	CheckError(err)
	fileOffset, err := file.Seek(0, os.SEEK_CUR)
	CheckError(err)
	fmt.Printf("Read %v of %v bytes.\n", fileOffset, fi.Size())

	os.Exit(0)
}

func CToGoString(c []byte) string {
	n := -1
	for i, b := range c {
		if b == 0 {
			break
		}
		n = i
	}
	return string(c[:n+1])
}

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

	var texture gl.Uint
	gl.GenTextures(1, &texture)
	gl.BindTexture(gl.TEXTURE_2D, texture)
	gl.TexParameteri(gl.TEXTURE_2D, gl.GENERATE_MIPMAP, gl.TRUE)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_LINEAR)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR)
	gl.TexImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.Sizei(bounds.Dx()), gl.Sizei(bounds.Dy()), 0, gl.RGBA, gl.UNSIGNED_BYTE, gl.Pointer(pixPointer))
	CheckGLError()
}

func Render() {
	gl.PushMatrix()
	defer gl.PopMatrix()
	gl.Translatef(100, 100, 0)
	gl.Color3f(1, 1, 1)

	gl.Enable(gl.TEXTURE_2D)
	gl.Begin(gl.TRIANGLE_FAN)
	{
		const size = 256
		gl.TexCoord2i(0, 0)
		gl.Vertex2i(0, 0)
		gl.TexCoord2i(1, 0)
		gl.Vertex2i(size, 0)
		gl.TexCoord2i(1, 1)
		gl.Vertex2i(size, size)
		gl.TexCoord2i(0, 1)
		gl.Vertex2i(0, size)
	}
	gl.End()
	gl.Disable(gl.TEXTURE_2D)
}

var startedProcess = time.Now()

func main() {
	loadTrack()

	runtime.LockOSThread()
	runtime.GOMAXPROCS(runtime.NumCPU())

	glfw.SetErrorCallback(func(err glfw.ErrorCode, desc string) {
		panic(fmt.Sprintf("glfw.ErrorCallback: %v: %v\n", err, desc))
	})

	if !glfw.Init() {
		panic("glfw.Init()")
	}
	defer glfw.Terminate()

	//glfw.WindowHint(glfw.Samples, 32) // Anti-aliasing
	window, err := glfw.CreateWindow(640, 480, "", nil, nil)
	CheckError(err)
	window.SetTitle("Hover")
	window.MakeContextCurrent()

	if err := gl.Init(); nil != err {
		log.Print(err)
	}

	glfw.SwapInterval(1) // Vsync

	LoadTexture("./sand.png")

	framebufferSizeCallback := func(w *glfw.Window, framebufferSize0, framebufferSize1 int) {
		gl.Viewport(0, 0, gl.Sizei(framebufferSize0), gl.Sizei(framebufferSize1))

		var windowSize [2]int
		windowSize[0], windowSize[1] = w.GetSize()

		// Update the projection matrix
		gl.MatrixMode(gl.PROJECTION)
		gl.LoadIdentity()
		gl.Ortho(0, gl.Double(windowSize[0]), gl.Double(windowSize[1]), 0, -1, 1)
		gl.MatrixMode(gl.MODELVIEW)
	}
	{
		var framebufferSize [2]int
		framebufferSize[0], framebufferSize[1] = window.GetFramebufferSize()
		framebufferSizeCallback(window, framebufferSize[0], framebufferSize[1])
	}
	window.SetFramebufferSizeCallback(framebufferSizeCallback)

	gl.ClearColor(0.85, 0.85, 0.85, 1)

	fpsWidget := NewFpsWidget(mathgl.Vec2d{0, 60})

	fmt.Printf("Loaded in %v ms.\n", time.Since(startedProcess).Seconds()*1000)

	// ---

	for !window.ShouldClose() && glfw.Press != window.GetKey(glfw.KeyEscape) {
		frameStartTime := time.Now()

		glfw.PollEvents()

		// Input

		gl.Clear(gl.COLOR_BUFFER_BIT)
		gl.LoadIdentity()

		Render()
		fpsWidget.Render()

		fpsWidget.PushTimeToRender(time.Since(frameStartTime).Seconds() * 1000)

		window.SwapBuffers()
		runtime.Gosched()

		fpsWidget.PushTimeTotal(time.Since(frameStartTime).Seconds() * 1000)
	}
}

// TODO: Import the stuff below instead of copy-pasting it.

// ---

func DrawBorderlessBox(pos, size mathgl.Vec2d, backgroundColor mathgl.Vec3d) {
	gl.Color3dv((*gl.Double)(&backgroundColor[0]))
	gl.Rectd(gl.Double(pos[0]), gl.Double(pos[1]), gl.Double(pos.Add(size)[0]), gl.Double(pos.Add(size)[1]))
}

// ---

type Widget struct {
	pos  mathgl.Vec2d
	size mathgl.Vec2d
}

func NewWidget(pos, size mathgl.Vec2d) Widget {
	return Widget{pos: pos, size: size}
}

// ---

type fpsSample struct{ Render, Total float64 }

type FpsWidget struct {
	Widget
	samples []fpsSample
}

func NewFpsWidget(pos mathgl.Vec2d) *FpsWidget {
	return &FpsWidget{Widget: NewWidget(pos, mathgl.Vec2d{})}
}

func (w *FpsWidget) Render() {
	gl.PushMatrix()
	defer gl.PopMatrix()
	gl.Translated(gl.Double(w.pos[0]), gl.Double(w.pos[1]), 0)
	gl.Begin(gl.LINES)
	gl.Color3d(1, 0, 0)
	gl.Vertex2d(gl.Double(0), gl.Double(-1000.0/60))
	gl.Vertex2d(gl.Double(30), gl.Double(-1000.0/60))
	gl.End()
	for index, sample := range w.samples {
		var color mathgl.Vec3d
		if sample.Render <= 1000.0/60*1.25 {
			color = mathgl.Vec3d{0, 0, 0}
		} else {
			color = mathgl.Vec3d{1, 0, 0}
		}
		DrawBorderlessBox(mathgl.Vec2d{float64(30 - len(w.samples) + index), -sample.Render}, mathgl.Vec2d{1, sample.Render}, color)
		DrawBorderlessBox(mathgl.Vec2d{float64(30 - len(w.samples) + index), -sample.Total}, mathgl.Vec2d{1, sample.Total - sample.Render}, mathgl.Vec3d{0.65, 0.65, 0.65})
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
