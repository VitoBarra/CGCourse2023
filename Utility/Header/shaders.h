#pragma once

#include <GL/glew.h>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <iostream>
#include <map>
#include <fstream>
#include <regex>
#include "debugging.h"
#include "IOutil.h"

class Shader {
public:
    GLuint VertexShader, gs, FragmentShader, Program;

    std::map<std::string, int> ShaderUniformVariable;

    Shader() {    }
    Shader(const GLchar *nameV, const char *nameF) {
        create_program(nameV, nameF);
    }

    void create_program(const GLchar *nameV, const char *nameF) {

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


    void RegisterUniformVariable(std::string name) {
        ShaderUniformVariable[name] = glGetUniformLocation(Program, name.c_str());
    }

    int operator[](std::string name) {
        return ShaderUniformVariable[name];
    }

private:
    bool create_shader(const GLchar *src, unsigned int SHADER_TYPE) {
        GLuint s;
        switch (SHADER_TYPE) {
            case GL_VERTEX_SHADER:
                s = VertexShader = glCreateShader(GL_VERTEX_SHADER);
                break;
            case GL_FRAGMENT_SHADER:
                s = FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
                break;
        }

        glShaderSource(s, 1, &src, NULL);
        glCompileShader(s);
        int status;
        glGetShaderiv(s, GL_COMPILE_STATUS, &status);
        if (status != GL_TRUE) {
            check_shader(s);
            return false;
        }
        return true;
    }

    void bind_uniform_variables(std::string code) {

        code.erase(std::remove(code.begin(), code.end(), '\n'), code.end());
        code.erase(std::remove(code.begin(), code.end(), '\t'), code.end());
        code.erase(std::remove(code.begin(), code.end(), '\b'), code.end());

        int pos;
        std::istringstream check1(code);

        std::string intermediate;
        std::vector<std::string> tokens;
        // Tokenizing w.r.t. space ' '
        while (getline(check1, intermediate, ';')) {
            std::string _ = std::regex_replace(intermediate, std::regex("  "), " ");

            if (intermediate.find(" ") == 0)
                intermediate.erase(0, 1);

            if (intermediate.find("uniform") == 0) {
                pos = intermediate.find_last_of(" ");
                std::string uniform_name = intermediate.substr(pos + 1, intermediate.length() - pos);
                this->RegisterUniformVariable(uniform_name);
                tokens.push_back(intermediate.substr(pos + 1, intermediate.length() - pos));
            }
        }
    }


};

 
