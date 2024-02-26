#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "debugging.h"
#include "Renderable.h"
#include "shaders.h"


Renderable *SetUpRenderable();

Shader *ReadShaderFromFile(std::string vertexShader, std::string fragmentShader);

void DrawElements(int n);


int WrappedTriangle(void) {


    GLFWwindow *window;
    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(1000, 800, "WrappedTriangle", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    glewInit();

    printout_opengl_glsl_info();

    auto r = SetUpRenderable();

    Shader *program_shader = ReadShaderFromFile("../src/shaders/lez2.vert", "../src/shaders/lez2.frag");
    r->bind();

    int it = 0;
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window)) {
        it++;
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program_shader->Program);

        /* issue the DrawElements command*/
        glUniform1f((*program_shader)["uDelta"], (it % 100) / 200.0);
        DrawElements(6 * 3);

        glUseProgram(0);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);


        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

Shader *ReadShaderFromFile(std::string vertexShader, std::string fragmentShader) {
    auto *basic_shader = new Shader("../src/shaders/lez2.vert", "../src/shaders/lez2.frag");
    basic_shader->RegisterUniformVariable("uDelta");
    check_shader(basic_shader->VertexShader);
    check_shader(basic_shader->FragmentShader);
    validate_shader_program(basic_shader->Program);

    GlDebug_CheckError()
    return basic_shader;
}

Renderable *SetUpRenderable() {
    auto *renderable = new Renderable();

/* create render data in RAM */
    GLuint positionAttribIndex = 0;
    GLuint ColorAttribIndex = 1;


    std::vector<float> positionsAndColorsValues =
            {
                    -0.5, -0.5, 1.0, 0.0, 0.0,
                    -0.5, 0.0, 0.0, 1.0, 0.0,
                    -0.5, 0.5, 0.0, 0.0, 1.0,
                    0.0, 0.7, 0.0, 0.0, 1.0,
                    0.0, 0.3, 0.7, 0.0, 0.3,
                    0.0, -0.1, 0.3, 0.0, 0.7,
                    0.0, -0.7, 1.0, 0.0, 0.0,
                    0.5, -0.5, 1.0, 0.0, 0.0,
                    0.5, 0.0, 0.0, 1.0, 0.0,
                    0.5, 0.5, 0.0, 0.0, 1.0
            };

    renderable->AddVertexAttribute(positionsAndColorsValues,
                                   std::vector<GLuint>{positionAttribIndex, ColorAttribIndex},
                                   std::vector<unsigned int>{2, 3},
                                   sizeof(float) * 5,
                                   std::vector<unsigned int>{0, sizeof(float) * 2});
    GlDebug_CheckError();

    std::vector<GLuint> indices = {
            0, 3, 4,
            1, 4, 5,
            2, 5, 6,
            7, 3, 4,
            9, 5, 6,
            8, 4, 5
    };
    renderable->add_indices(indices, GL_TRIANGLES);

    GlDebug_CheckError();

    return renderable;
}