#pragma once

#include <regex>
#include "debugging.h"
#include "IOutil.h"
#include <glm/ext.hpp>

class Shader {
public:
    GLuint VertexShader, gs, FragmentShader, Program;

    std::map<std::string, int> ShaderUniformVariable;

    Shader() {}

    Shader(const std::string *nameV, const std::string *nameF)
    {
        create_program(nameV->c_str(), nameF->c_str());
    }

    Shader(const GLchar *nameV, const GLchar *nameF)
    {
        create_program(nameV, nameF);
    }

    static Shader *CreateShaderFromFile(const std::string &shaderPath, const std::string &vertexShader,
                                        const std::string &fragmentShader)
    {
        return CreateShaderFromFile(shaderPath + vertexShader, shaderPath + fragmentShader);
    }

    static Shader *CreateShaderFromFile(const std::string &vertexShader, const std::string &fragmentShader)
    {
        return CreateShaderFromFile(vertexShader.c_str(), fragmentShader.c_str());
    }

    static Shader *CreateShaderFromFile(const char *vertexShader, const char *fragmentShader)
    {
        auto *shader = new Shader(vertexShader, fragmentShader);

        GlDebug_CheckError()
        return shader;
    }

    void create_program(const std::string &nameV, const std::string &nameF)
    {
        create_program(nameV.c_str(), nameF.c_str());
    }

    void create_program(const GLchar *nameV, const GLchar *nameF)
    {
        std::string vs_src_code = textFileRead(nameV);
        std::string fs_src_code = textFileRead(nameF);

        create_shader(vs_src_code.c_str(), GL_VERTEX_SHADER);
        create_shader(fs_src_code.c_str(), GL_FRAGMENT_SHADER);

        Program = glCreateProgram();
        glAttachShader(Program, VertexShader);
        glAttachShader(Program, FragmentShader);

        glLinkProgram(Program);

        bind_uniform_variables(vs_src_code);
        bind_uniform_variables(fs_src_code);

        check_shader(VertexShader);
        check_shader(FragmentShader);
        validate_shader_program(Program);
    }


    void LoadProgram() const
    {
        glUseProgram(Program);
    }


    static void UnloadProgram()
    {
        glUseProgram(0);
    }

    Shader RegisterUniformVariable(const std::string &name)
    {
        ShaderUniformVariable[name] = glGetUniformLocation(Program, name.c_str());
        return *this;
    }

    int operator[](const std::string &name)
    {
        return ShaderUniformVariable[name];
    }


#define CHECK_UNIFORM(uniformName) if (ShaderUniformVariable.find(uniformName) == ShaderUniformVariable.end())\
    {\
        return;\
    }

    void SetUniformMat4f(const char *uniformName, glm::mat4 matrix)
    {
        CHECK_UNIFORM(uniformName);
        glUniformMatrix4fv((*this)[uniformName], 1, GL_FALSE, &matrix[0][0]);
        CheckGLErrors(__LINE__, __FILE__);
    }


    void SetUniformVec3f(const char *uniformName, float a, float b, float c)
    {
        CHECK_UNIFORM(uniformName);
        glUniform3f((*this)[uniformName], a, b, c);
        CheckGLErrors(__LINE__, __FILE__);
    }

    void SetUniformVec3f(const char *uniformName, float vec[3])
    {
        CHECK_UNIFORM(uniformName);
        glUniform3fv((*this)[uniformName], 1, &vec[0]);
        CheckGLErrors(__LINE__, __FILE__);
    }

    void SetUniformVec3f(const char *uniformName, glm::vec3 vec)
    {
        CHECK_UNIFORM(uniformName);
        glUniform3fv((*this)[uniformName], 1, &vec[0]);
        CheckGLErrors(__LINE__, __FILE__);
    }

    void SetUniformVec4f(const char *uniformName, float a, float b, float c, float d)
    {
        CHECK_UNIFORM(uniformName);
        glUniform4f((*this)[uniformName], a, b, c, d);
        CheckGLErrors(__LINE__, __FILE__);
    }

    void SetUniformVec4f(const char *uniformName, float vec[4])
    {
        CHECK_UNIFORM(uniformName);
        glUniform4fv((*this)[uniformName], 1, &vec[0]);
        CheckGLErrors(__LINE__, __FILE__);
    }

    void SetUniformVec4f(const char *uniformName, glm::vec4 vec)
    {
        CHECK_UNIFORM(uniformName);
        glUniform4fv((*this)[uniformName], 1, &vec[0]);
        CheckGLErrors(__LINE__, __FILE__);
    }

    void SetUniform1f(const char *uniformName, float value)
    {
        CHECK_UNIFORM(uniformName);
        glUniform1f((*this)[uniformName], value);
        CheckGLErrors(__LINE__, __FILE__);
    }

    void SetUniform1i(const char *uniformName, int value)
    {
        CHECK_UNIFORM(uniformName);
        glUniform1i((*this)[uniformName], value);
        CheckGLErrors(__LINE__, __FILE__);
    }

    void SetUniform2i(const char *uniformName, int a, int b)
    {
        CHECK_UNIFORM(uniformName);
        glUniform2i((*this)[uniformName], a, b);
        CheckGLErrors(__LINE__, __FILE__);
    }

private
:
    bool create_shader(const GLchar *src, unsigned int SHADER_TYPE)
    {
        GLuint s;
        switch (SHADER_TYPE)
        {
            case GL_VERTEX_SHADER:
                s = VertexShader = glCreateShader(GL_VERTEX_SHADER);
                break;
            case GL_FRAGMENT_SHADER:
                s = FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
                break;
            default:
                break;
        }

        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        int status;
        glGetShaderiv(s, GL_COMPILE_STATUS, &status);
        if (status != GL_TRUE)
        {
            check_shader(s);
            return false;
        }
        return true;
    }

    void bind_uniform_variables(std::string code)
    {
        code.erase(std::remove(code.begin(), code.end(), '\n'), code.end());
        code.erase(std::remove(code.begin(), code.end(), '\t'), code.end());
        code.erase(std::remove(code.begin(), code.end(), '\b'), code.end());

        int pos;
        std::istringstream check1(code);

        std::string intermediate;
        std::vector<std::string> tokens;
        // Tokenizing w.r.t. space ' '
        while (getline(check1, intermediate, ';'))
        {
            std::string _ = std::regex_replace(intermediate, std::regex("  "), " ");

            if (intermediate.find(' ') == 0)
                intermediate.erase(0, 1);

            if (intermediate.find("uniform") == 0)
            {
                pos = intermediate.find_last_of(' ');
                std::string uniform_name = intermediate.substr(pos + 1, intermediate.length() - pos);
                this->RegisterUniformVariable(uniform_name);
                tokens.push_back(intermediate.substr(pos + 1, intermediate.length() - pos));
            }
        }
    }
};
