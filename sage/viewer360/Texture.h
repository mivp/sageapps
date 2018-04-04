#ifndef TEXTURE_H
#define TEXTURE_H

#include <string>

#ifdef OMEGALIB_MODULE
#include <omegaGl.h>
#else
#include "GLInclude.h"
#endif

using namespace std;

namespace viewer360 {

    class Texture
    {
    public:
        Texture(int width, int height, int numchannels, unsigned int ind = 0);
        ~Texture();

        void initTexture();
        void updateTexture(const unsigned char* data, int stride = 0);
        void freeTexture();

        void bind();
        void unbind();
        int getWidth();
        int getHeight();

        static unsigned int unitFromIndex(unsigned int _index);

        // Needs to be public to be accessed by GL calls
        unsigned int gluid;
        unsigned int glunit;
        unsigned int index;

        static const unsigned int LINEAR;
        static const unsigned int NEAREST;
        static const unsigned int MIPMAP;

        string filename;

    private:
        static unsigned int unitCount;
        static float borderColor[];
        static float borderColorB[];

        unsigned int height;
        unsigned int width;
        unsigned int height_low, width_low;
        unsigned int minFilter;
        unsigned int magFilter;
        unsigned int format;
        unsigned int globalFormat;
        unsigned int type;

        bool created;
        int numChannel;
    };

}; //namespace viewer360

#endif // TEXTURE_H
