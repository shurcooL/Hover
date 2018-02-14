// +build !js

package main

import (
	"go/build"
	"log"
	"os"
)

// Set the working directory to the root of Hover package, so that its assets can be accessed.
func init() {
	// importPathToDir resolves the absolute path from importPath.
	// There doesn't need to be a valid Go package inside that import path,
	// but the directory must exist.
	importPathToDir := func(importPath string) (string, error) {
		p, err := build.Import(importPath, "", build.FindOnly)
		return p.Dir, err
	}

	dir, err := importPathToDir("github.com/shurcooL/Hover")
	if err != nil {
		log.Fatalln("Unable to find github.com/shurcooL/Hover package in your GOPATH, it's needed to load assets:", err)
	}
	err = os.Chdir(dir)
	if err != nil {
		log.Fatalln("os.Chdir:", err)
	}
}
