#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include "debugging.h"
#include "Renderable.h"
#include "shaders.h"
#include "matrix_stack.h"
#include "simple_shapes.h"
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>
#include "GLFWWindowStarter.h"
#include "trackball.h"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include "obj_loader.h"

#ifdef COMPILE_PROF_CODE
#include "source.h"
#endif


#ifndef COMPILE_PROF_CODE

trackball trackball[2];
int curr_tb;

glm::mat4 viewMatrix;
glm::mat4 trasMatrix;
glm::mat4 perspProjection;
glm::mat4 identityMatrix;

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    bool pressed = GLFW_REPEAT || GLFW_PRESS;
    if (key == GLFW_KEY_A && pressed)
        trasMatrix = glm::translate(trasMatrix, glm::vec3(-0.1, 0.0, 0.0));
    if (key == GLFW_KEY_S && pressed)
        trasMatrix = glm::translate(trasMatrix, glm::vec3(0.0, 0.0, -0.1));
    if (key == GLFW_KEY_D && pressed)
        trasMatrix = glm::translate(trasMatrix, glm::vec3(0.1, 0.0, 0.0));
    if (key == GLFW_KEY_W && pressed)
        trasMatrix = glm::translate(trasMatrix, glm::vec3(0.0, 0.0, 0.1));
}

static void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
    trackball[curr_tb].mouse_move(perspProjection, viewMatrix, xpos, ypos);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        trackball[curr_tb].mouse_press(perspProjection, viewMatrix, xpos, ypos);
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        trackball[curr_tb].mouse_release();
    }
}


/* callback function called when a mouse wheel is rotated */
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    if (curr_tb == 0)
        trackball[0].mouse_scroll(xoffset, yoffset);
}

#endif

int main() {


#ifdef COMPILE_PROF_CODE
    //    lez15("knife.obj");
        lez9();
        return 0;
#else
    GLFWwindow *window = GLFWWindowStarter::CreateWindow(1000, 800, "CG_Project");
    glewInit();

    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    /* set the trackball position */
    trackball[0].set_center_radius(glm::vec3(0, 0, 0), 2.f);
    trackball[1].set_center_radius(glm::vec3(0, 0, 0), 2.f);
    curr_tb = 0;

    /* define the viewport  */
    glViewport(0, 0, 1000, 800);


    printout_opengl_glsl_info();

    glEnable(GL_DEPTH_TEST);

    Shader shader = *Shader::CreateShaderFromFile("../Shaders/", "Basic.vert", "FlatShading.frag");
    shader.RegisterUniformVariable("uP") //View->Projection (NDC)
            .RegisterUniformVariable("uV") //Word->View
            .RegisterUniformVariable("uT") //Transformation
            .RegisterUniformVariable("uColor");

    /* Transformation to setup the point of viewMatrix on the scene */
    identityMatrix = glm::mat4(1.0);
    trasMatrix = glm::mat4(1.0);
    perspProjection = glm::perspective(glm::radians(45.F), 1.33F, 0.1F, 100.F);
    viewMatrix = glm::lookAt(glm::vec3(0, 5, 10.F), glm::vec3(0.F, 0.F, 0.F), glm::vec3(0.F, 1.F, 0.F));
    /* Set shader matrix */
    shader.SetAsCurrentProgram();
    shader.SetUniformMat4f("uT", identityMatrix);
    shader.SetUniformMat4f("uV", viewMatrix);
    shader.SetUniformMat4f("uP", perspProjection);
    Shader::UnloadProgram();
    CheckGLErrors(__LINE__, __FILE__);


    glm::mat4 tempMatrix;
    matrix_stack matrixStack = *new matrix_stack(identityMatrix);

    /* load from file */
    std::vector<Renderable> r_cb;
    load_obj(r_cb, "../Models/Datsun_280Z", "datsun_280Z.obj");

    /* Loop until the user closes the window */
    for (int i = 0; glfwWindowShouldClose(window) == 0; i++) {

        // viewMatrix = glm::rotate(viewMatrix, glm::radians(1.0f), glm::vec3(0.0, 1.0, 0.0));


        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.SetAsCurrentProgram();
        shader.SetUniformMat4f("uV", viewMatrix);


        if (!r_cb.empty()) {

            /*scale the object using the diagonal of the bounding box of the vertices position.
            This operation guarantees that the drawing will be inside the unit cube.*/
            float diag = r_cb[0].bbox.diagonal();
            trasMatrix = glm::scale(glm::mat4(1.f), glm::vec3(1.f / diag, 1.f / diag, 1.f / diag));

            shader.SetUniformMat4f("uT", trasMatrix);

            for (auto &is: r_cb) {
                is.SetAsCurrentObjectToRender();
                shader.SetUniformVec3f("uColor", -1.0, 0.0, 0.0);
                /* every Renderable object has its own material. Here just the diffuse color is used.
                ADD HERE CODE TO PASS OTHE MATERIAL PARAMETERS.
                */
//                shader.SetUniformVec3f("uDiffuseColor", is.material.diffuse);
//                shader.SetUniformVec3f("uSpecularColor", is.material.specular);
//                shader.SetUniformVec3f("uAmbientColor", is.material.ambient);
//                shader.SetUniformVec3f("uEmissiveColor", is.material.emission);
//                shader.SetUniform1f("uRefractionIndex", is.material.ior);
//                shader.SetUniform1f("uShininess", is.material.shininess);
                is.elements[0].Render();
            }
            Shader::UnloadProgram();
        }

        auto tempMatrix = trackball[0].matrix();
        shader.SetUniformMat4f("uT", trackball[0].matrix() * trasMatrix);
        shader.SetUniformVec3f("uColor", 1.0, 0.0, 0.0);

        CheckGLErrors(__LINE__, __FILE__);


        Shader::UnloadProgram();

        /* Swap front and back buffers */
        glfwSwapBuffers(window);
        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
#endif
}


