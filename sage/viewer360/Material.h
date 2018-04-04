#ifndef MATERIAL_H__
#define MATERIAL_H__

#include <glm/gtx/transform.hpp>
#include "Program.h"

namespace viewer360 {

    class Material {

    protected:
        GLSLProgram* m_shader;
        string m_shaderDir;

    public:
        glm::vec3 color;
        glm::vec3 Ka;            // Ambient reflectivity
        glm::vec3 Kd;            // Diffuse reflectivity
        glm::vec3 Ks;            // Specular reflectivity
        float shininess;         // Specular shininess exponent

    public:
        Material();
        ~Material();

        GLSLProgram* getShader() { return m_shader; }
    };

    /**
     */
    class ImageMaterial: public Material {

    protected:

    public:
        ImageMaterial();
        ~ImageMaterial();
    };

    /**
     */
    class VideoMaterial: public Material {

    protected:

    public:
        VideoMaterial();
        ~VideoMaterial();
    };

    /**
     */
    class SSMaterial: public Material {

    protected:

    public:
        SSMaterial();
        ~SSMaterial();
    };

}; //namespace viewer360

#endif
