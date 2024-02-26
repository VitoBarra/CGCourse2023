#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <conio.h>
#include "Renderable.h"
#include "debugging.h"
#include "shaders.h"

int lez2(void)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(1000, 800, "code_2_wrapping", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    
    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    glewInit();

    printout_opengl_glsl_info();

    /* create render data in RAM */
    GLuint positionAttribIndex = 0;
    float positions[] = {   -0, -0.0,	// 1st vertex
                            0.5, -0.0,  // 2nd vertex
                            0.5, 0.5,	// 3nd vertex
                            0.0, 0.5    // 4th vertex
    };
    Renderable r;
    r.create();

    r.AddVertexAttribute(positions, 8, positionAttribIndex, 2);
    CheckGLErrors(__LINE__, __FILE__);

    GLuint colorAttribIndex = 1;
    float colors[] =      { 1.0, 0.0, 0.0,  // 1st vertex
                            0.0, 1.0, 0.0,  // 2nd vertex
                            0.0, 0.0, 1.0,  // 3rd vertex
                            1.0, 1.0, 1.0   // 4th vertex
    };
    r.AddVertexAttribute(colors, 12, colorAttribIndex, 3);
    CheckGLErrors(__LINE__, __FILE__);


    GLuint indices[] = { 0,1,2,0,2,3 };
    r.add_indices(indices, 3, GL_TRIANGLES);

    Shader basic_shader =*new Shader();
	basic_shader.create_program("../src/shaders/lez2.vert", "../src/shaders/lez2.frag");
    basic_shader.RegisterUniformVariable("uDelta");
	check_shader(basic_shader.VertexShader);
	check_shader(basic_shader.FragmentShader);
    validate_shader_program(basic_shader.Program);

    CheckGLErrors(__LINE__, __FILE__);

	r.bind();

	int it = 0;
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
		it++;
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(basic_shader.Program);

       /* update the value of uDelta in the fragment Shader */
		glUniform1f(basic_shader["uDelta"], (it % 100) / 200.0);

        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, NULL);
        CheckGLErrors(__LINE__, __FILE__);
        glUseProgram(0);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
