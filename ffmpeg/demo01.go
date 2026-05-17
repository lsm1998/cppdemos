package main

/*
#cgo pkg-config: libavformat libavcodec libavutil

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <stdio.h>
*/
import "C"
import "fmt"

func main() {
	version := C.GoString(C.av_version_info())
	fmt.Printf("FFmpeg version: %s\n", version)
}
