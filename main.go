package main

import (
	"encoding/binary"
	"fmt"
	"image"
	_ "image/png"
	"log"
	"math"
	"os"
	"runtime"
	"time"
	"unsafe"

	"github.com/go-gl/glow/gl/2.1/gl"
	"github.com/go-gl/mathgl/mgl64"
	glfw "github.com/shurcooL/glfw3"

	"github.com/shurcooL/go-goon"
)

const TRIGROUP_NUM_BITS_USED = 510
const TRIGROUP_NUM_DWORDS = (TRIGROUP_NUM_BITS_USED + 2) / 32
const TRIGROUP_WIDTHSHIFT = 4
const TERR_HEIGHT_SCALE = 1.0 / 32
const TERR_TEXTURE_SCALE = 1.0 / 20

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

type TrackFileHeader struct {
	SunlightDirection, SunlightPitch float32
	RacerStartPositions              [8][3]float32
	NumTerrTypes                     uint16
	NumTerrTypeNodes                 uint16
	NumNavCoords                     uint16
	NumNavCoordLookupNodes           uint16
	Width, Depth                     uint16
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

	vertexVbo       uint32
	colorVbo        uint32
	textureCoordVbo uint32
}

func loadTrack() *Track {
	const path = "./track1.dat"

	file, err := os.Open(path)
	if err != nil {
		panic(err)
	}
	defer file.Close()

	var track Track

	binary.Read(file, binary.LittleEndian, &track.TrackFileHeader)

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

	fi, err := file.Stat()
	if err != nil {
		panic(err)
	}
	fileOffset, err := file.Seek(0, os.SEEK_CUR)
	if err != nil {
		panic(err)
	}
	fmt.Printf("Read %v of %v bytes.\n", fileOffset, fi.Size())

	{
		rowCount := uint64(track.Depth) - 1
		rowLength := uint64(track.Width)

		vertexData := make([][3]float32, 2*rowLength*rowCount)
		colorData := make([][3]byte, 2*rowLength*rowCount)
		textureCoordData := make([][2]float32, 2*rowLength*rowCount)

		var index uint64
		for y := uint16(1); y < track.Depth; y++ {
			for x := uint16(0); x < track.Width; x++ {
				for i := uint16(0); i < 2; i++ {
					yy := y - i

					terrCoord := track.TerrCoords[uint64(yy)*uint64(track.Width)+uint64(x)]
					height := float64(terrCoord.Height) * TERR_HEIGHT_SCALE
					lightIntensity := byte(terrCoord.LightIntensity)

					vertexData[index] = [3]float32{float32(x), float32(yy), float32(height)}
					colorData[index] = [3]byte{lightIntensity, lightIntensity, lightIntensity}
					textureCoordData[index] = [2]float32{float32(float32(x) * TERR_TEXTURE_SCALE), float32(float32(yy) * TERR_TEXTURE_SCALE)}
					index++
				}
			}
		}

		track.vertexVbo = createVbo3Float(vertexData)
		track.colorVbo = createVbo3Ubyte(colorData)
		track.textureCoordVbo = createVbo2Float(textureCoordData)
	}

	return &track
}

func (track *Track) getHeightAt(x, y float64) float64 {
	// TODO: Interpolate between 4 points.
	return track.getHeightAtPoint(uint64(x), uint64(y))
}

func (track *Track) getHeightAtPoint(x, y uint64) float64 {
	if x > uint64(track.Width)-1 {
		x = uint64(track.Width) - 1
	}
	if y > uint64(track.Depth)-1 {
		y = uint64(track.Depth) - 1
	}

	terrCoord := track.TerrCoords[y*uint64(track.Width)+x]
	height := float64(terrCoord.Height) * TERR_HEIGHT_SCALE
	return height
}

func createVbo3Float(vertices [][3]float32) uint32 {
	var vbo uint32
	gl.GenBuffers(1, &vbo)
	gl.BindBuffer(gl.ARRAY_BUFFER, vbo)
	defer gl.BindBuffer(gl.ARRAY_BUFFER, 0)

	gl.BufferData(gl.ARRAY_BUFFER, int(unsafe.Sizeof(vertices[0]))*len(vertices), gl.Ptr(vertices), gl.STATIC_DRAW)

	return vbo
}

func createVbo2Float(vertices [][2]float32) uint32 {
	var vbo uint32
	gl.GenBuffers(1, &vbo)
	gl.BindBuffer(gl.ARRAY_BUFFER, vbo)
	defer gl.BindBuffer(gl.ARRAY_BUFFER, 0)

	gl.BufferData(gl.ARRAY_BUFFER, int(unsafe.Sizeof(vertices[0]))*len(vertices), gl.Ptr(vertices), gl.STATIC_DRAW)

	return vbo
}

func createVbo3Ubyte(vertices [][3]byte) uint32 {
	var vbo uint32
	gl.GenBuffers(1, &vbo)
	gl.BindBuffer(gl.ARRAY_BUFFER, vbo)
	defer gl.BindBuffer(gl.ARRAY_BUFFER, 0)

	gl.BufferData(gl.ARRAY_BUFFER, int(unsafe.Sizeof(vertices[0]))*len(vertices), gl.Ptr(vertices), gl.STATIC_DRAW)

	return vbo
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

	var texture uint32
	gl.GenTextures(1, &texture)
	gl.BindTexture(gl.TEXTURE_2D, texture)
	gl.TexParameteri(gl.TEXTURE_2D, gl.GENERATE_MIPMAP, gl.TRUE)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_LINEAR)
	gl.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR)
	gl.TexImage2D(gl.TEXTURE_2D, 0, gl.RGBA, int32(bounds.Dx()), int32(bounds.Dy()), 0, gl.RGBA, gl.UNSIGNED_BYTE, gl.Ptr(pixPointer))
	CheckGLError()
}

func (track *Track) Render() {
	gl.PushMatrix()
	defer gl.PopMatrix()

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

	{
		rowCount := uint64(track.Depth) - 1
		rowLength := uint64(track.Width)

		gl.EnableClientState(gl.VERTEX_ARRAY)
		gl.BindBuffer(gl.ARRAY_BUFFER, track.vertexVbo)
		gl.VertexPointer(3, gl.FLOAT, 0, nil)

		gl.EnableClientState(gl.COLOR_ARRAY)
		gl.BindBuffer(gl.ARRAY_BUFFER, track.colorVbo)
		gl.ColorPointer(3, gl.UNSIGNED_BYTE, 0, nil)

		gl.EnableClientState(gl.TEXTURE_COORD_ARRAY)
		gl.BindBuffer(gl.ARRAY_BUFFER, track.textureCoordVbo)
		gl.TexCoordPointer(2, gl.FLOAT, 0, nil)

		for row := uint64(0); row < rowCount; row++ {
			gl.DrawArrays(gl.TRIANGLE_STRIP, int32(row*2*rowLength), int32(2*rowLength))
		}

		gl.DisableClientState(gl.VERTEX_ARRAY)
		gl.DisableClientState(gl.COLOR_ARRAY)
		gl.DisableClientState(gl.TEXTURE_COORD_ARRAY)
	}
	gl.Disable(gl.TEXTURE_2D)

	if wireframe {
		gl.PolygonMode(gl.FRONT_AND_BACK, gl.FILL)
	}
}

var startedProcess = time.Now()
var windowSize [2]int

var wireframe bool

var track *Track

func main() {
	runtime.LockOSThread()
	runtime.GOMAXPROCS(runtime.NumCPU())

	if err := glfw.Init(); err != nil {
		log.Panicln("glfw.Init():", err)
	}
	defer glfw.Terminate()

	glfw.WindowHint(glfw.Samples, 32) // Anti-aliasing
	window, err := glfw.CreateWindow(640, 480, "", nil, nil)
	if err != nil {
		panic(err)
	}
	window.SetTitle("Hover")
	window.MakeContextCurrent()

	if err := gl.Init(); nil != err {
		log.Print(err)
	}

	glfw.SwapInterval(1) // Vsync

	LoadTexture("./dirt.png")

	framebufferSizeCallback := func(w *glfw.Window, framebufferSize0, framebufferSize1 int) {
		gl.Viewport(0, 0, int32(framebufferSize0), int32(framebufferSize1))

		windowSize[0], windowSize[1] = w.GetSize()
	}
	{
		var framebufferSize [2]int
		framebufferSize[0], framebufferSize[1] = window.GetFramebufferSize()
		framebufferSizeCallback(window, framebufferSize[0], framebufferSize[1])
	}
	window.SetFramebufferSizeCallback(framebufferSizeCallback)

	var lastMousePos mgl64.Vec2
	lastMousePos[0], lastMousePos[1] = window.GetCursorPosition()
	mousePos := func(w *glfw.Window, x, y float64) {
		sliders := []float64{x - lastMousePos[0], y - lastMousePos[1]}
		//axes := []float64{x, y}

		lastMousePos[0] = x
		lastMousePos[1] = y

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
	mousePos(window, lastMousePos[0], lastMousePos[1])

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

	track = loadTrack()

	fmt.Printf("Loaded in %v ms.\n", time.Since(startedProcess).Seconds()*1000)

	// ---

	for !window.ShouldClose() {
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
	}

	goon.DumpExpr(camera)
}

// ---

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

var player = Hovercraft{x: 250.8339829707148, y: 630.3799668664172, z: 565, r: 0}

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
