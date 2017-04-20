// Package track defines Hover track data structure
// and provides loading functionality.
package track

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"io"
	"io/ioutil"
)

const (
	// TerrHeightScale is the scale factor for terrain height.
	TerrHeightScale = 1.0 / 32

	triGroupNumBitsUsed = 510
	triGroupNumDwords   = (triGroupNumBitsUsed + 2) / 32
	triGroupWidthShift  = 4
)

// Track is a Hover track.
type Track struct {
	Header

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

// Header of a track file.
type Header struct {
	SunlightDirection, SunlightPitch float32
	RacerStartPositions              [8][3]float32
	NumTerrTypes                     uint16
	NumTerrTypeNodes                 uint16
	NumNavCoords                     uint16
	NumNavCoordLookupNodes           uint16
	Width, Depth                     uint16
}

// TerrTypeNode is a terrain type node.
type TerrTypeNode struct {
	Type       uint8
	_          uint8
	NextStartX uint16
	Next       uint16
}

// NavCoord is a navigation coordinate.
type NavCoord struct {
	X, Z             uint16
	DistToStartCoord uint16 // Decider at forks, and determines racers' rank/place.
	Next             uint16
	Alt              uint16
}

// NavCoordLookupNode is a navigation coordinate lookup node.
type NavCoordLookupNode struct {
	NavCoord   uint16
	NextStartX uint16
	Next       uint16
}

// TerrCoord is a terrain coordinate.
type TerrCoord struct {
	Height         uint16
	LightIntensity uint8
}

const terrCoordSize = 3

// loadTerrCoords loads n TerrCoords from r.
func loadTerrCoords(r io.Reader, n uint32) ([]TerrCoord, error) {
	b := make([]byte, n*terrCoordSize)
	_, err := io.ReadFull(r, b)
	if err != nil {
		return nil, err
	}
	tc := make([]TerrCoord, n)
	for i := 0; i < len(tc); i++ {
		offset := i * terrCoordSize
		tc[i].Height = uint16(b[offset+0]) | uint16(b[offset+1])<<8
		tc[i].LightIntensity = uint8(b[offset+2])
	}
	return tc, nil
}

// TriGroup is a triangle group.
type TriGroup struct {
	Data [triGroupNumDwords]uint32
}

// Load loads track from r.
func Load(r io.Reader) (Track, error) {
	var t Track

	err := binary.Read(r, binary.LittleEndian, &t.Header)
	if err != nil {
		return t, err
	}

	// Values derived from header.
	t.NumTerrCoords = uint32(t.Width) * uint32(t.Depth)
	t.TriGroupsWidth = (uint32(t.Width) - 1) >> triGroupWidthShift
	t.TriGroupsDepth = (uint32(t.Depth) - 1) >> triGroupWidthShift
	t.NumTriGroups = t.TriGroupsWidth * t.TriGroupsDepth

	t.TerrTypeTextureFilenames = make([]string, t.NumTerrTypes)
	for i := 0; i < len(t.TerrTypeTextureFilenames); i++ {
		var terrTypeTextureFilename [32]byte
		err = binary.Read(r, binary.LittleEndian, &terrTypeTextureFilename)
		if err != nil {
			return t, err
		}
		t.TerrTypeTextureFilenames[i] = cStringToGoString(terrTypeTextureFilename[:])
	}

	t.TerrTypeRuns = make([]TerrTypeNode, t.Depth)
	err = binary.Read(r, binary.LittleEndian, &t.TerrTypeRuns)
	if err != nil {
		return t, err
	}

	t.TerrTypeNodes = make([]TerrTypeNode, t.NumTerrTypeNodes)
	err = binary.Read(r, binary.LittleEndian, &t.TerrTypeNodes)
	if err != nil {
		return t, err
	}

	t.NavCoords = make([]NavCoord, t.NumNavCoords)
	err = binary.Read(r, binary.LittleEndian, &t.NavCoords)
	if err != nil {
		return t, err
	}

	t.NavCoordLookupRuns = make([]NavCoordLookupNode, t.Depth)
	err = binary.Read(r, binary.LittleEndian, &t.NavCoordLookupRuns)
	if err != nil {
		return t, err
	}

	t.NavCoordLookupNodes = make([]NavCoordLookupNode, t.NumNavCoordLookupNodes)
	err = binary.Read(r, binary.LittleEndian, &t.NavCoordLookupNodes)
	if err != nil {
		return t, err
	}

	// TerrCoords contains the largest amount of data. Optimize decoding by
	// writing custom code, rather than relying on reflection-based binary.Read.
	t.TerrCoords, err = loadTerrCoords(r, t.NumTerrCoords)
	if err != nil {
		return t, err
	}

	t.TriGroups = make([]TriGroup, t.NumTriGroups)
	err = binary.Read(r, binary.LittleEndian, &t.TriGroups)
	if err != nil {
		return t, err
	}

	// Check that we've consumed the entire track file.
	if n, err := io.Copy(ioutil.Discard, r); err != nil {
		return t, err
	} else if n > 0 {
		return t, fmt.Errorf("did not get to end of track file, %d bytes left", n)
	}

	return t, nil
}

// cStringToGoString converts a C-style null-terminated string to a Go string.
func cStringToGoString(cString []byte) string {
	i := bytes.IndexByte(cString, 0)
	if i < 0 {
		return ""
	}
	return string(cString[:i])
}
