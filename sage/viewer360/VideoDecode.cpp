#include "VideoDecode.h"
#include "Utils.h"

#include <iostream>
using namespace std;

namespace viewer360 {

    VideoDecode::VideoDecode(video_decoder_callback frameCallback, void* user):
                m_codec(0), m_codecContext(0), m_loaded(false),
                m_fp(0), m_frame(0), m_videoStreamIndex(-1), m_formatContext(0),
                m_timeBetween2Frames(0), m_timePrev(0), m_isFirstFrame(true),
                m_numLoop(1), m_curLoop(0),
                cb_frame(frameCallback), cb_user(user)
    {
        //avcodec_register_all();
        av_register_all();
    }

    VideoDecode::~VideoDecode() {
        clean();
    }

    void VideoDecode::clean() {
        if(m_codecContext) {
            avcodec_close(m_codecContext);
            av_free(m_codecContext);
            m_codecContext = NULL;
        }

        if(m_frame) {
            av_free(m_frame);
            m_frame = NULL;
        }

        if(m_fp) {
            fclose(m_fp);
            m_fp = NULL;
        }

        if(m_formatContext) {
            delete m_formatContext;
            //avformat_free_context(m_formatContext);
            m_formatContext = NULL;
        }
    }

    void VideoDecode::pgmSave(const unsigned char *buf, int wrap, int xsize, int ysize, const char *filename) {
        FILE *f;
        int i;
        f=fopen(filename,"w");
        fprintf(f,"P5\n%d %d\n%d\n",xsize,ysize,255);
        for(i=0;i<ysize;i++)
            fwrite(buf + i * wrap,1,xsize,f);
        fclose(f);
    }

    void VideoDecode::showError(string errStr) {
        cout << "ERROR " << AV_LOG_ERROR << " " << errStr << endl;
        clean();
        exit(-1);
    }

    int VideoDecode::load(string filename) {
        m_filename = filename;
        cout << m_filename << endl;

        // Open file
        //m_formatContext = avformat_alloc_context();
        if( avformat_open_input(&m_formatContext, m_filename.c_str(), NULL, NULL) < 0)
            showError("avformat_open_input failed");

        if( avformat_find_stream_info(m_formatContext, NULL) < 0)
            showError("avformat_find_stream_info failed");

        av_dump_format(m_formatContext, 0, m_filename.c_str(), 0);

        for (unsigned int i = 0; i < m_formatContext->nb_streams; i++)
        {
            if (m_formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                m_videoStreamIndex = i;

                m_codecContext = m_formatContext->streams[i]->codec; //avcodec_alloc_context3(m_codec); //
                if(!m_codecContext)
                     showError("avcodec_alloc_context3 failed");

                // Find decoder
                m_codecId = m_codecContext->codec_id; //m_formatContext->streams[i]->codec->codec_id;
                m_codec = avcodec_find_decoder(m_codecId);
                if(!m_codec)
                    showError("avcodec_find_decoder failed");

                //if(avcodec_parameters_to_context(m_codecContext, m_formatContext->streams[i]->codecpar) < 0)
                //    showError("avcodec_parameters_to_context failed");
                //m_parser = av_parser_init(m_codecId);
                //if(!m_parser)
                //    showError("av_parser_init failed");

                // Open decoder
                if (avcodec_open2(m_codecContext, m_codec, NULL) < 0)
                    showError("avcodec_open2 failed");

                m_width = m_codecContext->coded_width;
                m_height = m_codecContext->coded_height;
                m_fps = av_q2d(m_formatContext->streams[m_videoStreamIndex]->r_frame_rate);
                cout << "width: " << m_width << " height: " << m_height << " fps: " << m_fps << endl;

                //if(m_codec->capabilities & CODEC_CAP_TRUNCATED) {
                //    m_codecContext->flags |= CODEC_FLAG_TRUNCATED;
                //}

                // others
                m_frame = av_frame_alloc();
                memset(m_inbuf + H264_INBUF_SIZE, 0, FF_INPUT_BUFFER_PADDING_SIZE); //AV_INPUT_BUFFER_PADDING_SIZE);
                m_fp = fopen(m_filename.c_str(), "rb");
                if(!m_fp)
                    showError("cannot open file to read");

                m_loaded = true;

                break;
            }
        }

        return 0;
    }

    int VideoDecode::readFrame(bool& hasFrame) {

        if(!m_loaded)
            return -1;

        bool skip = false;
        if(m_isFirstFrame) {
            m_timeBetween2Frames = (1 / m_fps) * 1000;
            m_timePrev = viewer360::Utils::getTime();
            m_isFirstFrame = false;
        }

        if(viewer360::Utils::getTime() - m_timePrev < m_timeBetween2Frames)
            skip = true;

        if (skip) {
            hasFrame = true;
            return 1;
        }

        AVPacket packet;
        char buf[1024];

        hasFrame = false;
        while (av_read_frame(m_formatContext, &packet) >= 0) {

            //int64_t pts = 0;
            //pts = (packet.dts != AV_NOPTS_VALUE) ? packet.dts : 0;

            if (packet.stream_index == m_videoStreamIndex) {
                int frame_finished = 0;
                int videoFrameBytes = avcodec_decode_video2(m_codecContext, m_frame, &frame_finished, &packet);
                if(videoFrameBytes <= 0)
                    showError("avcodec_decode_video2 failed");

                if (frame_finished) {
                    printf("frame %3d\n", m_codecContext->frame_number);
                    fflush(stdout);
                    //snprintf(buf, sizeof(buf), "%s-%d.pgm", "output/frame", m_codecContext->frame_number);
                    //pgmSave(m_frame->data[0], m_frame->linesize[0], m_frame->width, m_frame->height, buf);
                    hasFrame = true;
                    m_timePrev = viewer360::Utils::getTime();
                    cb_frame(m_frame, cb_user);
                    break;
                }
            }

            av_free_packet(&packet);
            packet = AVPacket();
        };

        if(!hasFrame) {
            m_curLoop++;
            if(m_curLoop < m_numLoop) {
                rewind();
                hasFrame = true;
            }
        }

        return 0;
    }

    void VideoDecode::rewind() {
        av_seek_frame( m_formatContext, m_videoStreamIndex, 0, AVSEEK_FLAG_FRAME );
    }

}; //namespace
