#include "batch.hpp"
#include <iostream>
#include <algorithm>

namespace ORCore
{

    Batch::Batch(ShaderProgram *program, Texture *texture, int batchSize, int id)
    : m_program(program), m_texture(texture), m_batchSize(batchSize), m_id(id), m_committed(false),
    m_matTexBuffer(GL_RGBA32F)
    {
        m_vertices.reserve(batchSize);
        m_matrices.reserve(batchSize);
        m_meshMatrixIndex.reserve(batchSize);
        init_gl();
    }

    void Batch::init_gl()
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        m_vertLoc = m_program->vertex_attribute("position");
        m_uvLoc = m_program->vertex_attribute("vertexUV");
        m_colorLoc = m_program->vertex_attribute("color");
        m_matIndexLoc = m_program->vertex_attribute("matrixIndex");
        m_texSampID = m_program->uniform_attribute("textureSampler");
        m_matBufTexID = m_program->uniform_attribute("matrixBuffer");


        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);
        
        // Vertex Buffer
        glGenBuffers(1, &m_vbo);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glEnableVertexAttribArray(m_vertLoc);
        glEnableVertexAttribArray(m_uvLoc);
        glEnableVertexAttribArray(m_colorLoc);
        glEnableVertexAttribArray(m_matIndexLoc);

        // Setup VAO attributes for this batch. Once these are set the vbo can be replaced or allocated and these will still be valid.
        glVertexAttribPointer(m_vertLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, vertex)));
        glVertexAttribPointer(m_uvLoc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, uv)));
        glVertexAttribPointer(m_colorLoc, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, color)));
        glVertexAttribIPointer(m_matIndexLoc, 1, GL_INT, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, matIndex)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Matrix Texture Buffer Objects
        glGenBuffers(1, &m_matBufferObject);
        glBindBuffer(GL_TEXTURE_BUFFER, m_matBufferObject);

        glBindBuffer(GL_TEXTURE_BUFFER, 0);

        glBindVertexArray(0);

    }

    void Batch::clear()
    {
        m_committed = false;
        m_matrices.clear();
        m_vertices.clear();
    }

    bool Batch::add_mesh(Mesh& mesh, const glm::mat4& transform)
    {
        // Optimize this using glMapBuffer? constrain batch with m_batchSize return false if mesh doesnt fit.
        int meshVertexCount = mesh.vertices.size();
        if ((m_vertices.size() + meshVertexCount) <= m_batchSize)
        {
            // Add one index per vertex
            for (auto &vertex : mesh.vertices)
            {
                vertex.matIndex = m_matrices.size();
            }

            mesh.transformOffset = m_matrices.size();
            mesh.verticesOffset = m_vertices.size();

            // Condensing the translation will be moved to the object side of things to make rebuilding geometry less cpu intensive.
            m_matrices.push_back(transform);
            m_vertices.insert(std::end(m_vertices), std::begin(mesh.vertices), std::end(mesh.vertices));

            return true;
        } else {
            return false;
        }
    }

    void Batch::update_mesh(Mesh& mesh, const glm::mat4& transform)
    {
        // TODO - Make this smarter about what it updates if performance becomes an issue.
        // Could glBufferSubdata just the parts needed?
        m_committed = false;
        m_matrices[mesh.transformOffset] = transform;
        for (auto &vertex : mesh.vertices)
        {
            vertex.matIndex = mesh.transformOffset;
        }
        std::copy(std::begin(mesh.vertices), std::end(mesh.vertices), std::begin(m_vertices)+mesh.verticesOffset);
    }

    void Batch::set_state(RenderState state)
    {
        m_state = state;
    }

    // update buffer objects
    void Batch::commit()
    {
        m_committed = true;

        //std::cout << m_vertices.size() << std::endl;

        // When switching to glMapBuffer
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, m_vertices.size()*sizeof(Vertex), &m_vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_TEXTURE_BUFFER, m_matBufferObject);
        glBufferData(GL_TEXTURE_BUFFER, m_matrices.size()*sizeof(glm::mat4), &m_matrices[0], GL_STATIC_DRAW);
        m_matTexBuffer.assign_buffer(m_matBufferObject);

        glBindBuffer(GL_TEXTURE_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

    }

    void Batch::render()
    {
        if (m_matrices.size() > 0) {

            glBindVertexArray(m_vao);

            // Bind textures
            m_texture->bind(m_texSampID);
            m_matTexBuffer.bind(m_matBufTexID);

            glDrawArrays(GL_TRIANGLES, 0, m_vertices.size());

        }

    }

    Batch::~Batch()
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDisableVertexAttribArray(m_colorLoc);
        glDisableVertexAttribArray(m_uvLoc);
        glDisableVertexAttribArray(m_vertLoc);
        glDisableVertexAttribArray(m_matIndexLoc);

        glDeleteBuffers(1, &m_vbo); 
        glDeleteBuffers(1, &m_matBufferObject);

        glDeleteVertexArrays(1, &m_vao);
    }

} // namespace ORCore
