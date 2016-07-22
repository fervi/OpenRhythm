#include "vfs.hpp"
#include <glad/glad.h>
#include "shader.hpp"
#include <iostream>
#include <stdexcept>

namespace MgCore
{
    Shader::Shader(ShaderInfo _info): info(_info)
    {
        shader = glCreateShader(info.type);
        std::string data {read_file(info.path)};
        const char *c_str = data.c_str();

        glShaderSource(shader, 1, &c_str, nullptr);
        glCompileShader(shader);


    }
    Shader::~Shader()
    {
        glDeleteShader(shader);
    }

    void Shader::check_error()
    {
        GLint status;

        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        // TODO - probably need to make a custom exception class for this..
        if (status != GL_TRUE) {
            GLint length;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

            auto logData = std::make_unique<GLchar[]>(length+1);
            logData[length] = '\0'; // Make sure it is null terminated.

            glGetShaderInfoLog(shader, length, nullptr, logData.get());
            std::cout << logData.get() << std::endl;

            throw std::runtime_error("Shader compilation failed.");
        } else {
            std::cout << "Shader compiled sucessfully." << std::endl;
        }
    }


    ShaderProgram::ShaderProgram(Shader* vertex, Shader* fragment)
    : m_vertex(*vertex), m_fragment(*fragment)
    {
        m_program = glCreateProgram();

        glAttachShader(m_program, m_vertex.shader);
        glAttachShader(m_program, m_fragment.shader);

        glLinkProgram(m_program);
    }

    ShaderProgram::~ShaderProgram()
    {
        glDeleteProgram(m_program);
    }

    void ShaderProgram::check_error()
    {
        // We want to check the compile/link status of the shaders all at once.
        // At some place that isn't right after the shader compilation step.
        // That way the drivers can better parallelize shader compilation.
        m_vertex.check_error();
        m_fragment.check_error();

        GLint status;

        glGetProgramiv(m_program, GL_LINK_STATUS, &status);
        // TODO - probably need to make a custom exception class for this..
        if (status != GL_TRUE) {
            GLint length;
            glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &length);

            auto logData = std::make_unique<GLchar[]>(length+1);
            logData[length] = '\0'; // Make sure it is null terminated.

            glGetProgramInfoLog(m_program, length, nullptr, logData.get());
            std::cout << logData.get() << std::endl;

            throw std::runtime_error("Shader linkage failed.");
        } else {
            std::cout << "Shader linked sucessfully." << std::endl;
        }
    }


    void ShaderProgram::use()
    {
        glUseProgram(m_program);
    }

    void ShaderProgram::disuse()
    {
        glUseProgram(0);
    }


    int ShaderProgram::vertex_attribute(std::string name)
    {
        return glGetAttribLocation(m_program, name.c_str());
    }

    int ShaderProgram::uniform_attribute(std::string name)
    {
        return glGetUniformLocation(m_program, name.c_str());
    }


    void ShaderProgram::set_uniform(int uniform, int value)
    {
        glUniform1i(uniform, value);
    }

    void ShaderProgram::set_uniform(int uniform, float value)
    {
        glUniform1f(uniform, value);
    }


    void ShaderProgram::set_uniform(int uniform, const glm::vec2& value)
    {
        glUniform2fv(uniform, 1, &value[0]);
    }

    void ShaderProgram::set_uniform(int uniform, const glm::vec3& value)
    {
        glUniform3fv(uniform, 1, &value[0]);
    }

    void ShaderProgram::set_uniform(int uniform, const glm::vec4& value)
    {
        glUniform4fv(uniform, 1, &value[0]);
    }


    void ShaderProgram::set_uniform(int uniform, const glm::mat2& value)
    {
        glUniformMatrix2fv(uniform, 1, GL_FALSE, &value[0][0]);
    }

    void ShaderProgram::set_uniform(int uniform, const glm::mat3& value)
    {
        glUniformMatrix3fv(uniform, 1, GL_FALSE, &value[0][0]);
    }

    void ShaderProgram::set_uniform(int uniform, const glm::mat4& value)
    {
        glUniformMatrix4fv(uniform, 1, GL_FALSE, &value[0][0]);
    }


    void ShaderProgram::set_uniform(int uniform, const std::array<float, 2>& value)
    {
        glUniform2fv(uniform, 1, &value[0]);
    }

    void ShaderProgram::set_uniform(int uniform, const std::array<float, 3>& value)
    {
        glUniform3fv(uniform, 1, &value[0]);
    }

    void ShaderProgram::set_uniform(int uniform, const std::array<float, 4>& value)
    {
        glUniform4fv(uniform, 1, &value[0]);
    }


    void ShaderProgram::set_uniform(int uniform, const std::array<int, 2>& value)
    {
        glUniform2iv(uniform, 1, &value[0]);
    }

    void ShaderProgram::set_uniform(int uniform, const std::array<int, 3>& value)
    {
        glUniform3iv(uniform, 1, &value[0]);
    }

    void ShaderProgram::set_uniform(int uniform, const std::array<int, 4>& value)
    {
        glUniform4iv(uniform, 1, &value[0]);
    }



}
