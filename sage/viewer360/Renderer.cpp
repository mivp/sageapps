#include "Renderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
using namespace std;

namespace viewer360 {

  Renderer::Renderer(bool video, int w, int h): m_initialized(false), m_mesh(0), m_data(0),
                m_textureY(0), m_textureU(0), m_textureV(0),
                m_updatetexture(false), m_video(video),
                m_width(w), m_height(h),
                m_material(0)
  {
  }

  Renderer::~Renderer() {
    if(m_data) delete m_data;
    if(m_textureY) delete m_textureY;
    if(m_textureU) delete m_textureU;
    if(m_textureV) delete m_textureV;
    if(m_mesh) delete m_mesh;
    if(m_material) delete m_material;
  }


  void Renderer::update(const uint8_t* rgb_pixels) {
      if(m_video)
        return;
      m_textureY->updateTexture(rgb_pixels);
  }

  void Renderer::update(const uint8_t* pixels_y, int stride_y, const uint8_t* pixels_u, int stride_u,
  const uint8_t* pixels_v, int stride_v) {

      if(!m_video)
        return;
      m_textureY->updateTexture(pixels_y, stride_y);
      m_textureU->updateTexture(pixels_u, stride_u);
      m_textureV->updateTexture(pixels_v, stride_v);
  }

  void Renderer::setup() {
    if(m_initialized)
      return;

    /*
    int x,y,n;
    m_data = stbi_load(m_filename.c_str(), &x, &y, &n, 0);
    if(!m_data) {
        cout << "Failed to load texture " << m_filename << endl;
        exit(0);
    }
    */

    if(!m_video) {
        m_textureY = new Texture(m_width, m_height, 3, 0);
        m_textureY->initTexture();

        m_material = new ImageMaterial();
    }
    else {
        m_textureY = new Texture(m_width, m_height, 1, 0);
        m_textureY->initTexture();

        m_textureU = new Texture(m_width/2, m_height/2, 1, 1);
        m_textureU->initTexture();

        m_textureV = new Texture(m_width/2, m_height/2, 1, 2);
        m_textureV->initTexture();

        m_material = new VideoMaterial();
    }

    m_mesh = MeshUtils::sphere(500, 80, 50);


    m_initialized = true;
  }

  void Renderer::render(const float MV[16], const float P[16]) {
    setup();

    /*
    if(m_updatetexture) {
        m_texture->updateTexture(m_data);
        m_updatetexture = false;
    }
    */

    m_textureY->bind();
    if(m_video) {
        m_textureU->bind();
        m_textureV->bind();
    }

    m_mesh->render(m_material, MV, P);

    m_textureY->unbind();
    if(m_video) {
        m_textureU->bind();
        m_textureV->bind();
    }
  }

}; //namespace
