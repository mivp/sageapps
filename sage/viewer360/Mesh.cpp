#include "Mesh.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <float.h>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;
using namespace glm;

namespace viewer360 {

    /**
     Mesh Terrain
     */
    Mesh::Mesh(): m_vbo(0), m_vao(0), m_ibo(0),
    m_initialized(false), m_wireframe(false) {
        vertexes.clear();
        indices.clear();
        m_modelMatrix = glm::mat4(1.0);
    }

    Mesh::~Mesh() {
        if(m_vao > 0) {
            glDeleteVertexArrays(1,&m_vao);
            glDeleteBuffers(1,&m_vbo);
            glDeleteBuffers(1,&m_ibo);
        }
        vertexes.clear();
        indices.clear();
    }

    void Mesh::setup(Material* material){

        if(m_initialized)
            return;

        cout << "Mesh::setup" << endl;

        if(vertexes.size() < 1)
            return;

        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        // create vbo
        glGenBuffers(1,&m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*vertexes.size(), &vertexes[0], GL_STATIC_DRAW);

        GLSLProgram* shader = material->getShader();
        //shader->bind();
        unsigned int val;

        val = glGetAttribLocation(shader->getHandle(), "inPosition");
        glEnableVertexAttribArray(val);
        glVertexAttribPointer( val,  3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

        val = glGetAttribLocation(shader->getHandle(), "inTexCoord");
        glEnableVertexAttribArray(val);
        glVertexAttribPointer( val,  2, GL_FLOAT, GL_TRUE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

        // create ibo
        glGenBuffers(1,&m_ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*indices.size(), &indices[0], GL_STATIC_DRAW);

        glBindVertexArray(0);

        m_initialized = true;
    }

    void Mesh::moveTo(glm::vec3 pos) {
        m_modelMatrix = glm::mat4(1.0);
        m_modelMatrix = glm::translate(m_modelMatrix, pos);
    }

    void Mesh::render(Material* material, const float MV[16], const float P[16]) {

        setup(material);

        glEnable(GL_DEPTH_TEST);

        GLSLProgram* shader = material->getShader();
        shader->bind();

        shader->setUniform("uMV", MV);
        shader->setUniform("uP", P);
        shader->setUniform("uTexY", (int)0);
        shader->setUniform("uTexU", (int)1);
        shader->setUniform("uTexV", (int)2);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);

        glDrawElements( GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, (void*)0 );

        glBindVertexArray(0);
    }

    // SPHERE
#ifndef PI
#define PI 3.14159265359
#endif
    void generateSphereVerts(float * verts, float * norms, float * tex,
                             int * el, float radius, int slices, int stacks)
    {
        // Generate positions and normals
        GLfloat theta, phi;
        GLfloat thetaFac = (2.0 * PI) / slices;
        GLfloat phiFac = PI / stacks;
        GLfloat nx, ny, nz, s, t;
        GLuint idx = 0, tIdx = 0;
        for( int i = 0; i <= slices; i++ ) {
            theta = i * thetaFac;
            s = (GLfloat)i / slices;
            for( int j = 0; j <= stacks; j++ ) {
                phi = j * phiFac;
                t = (GLfloat)j / stacks;
                nx = sinf(phi) * cosf(theta);
                ny = sinf(phi) * sinf(theta);
                nz = cosf(phi);
                verts[idx] = radius * nx; verts[idx+1] = radius * ny; verts[idx+2] = radius * nz;
                norms[idx] = nx; norms[idx+1] = ny; norms[idx+2] = nz;
                idx += 3;

                tex[tIdx] = s;
                tex[tIdx+1] = t;
                tIdx += 2;
            }
        }

        // Generate the element list
        idx = 0;
        for( int i = 0; i < slices; i++ ) {
            GLuint stackStart = i * (stacks + 1);
            GLuint nextStackStart = (i+1) * (stacks+1);
            for( int j = 0; j < stacks; j++ ) {
                if( j == 0 ) {
                    el[idx] = stackStart;
                    el[idx+1] = stackStart + 1;
                    el[idx+2] = nextStackStart + 1;
                    idx += 3;
                } else if( j == stacks - 1) {
                    el[idx] = stackStart + j;
                    el[idx+1] = stackStart + j + 1;
                    el[idx+2] = nextStackStart + j;
                    idx += 3;
                } else {
                    el[idx] = stackStart + j;
                    el[idx+1] = stackStart + j + 1;
                    el[idx+2] = nextStackStart + j + 1;
                    el[idx+3] = nextStackStart + j;
                    el[idx+4] = stackStart + j;
                    el[idx+5] = nextStackStart + j + 1;
                    idx += 6;
                }
            }
        }
    }

    Mesh* MeshUtils::sphere(float radius, int slices, int stacks) {

        Mesh* sphere = new Mesh();

        int nVerts = (slices+1) * (stacks + 1);
        int elements = (slices * 2 * (stacks-1) ) * 3;

        // Verts
        float * v = new float[3 * nVerts];
        // Normals
        float * n = new float[3 * nVerts];
        // Tex coords
        float * tex = new float[2 * nVerts];
        // Elements
        int * el = new int[elements];

        // Generate the vertex data
        generateSphereVerts(v, n, tex, el, radius, slices, stacks);

        for(int i=0; i < nVerts; i++) {
            glm::vec3 position = glm::vec3(v[3*i], v[3*i+1], v[3*i+2]);
            glm::vec3 normal = glm::vec3(n[3*i], n[3*i+1], n[3*i+2]);
            glm::vec2 uv = glm::vec2(tex[2*i], tex[2*i+1]);
            sphere->vertexes.push_back(Vertex(position, normal, uv));
        }

        for(int i=0; i < elements; i++)
            sphere->indices.push_back(el[i]);

        return sphere;
    }

}; //namespace viewer360
