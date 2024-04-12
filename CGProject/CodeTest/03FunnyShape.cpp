#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "debugging.h"
#include "Renderable.h"
#include "shaders.h"


Renderable *Draw2D_FunnyShape();


int FunnyShape() {

    GLFWwindow *window;
    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(1000, 800, "FunnyShape", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    glewInit();

    printout_opengl_glsl_info();

    auto r = Draw2D_FunnyShape();
    r->SetAsCurrentObjectToRender();

    Shader *program_shader = Shader::CreateShaderFromFile("../src/Shaders/PositionSinFun.vert",
                                                          "../src/Shaders/ColorWithAlpha.frag");
    program_shader->RegisterUniformVariable("uDelta");

    int it = 0;
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window)) {
        it++;
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program_shader->Program);

        /* Set the uniform value time dependent value */
        glUniform1f((*program_shader)["uDelta"], (it % 100) / 200.0);
        /* issue the DrawTriangleElements command*/
        glDrawElements(GL_TRIANGLES, 6 * 3, GL_UNSIGNED_INT, nullptr);

        glUseProgram(0);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);


        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}


Renderable *Draw2D_FunnyShape() {
    auto *renderable = new Renderable();

/* create render data NumberOfIndices RAM */
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