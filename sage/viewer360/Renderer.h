#ifndef RENDERER_H__
#define RENDERER_H__

#include "Texture.h"
#include "Mesh.h"

#include <string>
using std::string;

namespace viewer360 {

  class Renderer {

  private:
    bool m_initialized;
    Material* m_material;
    Mesh* m_mesh;
    unsigned char* m_data;
    Texture *m_textureY, *m_textureU, *m_textureV;
    bool m_updatetexture;
    bool m_video;
    int m_width, m_height;

  private:
    void setup();

  public:
    Renderer(bool video, int w, int h);
    ~Renderer();

    void update(const uint8_t* rgb_pixels);
    void update(const uint8_t* pixels_y, int stride_y, const uint8_t* pixels_u, int stride_u,
                const uint8_t* pixels_v, int stride_v);
    void render(const float MV[16], const float P[16]);

  };

}; // namespace

#endif
