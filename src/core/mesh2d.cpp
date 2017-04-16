#include "mesh2d.hpp"
#include <iostream>

namespace ORCore
{
    std::vector<Vertex> create_rect_mesh(glm::vec4 color)
    {
        return {
            // Vertex2     UV            Color
            {{0.0f, 0.0f, 0.5f}, {0.0f, 0.0f}, color},
            {{0.0f, 1.0f, 0.5f}, {0.0f, 1.0f}, color},
            {{1.0f, 0.0f, 0.5f}, {1.0f, 0.0f}, color},
            {{0.0f, 1.0f, 0.5f}, {0.0f, 1.0f}, color},
            {{1.0f, 1.0f, 0.5f}, {1.0f, 1.0f}, color},
            {{1.0f, 0.0f, 0.5f}, {1.0f, 0.0f}, color}
        };
    }


    Render::Render(ShaderProgram *program, Texture *texture)
    : m_program(program), m_texture(texture), m_matTexBuffer(GL_RGBA32F), m_matTexIndexBuffer(GL_R32UI)
    {
        m_vertices.reserve(32*6); // 32 object each object has 3 verts of 2 values
        m_matrices.reserve(32);
        m_meshMatrixIndex.reserve(32*2); // 32 objects each object has 2 triangles
        init_gl();
    }

    void Render::init_gl()
    {
        glEnable(GL_BLEND);
        m_vertLoc = m_program->vertex_attribute("position");
        m_uvLoc = m_program->vertex_attribute("vertexUV");
        m_colorLoc = m_program->vertex_attribute("color");
        m_texSampID = m_program->uniform_attribute("textureSampler");
        m_matBufTexID = m_program->uniform_attribute("matrixBuffer");
        m_matIndexBufTexID = m_program->uniform_attribute("matrixIndices");
        
        // Vertex Buffer
        glGenBuffers(1, &m_vbo);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glEnableVertexAttribArray(m_vertLoc);
        glEnableVertexAttribArray(m_uvLoc);
        glEnableVertexAttribArray(m_colorLoc);

        glVertexAttribPointer( m_vertLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, vertex)));
        glVertexAttribPointer( m_uvLoc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, uv)));
        glVertexAttribPointer( m_colorLoc, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, color)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Matrix Texture Buffer Objects
        glGenBuffers(1, &m_matBufferObject);
        glBindBuffer(GL_TEXTURE_BUFFER, m_matBufferObject);

        glGenBuffers(1, &m_matIndexBufferObject);
        glBindBuffer(GL_TEXTURE_BUFFER, m_matIndexBufferObject);

        glBindBuffer(GL_TEXTURE_BUFFER, 0);

    }


    void Render::mesh_clear()
    {
        m_meshMatrixIndex.clear();
        m_matrices.clear();
        m_vertices.clear();
    }

    void Render::add_mesh(const Mesh& mesh)
    {
        int meshVertexCount = mesh.vertices.size();

        // Add one index per vertex
        for (int i = 0; i < meshVertexCount/3; i++) // 3 is for 3 vertices in a triangle
        {
            m_meshMatrixIndex.push_back(m_matrices.size());
        }
        m_matrices.push_back(glm::scale(glm::translate(glm::mat4(1.0f), mesh.translate), mesh.scale));
        m_vertices.insert(m_vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
}

    // update buffer objects
    void Render::mesh_commit()
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, m_vertices.size()*sizeof(Vertex), &m_vertices[0], GL_DYNAMIC_DRAW);

        glBindBuffer(GL_TEXTURE_BUFFER, m_matBufferObject);
        glBufferData(GL_TEXTURE_BUFFER, m_matrices.size()*sizeof(glm::mat4), &m_matrices[0], GL_DYNAMIC_DRAW);
        m_matTexBuffer.assign_buffer(m_matBufferObject);

        glBindBuffer(GL_TEXTURE_BUFFER, m_matIndexBufferObject);
        glBufferData(GL_TEXTURE_BUFFER, m_meshMatrixIndex.size()*sizeof(unsigned int), &m_meshMatrixIndex[0], GL_DYNAMIC_DRAW);
        m_matTexIndexBuffer.assign_buffer(m_matIndexBufferObject);

        glBindBuffer(GL_TEXTURE_BUFFER, 0);

    }

    void Render::render()
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        // Bind textures
        m_texture->bind(m_texSampID);
        m_matTexBuffer.bind(m_matBufTexID);
        m_matTexIndexBuffer.bind(m_matIndexBufTexID);

        glDrawArrays(GL_TRIANGLES, 0, m_vertices.size());

    }

    Render::~Render()
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDisableVertexAttribArray(m_colorLoc);
        glDisableVertexAttribArray(m_uvLoc);
        glDisableVertexAttribArray(m_vertLoc);

        glDeleteBuffers(1, &m_vbo);
    }

} // namespace ORCore
