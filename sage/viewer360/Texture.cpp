#include "Texture.h"
#include "Utils.h"
#include <iostream>
#include <stdlib.h>

using namespace std;

namespace viewer360 {

    const unsigned int Texture::LINEAR = GL_LINEAR;
    const unsigned int Texture::NEAREST = GL_NEAREST;
    const unsigned int Texture::MIPMAP = GL_LINEAR_MIPMAP_LINEAR;

    void Texture::initTexture() {

        if(!created) {
            //init texture
            glunit = unitFromIndex(index);
            minFilter = GL_LINEAR;
            magFilter = GL_LINEAR;
            type = GL_UNSIGNED_BYTE;
            if(numChannel == 1) {
                format = GL_R8;
                globalFormat = GL_RED;
            }
            else if (numChannel == 3) {
                format = globalFormat = GL_RGB;
            }
            else {
                format = GL_RGBA8;
                globalFormat = GL_RGBA;
                type = GL_UNSIGNED_INT_8_8_8_8_REV;
            }

            glActiveTexture(glunit);
            glGenTextures(1, &gluid);
            glBindTexture(GL_TEXTURE_2D, gluid);

            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, globalFormat, type, NULL);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); //GL_CLAMP_TO_BORDER GL_REPEAT
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
            glBindTexture(GL_TEXTURE_2D, 0);

            created = true;
        }
    }

    void Texture::updateTexture(const unsigned char* data, int stride) {
        if(!data)
            return;

        glActiveTexture(glunit);
        glBindTexture(GL_TEXTURE_2D, gluid);

        if(stride > 0)
            glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, globalFormat, type, data);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // for terrain
    Texture::Texture(int width, int height, int numchannels, unsigned int ind): created(false) {

        this->numChannel = numchannels;
        this->width = width;
        this->height = height;
        this->gluid = 0;
        this->index = ind;
    }

    void Texture::freeTexture()  {
        if(gluid)
            glDeleteTextures(1, &gluid);
        created = false;
        gluid = 0;
    }

    Texture::~Texture() {
        //glActiveTexture(glunit);
        if(gluid)
            glDeleteTextures(1, &gluid);
    }

    void Texture::bind() {
        if(!gluid)
            return;
        glActiveTexture(glunit);
        glBindTexture(GL_TEXTURE_2D, gluid);
    }

    void Texture::unbind() {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    int Texture::getWidth() {
        return width;
    }

    int Texture::getHeight() {
        return height;
    }


    // static
    unsigned int Texture::unitCount = 0;
    float Texture::borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float Texture::borderColorB[] = {0.0f, 0.0f, 0.0f, 0.0f};

    unsigned int Texture::unitFromIndex(unsigned int index)
    {
        switch(index)
        {
            case 1: return GL_TEXTURE1;
            case 2: return GL_TEXTURE2;
            case 3: return GL_TEXTURE3;
            case 4: return GL_TEXTURE4;
            case 5: return GL_TEXTURE5;
            case 6: return GL_TEXTURE6;
            case 7: return GL_TEXTURE7;
            case 8: return GL_TEXTURE8;
            case 9: return GL_TEXTURE9;
            default: return GL_TEXTURE0;
        }
    }

}; //namespace viewer360
