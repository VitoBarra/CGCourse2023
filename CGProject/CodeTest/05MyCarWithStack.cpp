#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>
#include "simple_shapes.h"
#include "shaders.h"
#include "matrix_stack.h"

int MyCarWithStack()
{
    GLFWwindow *window;
    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(1000, 800, "My car with MatrixStack", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    glewInit();

    printout_opengl_glsl_info();

    glEnable(GL_DEPTH_TEST);


    Shader shader = *Shader::CreateShaderFromFile("../src/Shaders/BasicNoPass.vert",
                                                  "../src/Shaders/flat.frag");
    shader.RegisterUniformVariable("uP"); //View->Projection (NDC)
    shader.RegisterUniformVariable("uV"); //Word->View
    shader.RegisterUniformVariable("uT"); //Transformation
    shader.RegisterUniformVariable("uColor");

    /* Transformation to setup the point of viewMatrix on the scene */
    auto identityMatrix = glm::mat4(1.0);
    auto perspectiveProjection = glm::perspective(glm::radians(45.f), 1.33f, 0.1f, 100.f);
    auto viewMatrix = glm::lookAt(glm::vec3(0, 5, 10.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));

    /* Set shader matrix */
    shader.SetAsCurrentProgram();
    glUniformMatrix4fv(shader["uT"], 1, GL_FALSE, &identityMatrix[0][0]);
    glUniformMatrix4fv(shader["uV"], 1, GL_FALSE, &viewMatrix[0][0]);
    glUniformMatrix4fv(shader["uP"], 1, GL_FALSE, &perspectiveProjection[0][0]);
    glUseProgram(0);
    CheckGLErrors(__LINE__, __FILE__);




    /* create a cube centered at the origin with side 2*/
    Renderable r_cube = shape_maker::cube(0.5, 0.3, 0.0);
    /* create a cylinder with base on the XZ plane, and height=2*/
    Renderable r_cyl = shape_maker::cylinder(30, 0.2, 0.1, 0.5);
    /* create 3 lines showing the reference frame*/
    Renderable r_frame = shape_maker::frame(4.0);
    /* create a plane*/
    Renderable r_plane = shape_maker::rectangle(1, 1);
    CheckGLErrors(__LINE__, __FILE__);

    /*create car body */
    auto cubeToBody = glm::scale(identityMatrix, glm::vec3(1.5, 0.5, 0.8));


    auto b = glm::scale(identityMatrix, glm::vec3(1.5, 0.1, 0.8));
    auto tra = glm::translate(identityMatrix, glm::vec3(0.0, 0.6, 0.0));
    auto cubeToCarRoof = tra * b;



    /*create transformations to create weals */
    glm::mat4 cylinderToWheel[4];
    std::vector<std::pair<float, float>> transformsValues = {{-1, 0.8},
                                                             {1,  0.8},
                                                             {-1, -1},
                                                             {1,  -1}};

    auto cylro = glm::rotate(identityMatrix, glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0));
    auto cylScale = glm::scale(identityMatrix, glm::vec3(0.5, 0.1, 0.5));

    for (int i = 0; i < 4; i++) {
        auto Tvalue = transformsValues[i];
        auto cyltra = glm::translate(identityMatrix, glm::vec3(Tvalue.first, -0.3, Tvalue.second));
        cylinderToWheel[i] = cyltra * cylro * cylScale;
    }

    /* scale the plane to make it bigger */
    glm::mat4 s_plane(1.0);
    s_plane[0][0] = s_plane[2][2] = 4.0;

    /* Transformation to lift up the car */
    auto raise_up = glm::translate(identityMatrix, glm::vec3(0.0, 0.8, 0.0));


    matrix_stack matrixStack = *new matrix_stack(raise_up);

    /* Loop until the user closes the window */
    for (int i = 0; !glfwWindowShouldClose(window); i++) {

        viewMatrix = glm::rotate(viewMatrix, glm::radians(1.0f), glm::vec3(0.0, 1.0, 0.0));


        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.SetAsCurrentProgram();

        glUniformMatrix4fv(shader["uV"], 1, GL_FALSE, &viewMatrix[0][0]);

        matrixStack.pushMultiply(cubeToBody);
        /* Draw body */
        r_cube.SetAsCurrentObjectToRender();
        glUniformMatrix4fv(shader["uT"], 1, GL_FALSE, &matrixStack.pop()[0][0]);
        glUniform3f(shader["uColor"], 0.0, 1.0, 0.8);
        glDrawElements(GL_TRIANGLES, r_cube.NumberOfIndices, GL_UNSIGNED_INT, 0);

        matrixStack.pushMultiply(cubeToCarRoof);
        glUniformMatrix4fv(shader["uT"], 1, GL_FALSE, &matrixStack.pop()[0][0]);
        glUniform3f(shader["uColor"], 0.0, 0.0, 1.0);
        glDrawElements(GL_TRIANGLES, r_cube.NumberOfIndices, GL_UNSIGNED_INT, 0);

        /* Draw weals */
        r_cyl.SetAsCurrentObjectToRender();
        glUniform3f(shader["uColor"], 0.0, 1.0, 0.0);
        for (int iw = 0; iw < 4; iw++) {
            matrixStack.pushMultiply(cylinderToWheel[iw]);
            glUniformMatrix4fv(shader["uT"], 1, GL_FALSE, &matrixStack.pop()[0][0]);
            glDrawElements(GL_TRIANGLES, r_cyl.NumberOfIndices, GL_UNSIGNED_INT, 0);
        }

        r_plane.SetAsCurrentObjectToRender();
        glUniform3f(shader["uColor"], 0.2, 0.5, 0.2);
        glUniformMatrix4fv(shader["uT"], 1, GL_FALSE, &s_plane[0][0]);
        glDrawElements(GL_TRIANGLES, r_plane.NumberOfIndices, GL_UNSIGNED_INT, 0);



        /* Draw the frame */
        glUniformMatrix4fv(shader["uT"], 1, GL_FALSE, &identityMatrix[0][0]);
        glUniform3f(shader["uColor"], -1.0, 0.0, 1.0);
        r_frame.SetAsCurrentObjectToRender();
        glDrawArrays(GL_LINES, 0, 6);

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