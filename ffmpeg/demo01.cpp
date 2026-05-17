#include <stdio.h>
extern "C" {
#include <libavformat/avformat.h>
}

int main()
{
    printf("FFmpeg version: %s\n", av_version_info());
    return 0;
}
