#pragma once


#include <iostream>
#include <string>
#include <vector>
#include <map>


#define GlDebug_CheckError() CheckGLErrors(__LINE__, __FILE__);

static void printout_opengl_glsl_info()
{
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *vendor = glGetString(GL_VENDOR);
    const GLubyte *version = glGetString(GL_VERSION);
    const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    std::cout << "GL Vendor            :" << vendor << std::endl;
    std::cout << "GL Renderer          :" << renderer << std::endl;
    std::cout << "GL Version (string)  :" << version << std::endl;
    std::cout << "GLSL Version         :" << glslVersion << std::endl;
}

static bool CheckGLErrors(int line, const char *file, bool exit_on_error = true)
{
    auto err = glGetError();
    std::string err_string;
    std::string message = "The offending command is ignoredand has no other side effect than to set the error flag.\n";
    std::string debugInfo = "Line: " + std::to_string(line) + " File: " + file + "\n";
    switch (err)
    {
        case GL_INVALID_ENUM:
            throw std::invalid_argument(
                "GL_INVALID_ENUM\n An unacceptable value is specified for an enumerated argument." + message +
                debugInfo);
            break;

        case GL_INVALID_VALUE:
            throw std::invalid_argument("GL_INVALID_VALUE\n A numeric argument is out of range."
                + message + debugInfo);
            break;

        case GL_INVALID_OPERATION:
        {
            throw std::invalid_argument(
                "GL_INVALID_OPERATION\n The specified operation is not allowed in the current state."
                + message + debugInfo);
        }
        break;

        case GL_INVALID_FRAMEBUFFER_OPERATION:
            throw std::invalid_argument(
                "GL_INVALID_FRAMEBUFFER_OPERATION\n  The framebuffer object is not complete."
                +message + debugInfo);
            break;

        case GL_OUT_OF_MEMORY:
            throw std::invalid_argument(
                "GL_OUT_OF_MEMORY\n There is not enough memory left to execute the command.The state of the GL is undefined, except for the state of the error flags, after this error is recorded.\n"
                + debugInfo);
            break;

        case GL_STACK_UNDERFLOW:
            throw std::invalid_argument(
                "GL_STACK_UNDERFLOW\n An attempt has been made to perform an operation that would cause an internal stack to underflow.\n"
                + debugInfo);
            break;

        case GL_STACK_OVERFLOW:
            throw std::invalid_argument(
                "GL_STACK_OVERFLOW\n An attempt has been made to perform an operation that would cause an internal stack to overflow.\n"
                + debugInfo);
            break;
        default:
            break;
    }
    bool ok_res = (err == GL_NO_ERROR);
    if (!ok_res && exit_on_error)
        exit(0);
    return ok_res;
}

static bool CheckGLErrors(bool exit_on_error = true)
{
    return CheckGLErrors(-1, ".", exit_on_error);
}

static bool check_shader(GLuint s, bool exit_on_error = true)
{
    std::vector<GLchar> buf;
    GLint l;
    glGetShaderiv(s, GL_COMPILE_STATUS, &l);

    if (l == GL_FALSE)
    {
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &l);
        buf.resize(l);
        glGetShaderInfoLog(s, l, &l, &buf[0]);
        std::cout << &buf[0] << std::endl;
        if (exit_on_error)
            exit(0);
        return false;
    }
    return true;
}

static bool validate_shader_program(GLuint s)
{
    GLint res;
    glValidateProgram(s);
    glGetProgramiv(s, GL_VALIDATE_STATUS, &res);
    std::cout << "validation of program " << s << " " << res << std::endl;
    if (res != GL_TRUE)
    {
        GLchar infoLog[65536];
        int length;
        glGetProgramInfoLog(s, 65536, &length, &infoLog[0]);
        std::cout << infoLog << "\n";
        return false;
    }
    glGetProgramiv(s, GL_LINK_STATUS, &res);
    std::cout << "linking of program " << s << " " << res << std::endl;
    if (res != GL_TRUE) return false;

    glGetProgramiv(s, GL_ACTIVE_ATTRIBUTES, &res);
    std::cout << "active attribute of program " << s << " " << res << std::endl;

    glGetProgramiv(s, GL_ACTIVE_UNIFORMS, &res);
    std::cout << "active uniform  of program " << s << " " << res << std::endl;

    glGetProgramiv(s, GL_ACTIVE_UNIFORM_MAX_LENGTH, &res);
    std::cout << "active uniform Max Length of program " << s << " " << res << std::endl;
    return true;
}
