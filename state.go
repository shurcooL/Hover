package main

import (
	"encoding/gob"
	"os"

	"github.com/goxjs/glfw"
)

func saveState(path string) error {
	f, err := os.OpenFile(path, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, 0644)
	if err != nil {
		return err
	}
	defer f.Close()

	enc := gob.NewEncoder(f)

	err = enc.Encode(cameraIndex)
	if err != nil {
		return err
	}
	err = enc.Encode(camera)
	if err != nil {
		return err
	}
	err = enc.Encode(player)
	if err != nil {
		return err
	}
	err = enc.Encode(wireframe)
	if err != nil {
		return err
	}

	return nil
}

func loadState(path string) error {
	f, err := glfw.Open(path)
	if err != nil {
		return err
	}
	defer f.Close()

	dec := gob.NewDecoder(f)

	err = dec.Decode(&cameraIndex)
	if err != nil {
		return err
	}
	err = dec.Decode(&camera)
	if err != nil {
		return err
	}
	err = dec.Decode(&player)
	if err != nil {
		return err
	}
	err = dec.Decode(&wireframe)
	if err != nil {
		return err
	}

	return nil
}
