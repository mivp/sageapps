#include "Material.h"
#include <iostream>

namespace viewer360 {

    Material::Material(): m_shader(0) {
        m_shaderDir = SHADER_DIR;
        std::cout << "Shader dir: " << SHADER_DIR << std::endl;

        color = glm::vec3(1.0);
        Ka = glm::vec3( 0.1f, 0.1f, 0.1f );
        Kd = glm::vec3( 1.0f, 1.0f, 1.0f );
        Ks = glm::vec3( 0.3f, 0.3f, 0.3f );
        shininess = 1.0;
    }

    Material::~Material() {
        if(m_shader)
            delete m_shader;
    }

    /**
     */
    ImageMaterial::ImageMaterial(): Material() {
        m_shader = new GLSLProgram();
        string filename;
        filename = m_shaderDir; filename.append("image.vert");
        m_shader->compileShader(filename.c_str());
        filename = m_shaderDir; filename.append("image.frag");
        m_shader->compileShader(filename.c_str());
        m_shader->link();
    }

    ImageMaterial::~ImageMaterial() {

    }

    /**
     */
    VideoMaterial::VideoMaterial(): Material() {
        m_shader = new GLSLProgram();
        string filename;
        filename = m_shaderDir; filename.append("video.vert");
        m_shader->compileShader(filename.c_str());
        filename = m_shaderDir; filename.append("video.frag");
        m_shader->compileShader(filename.c_str());
        m_shader->link();
    }

    VideoMaterial::~VideoMaterial() {

    }

    /**
     */
    SSMaterial::SSMaterial(): Material() {
        m_shader = new GLSLProgram();
        string filename;
        filename = m_shaderDir; filename.append("ss_display.vert");
        m_shader->compileShader(filename.c_str());
        filename = m_shaderDir; filename.append("ss_display.frag");
        m_shader->compileShader(filename.c_str());
        m_shader->link();
    }

    SSMaterial::~SSMaterial() {

    }

}; //namespace viewer360
