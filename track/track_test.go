package track

import (
	"bytes"
	"io/ioutil"
	"path/filepath"
	"testing"
)

func TestCStringToGoString(t *testing.T) {
	tests := []struct {
		in   []uint8
		want string
	}{
		{
			in:   []uint8{100, 105, 114, 116, 46, 98, 109, 112, 0, 0, 0, 0, 5, 83, 231, 119, 247, 58, 231, 119, 0, 0, 0, 0, 1, 0, 0, 0, 26, 0, 0, 0},
			want: "dirt.bmp",
		},
		{
			in:   []uint8{115, 97, 110, 100, 46, 98, 109, 112, 0, 0, 100, 0, 114, 0, 118, 0, 46, 0, 100, 0, 108, 0, 108, 0, 0, 0, 0, 0, 100, 238, 18, 0},
			want: "sand.bmp",
		},
		{
			in:   []uint8{102, 105, 110, 105, 115, 104, 108, 105, 110, 101, 46, 98, 109, 112, 0, 127, 36, 238, 18, 0, 130, 80, 245, 119, 72, 238, 18, 0, 128, 238, 18, 0},
			want: "finishline.bmp",
		},
	}

	for _, test := range tests {
		if got := cStringToGoString(test.in); got != test.want {
			t.Errorf("\ngot %q\nwant %q", got, test.want)
		}
	}
}

func BenchmarkLoad(b *testing.B) {
	track1Bytes, err := ioutil.ReadFile(filepath.Join("..", "track1.dat"))
	if err != nil {
		b.Fatal(err)
	}

	b.ResetTimer()

	for i := 0; i < b.N; i++ {
		r := bytes.NewReader(track1Bytes)
		_, err := Load(r)
		if err != nil {
			b.Fatal(err)
		}
	}
}
