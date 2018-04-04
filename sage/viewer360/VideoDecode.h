#ifndef VIDEO_DECODE_H
#define VIDEO_DECODE_H

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#ifdef __cplusplus
} // end extern "C".
#endif // __cplusplus

#include <string>
#include <vector>
using std::string;
using std::vector;

namespace viewer360 {

    #define H264_INBUF_SIZE 4096  //16384

    typedef void(*video_decoder_callback)(const AVFrame* frame, void* user);

    class VideoDecode {

    private:
        string m_filename;
        int m_width, m_height;
        float m_fps;
        int m_frameIndex;
        bool m_loaded;

        unsigned char m_inbuf[H264_INBUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE];
        std::vector<uint8_t> m_buffer;
        AVCodec* m_codec;
        AVCodecContext* m_codecContext;
        AVFormatContext* m_formatContext;
        AVFrame* m_frame;
        AVCodecID m_codecId;
        FILE* m_fp;
        int m_videoStreamIndex;

        //delay
        int m_numLoop, m_curLoop;
        unsigned int m_timeBetween2Frames;
        unsigned int m_timePrev;
        bool m_isFirstFrame;

        //callback
        video_decoder_callback cb_frame;                                                        /* the callback function which will receive the frame/packet data */
        void* cb_user;

    public:
        VideoDecode(video_decoder_callback frameCallback, void* user);
        ~VideoDecode();

        int load(string filename);
        int readFrame(bool& hasFrame);
        void rewind();

        void setNumLoop(int val) { m_numLoop = val; }
        int getWidth() { return m_width; }
        int getHeight() { return m_height; }
        float getFps() { return m_fps; }

    private:
        void clean();
        void showError(string errStr);
        void pgmSave(const unsigned char *buf, int wrap, int xsize, int ysize, const char *filename);
    };

}; //namespace

#endif
