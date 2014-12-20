package main

import "github.com/shurcooL/go-goon"

func ExampleCStringToGoString() {
	{
		c := []uint8{100, 105, 114, 116, 46, 98, 109, 112, 0, 0, 0, 0, 5, 83, 231, 119, 247, 58, 231, 119, 0, 0, 0, 0, 1, 0, 0, 0, 26, 0, 0, 0}
		goon.Dump(cStringToGoString(c))
	}

	{
		c := []uint8{115, 97, 110, 100, 46, 98, 109, 112, 0, 0, 100, 0, 114, 0, 118, 0, 46, 0, 100, 0, 108, 0, 108, 0, 0, 0, 0, 0, 100, 238, 18, 0}
		goon.Dump(cStringToGoString(c))
	}

	{
		c := []uint8{102, 105, 110, 105, 115, 104, 108, 105, 110, 101, 46, 98, 109, 112, 0, 127, 36, 238, 18, 0, 130, 80, 245, 119, 72, 238, 18, 0, 128, 238, 18, 0}
		goon.Dump(cStringToGoString(c))
	}

	// Output:
	// (string)("dirt.bmp")
	// (string)("sand.bmp")
	// (string)("finishline.bmp")
}
