#include "VideoDecode.h"

#include <iostream>
#include <string>
using namespace std;
using namespace video360;

#define FILE_NAME          "testdata/testvideo.mp4"
#define OUTPUT_FILE_PREFIX "output/image%d.bmp"
#define FRAME_COUNT        10

void frameCallback(const AVFrame* frame, void* user) {
    cout << "get a frame width " << frame->width << " height: " << frame->height << endl;
}

int main(int argc, char* argv[]) {

    cout << "test video decoding" << endl;

    VideoDecode* video = new VideoDecode(frameCallback, NULL);
    video->load("testdata/testvideo.mp4");

    bool hasFrame = true;
    while(hasFrame) {
        video->readFrame(hasFrame);
    }

    //for(int i=0; i < 3; i++)
    //    video->readFrame(hasFrame);

    if(video)
        delete video;

    cout << "done" << endl;

    return 0;
}
