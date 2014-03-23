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

const TRIGROUP_NUM_BITS_USED = 510
const TRIGROUP_NUM_DWORDS = ((TRIGROUP_NUM_BITS_USED + 2) / 32)
const TRIGROUP_WIDTHSHIFT = 4

type TerrTypeNode struct {
	Type       uint8
	NextStartX uint16
	Next       uint16
	_          uint8
}

type NavCoord struct {
	X, Z             uint16
	DistToStartCoord uint16 // Decider at forks, and determines racers' rank/place.
	Next             uint16
	Alt              uint16
}

type NavCoordLookupNode struct {
	NavCoord   uint16
	NextStartX uint16
	Next       uint16
}

type TerrCoord struct {
	Height         uint16
	LightIntensity uint8
}

type TriGroup struct {
	Data [TRIGROUP_NUM_DWORDS]uint32
}

type Track struct {
	TrackFileHeader
	NumTerrCoords  uint32
	TriGroupsWidth uint32
	TriGroupsDepth uint32
	NumTriGroups   uint32

	TerrTypeTextureFilenames []string

	TerrTypeRuns  []TerrTypeNode
	TerrTypeNodes []TerrTypeNode

	NavCoords           []NavCoord
	NavCoordLookupRuns  []NavCoordLookupNode
	NavCoordLookupNodes []NavCoordLookupNode

	TerrCoords []TerrCoord
	TriGroups  []TriGroup
}

func loadTrack() *Track {
	path := "./Hover/track1.dat"

	file, err := os.Open(path)
	CheckError(err)
	defer file.Close()

	var track Track

	binary.Read(file, binary.LittleEndian, &track.TrackFileHeader)
	//goon.DumpExpr(header)

	// Stuff derived from header info.
	track.NumTerrCoords = uint32(track.Width) * uint32(track.Depth)
	track.TriGroupsWidth = (uint32(track.Width) - 1) >> TRIGROUP_WIDTHSHIFT
	track.TriGroupsDepth = (uint32(track.Depth) - 1) >> TRIGROUP_WIDTHSHIFT
	track.NumTriGroups = track.TriGroupsWidth * track.TriGroupsDepth

	track.TerrTypeTextureFilenames = make([]string, track.NumTerrTypes)
	for i := uint16(0); i < track.NumTerrTypes; i++ {
		var terrTypeTextureFilename [32]byte
		binary.Read(file, binary.LittleEndian, &terrTypeTextureFilename)
		track.TerrTypeTextureFilenames[i] = CToGoString(terrTypeTextureFilename[:])
	}

	track.TerrTypeRuns = make([]TerrTypeNode, track.Depth)
	binary.Read(file, binary.LittleEndian, &track.TerrTypeRuns)

	track.TerrTypeNodes = make([]TerrTypeNode, track.NumTerrTypeNodes)
	binary.Read(file, binary.LittleEndian, &track.TerrTypeNodes)

	track.NavCoords = make([]NavCoord, track.NumNavCoords)
	binary.Read(file, binary.LittleEndian, &track.NavCoords)

	track.NavCoordLookupRuns = make([]NavCoordLookupNode, track.Depth)
	binary.Read(file, binary.LittleEndian, &track.NavCoordLookupRuns)

	track.NavCoordLookupNodes = make([]NavCoordLookupNode, track.NumNavCoordLookupNodes)
	binary.Read(file, binary.LittleEndian, &track.NavCoordLookupNodes)

	track.TerrCoords = make([]TerrCoord, track.NumTerrCoords)
	binary.Read(file, binary.LittleEndian, &track.TerrCoords)

	track.TriGroups = make([]TriGroup, track.NumTriGroups)
	binary.Read(file, binary.LittleEndian, &track.TriGroups)

	//goon.Dump(track)

	fi, err := file.Stat()
	CheckError(err)
	fileOffset, err := file.Seek(0, os.SEEK_CUR)
	CheckError(err)
	fmt.Printf("Read %v of %v bytes.\n", fileOffset, fi.Size())

	return &track
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

	var modelviewMatrix [16]gl.Double
	lookAtMatrix := mathgl.LookAtd(-128, -128, 256, 128, 128, 0, 0, 0, 1)
	for i := 0; i < 16; i++ {
		modelviewMatrix[i] = gl.Double(lookAtMatrix[i])
	}
	gl.LoadMatrixd(&modelviewMatrix[0])

	gl.Color3f(1, 1, 1)

	if wireframe {
		gl.PolygonMode(gl.FRONT_AND_BACK, gl.LINE)
	}

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

	if wireframe {
		gl.PolygonMode(gl.FRONT_AND_BACK, gl.FILL)
	}
}

var startedProcess = time.Now()
var windowSize [2]int

var wireframe bool

func main() {
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

		windowSize[0], windowSize[1] = w.GetSize()
	}
	{
		var framebufferSize [2]int
		framebufferSize[0], framebufferSize[1] = window.GetFramebufferSize()
		framebufferSizeCallback(window, framebufferSize[0], framebufferSize[1])
	}
	window.SetFramebufferSizeCallback(framebufferSizeCallback)

	window.SetKeyCallback(func(w *glfw.Window, key glfw.Key, scancode int, action glfw.Action, mods glfw.ModifierKey) {
		if action == glfw.Press || action == glfw.Repeat {
			switch key {
			case glfw.KeyF1:
				wireframe = !wireframe
				goon.DumpExpr(wireframe)
			}
		}
	})

	gl.ClearColor(0.85, 0.85, 0.85, 1)
	//gl.Enable(gl.CULL_FACE)

	fpsWidget := NewFpsWidget(mathgl.Vec2d{0, 60})

	track := loadTrack()
	_ = track

	fmt.Printf("Loaded in %v ms.\n", time.Since(startedProcess).Seconds()*1000)

	// ---

	for !window.ShouldClose() && glfw.Press != window.GetKey(glfw.KeyEscape) {
		frameStartTime := time.Now()

		glfw.PollEvents()

		// Input

		gl.Clear(gl.COLOR_BUFFER_BIT)

		Set3DProjection()
		Render()

		Set2DProjection()
		fpsWidget.Render()

		fpsWidget.PushTimeToRender(time.Since(frameStartTime).Seconds() * 1000)

		window.SwapBuffers()
		runtime.Gosched()

		fpsWidget.PushTimeTotal(time.Since(frameStartTime).Seconds() * 1000)
	}
}

func Set2DProjection() {
	// Update the projection matrix
	gl.MatrixMode(gl.PROJECTION)
	gl.LoadIdentity()
	gl.Ortho(0, gl.Double(windowSize[0]), gl.Double(windowSize[1]), 0, -1, 1)
	gl.MatrixMode(gl.MODELVIEW)
	gl.LoadIdentity()
}

func Set3DProjection() {
	// Update the projection matrix
	gl.MatrixMode(gl.PROJECTION)
	gl.LoadIdentity()

	var projectionMatrix [16]gl.Double
	perspMatrix := mathgl.Perspectived(45, float64(windowSize[0])/float64(windowSize[1]), 0.1, 1000)
	for i := 0; i < 16; i++ {
		projectionMatrix[i] = gl.Double(perspMatrix[i])
	}
	gl.MultMatrixd(&projectionMatrix[0])

	gl.MatrixMode(gl.MODELVIEW)
	gl.LoadIdentity()
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
