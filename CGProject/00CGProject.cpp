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

trackball trackballs[2];
int curr_tb;
bool debugMode = true;

glm::mat4 viewMatrix;
glm::mat4 trasMatrix;
glm::mat4 perspProjection;
glm::mat4 identityMatrix;
glm::mat4 skyBoxScale;


void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    bool pressed = action == GLFW_REPEAT || action == GLFW_PRESS;
    if (key == GLFW_KEY_A && pressed)
        trasMatrix = glm::translate(trasMatrix, glm::vec3(-0.1, 0.0, 0.0));
    if (key == GLFW_KEY_S && pressed)
        trasMatrix = glm::translate(trasMatrix, glm::vec3(0.0, 0.0, -0.1));
    if (key == GLFW_KEY_D && pressed)
        trasMatrix = glm::translate(trasMatrix, glm::vec3(0.1, 0.0, 0.0));
    if (key == GLFW_KEY_W && pressed)
        trasMatrix = glm::translate(trasMatrix, glm::vec3(0.0, 0.0, 0.1));
    if (key == GLFW_KEY_Q && action == GLFW_PRESS)
        curr_tb = 1 - curr_tb;
    if (key == GLFW_KEY_E && action == GLFW_PRESS)
       debugMode = !debugMode;
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
        for (auto &tracball: trackballs)
            tracball.reset();
}

static void cursor_position_callback(GLFWwindow *window, double xpos, double ypos)
{
    trackballs[curr_tb].mouse_move(perspProjection, viewMatrix, xpos, ypos);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        trackballs[curr_tb].mouse_press(perspProjection, viewMatrix, xpos, ypos);
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        trackballs[curr_tb].mouse_release();
    }
}


/* callback function called when a mouse wheel is rotated */
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    if (curr_tb == 0)
        trackballs[0].mouse_scroll(xoffset, yoffset);
}


#endif

int main()
{
#ifdef COMPILE_PROF_CODE
    //    lez15("knife.obj");
        lez13();
        return 0;
#else
    GLFWwindow *window = GLFWWindowStarter::CreateWindow(1000, 800, "CG_Project");
    glewInit();

    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    /* set the trackball position */
    trackballs[0].set_center_radius(glm::vec3(0, 0, 0), 2.f);
    trackballs[1].set_center_radius(glm::vec3(0, 0, 0), 2.f);
    curr_tb = 0;

    /* define the viewport  */
    glViewport(0, 0, 1000, 800);


    printout_opengl_glsl_info();


    glEnable(GL_DEPTH_TEST);
    // glEnable(GL_CULL_FACE);
    CheckGLErrors(__LINE__, __FILE__);

    /* load from file */
    std::vector<Renderable> loadedModel;
    load_obj(loadedModel, "../Models/Datsun_280Z", "datsun_280Z.obj");

    /* initial light direction */
    glm::vec4 lightDirec = glm::vec4(0.0, 1.0, 0.0, 0.0);


    /* Transformation to setup the point of viewMatrix on the scene */
    identityMatrix = glm::mat4(1.0);
    trasMatrix = glm::mat4(1.0);
    perspProjection = glm::perspective(glm::radians(45.F), 1.33F, 0.1F, 100.F);
    viewMatrix = glm::lookAt(glm::vec3(0, 5, 10.F), glm::vec3(0.F, 0.F, 0.F), glm::vec3(0.F, 1.F, 0.F));
    skyBoxScale = glm::scale(glm::mat4(1.f), glm::vec3(40.0, 40.0, 40.0));

    // auto car_frame = glm::fra(4.0);
    std::string shaders_path = "../Shaders/";

    Shader Phong_shader = *Shader::CreateShaderFromFile(shaders_path, "phong.vert", "phong.frag");
    Phong_shader.RegisterUniformVariable("uP"); //View->Projection (NDC)
    Phong_shader.RegisterUniformVariable("uV"); //Word->View
    Phong_shader.RegisterUniformVariable("uT"); //Transformation
    Phong_shader.RegisterUniformVariable("uShadingMode");
    Phong_shader.RegisterUniformVariable("uDiffuseColor");
    Phong_shader.RegisterUniformVariable("uSpecularColor");
    Phong_shader.RegisterUniformVariable("uAmbientColor");
    Phong_shader.RegisterUniformVariable("uEmissiveColor");
    Phong_shader.RegisterUniformVariable("uShininess");
    Phong_shader.RegisterUniformVariable("uRefractionIndex");
    CheckGLErrors(__LINE__, __FILE__);


    Shader flatAlpha_shader = *Shader::CreateShaderFromFile(shaders_path, "flat.vert", "flatAlpha.frag");
    flatAlpha_shader.RegisterUniformVariable("uP");
    flatAlpha_shader.RegisterUniformVariable("uV");
    flatAlpha_shader.RegisterUniformVariable("uT");
    flatAlpha_shader.RegisterUniformVariable("uColor");
    CheckGLErrors(__LINE__, __FILE__);

    Shader texture_shader = *Shader::CreateShaderFromFile(shaders_path, "SkyBox.vert", "SkyBox.frag");
    texture_shader.RegisterUniformVariable("uT");
    texture_shader.RegisterUniformVariable("uP");
    texture_shader.RegisterUniformVariable("uV");
    texture_shader.RegisterUniformVariable("uSkybox");
    CheckGLErrors(__LINE__, __FILE__);


    /* Set shader matrix */
    Phong_shader.LoadProgram();
    Phong_shader.SetUniform1i("uShadingMode", 2);
    Phong_shader.SetUniformMat4f("uT", identityMatrix);
    Phong_shader.SetUniformMat4f("uV", viewMatrix);
    Phong_shader.SetUniformMat4f("uP", perspProjection);
    CheckGLErrors(__LINE__, __FILE__);

    flatAlpha_shader.LoadProgram();
    flatAlpha_shader.SetUniformMat4f("uT", identityMatrix);
    flatAlpha_shader.SetUniformMat4f("uV", viewMatrix);
    flatAlpha_shader.SetUniformMat4f("uP", perspProjection);
    flatAlpha_shader.SetUniformVec4f("uColor", -1.0, 0.0, 0.0, 1.0);
    CheckGLErrors(__LINE__, __FILE__);

    int textureUnit = 1;
    texture_shader.LoadProgram();
    texture_shader.SetUniformMat4f("uT", skyBoxScale);
    texture_shader.SetUniformMat4f("uV", viewMatrix);
    texture_shader.SetUniformMat4f("uP", perspProjection);
    texture_shader.SetUniform1i("uSkybox", textureUnit);


    Shader::UnloadProgram();


    /*texture loading*/

    texture skybox;
    std::string path = "../Models/CubeTexture/Yokohama/";
    skybox.load_cubemap(path, "posx.jpg", "negx.jpg",
                        "posy.jpg", "negy.jpg",
                        "posz.jpg", "negz.jpg", textureUnit);

    /*------------------------------------------------*/


    /*object inizialization */

    /* crete a plane*/
    auto r_plane = shape_maker::Rectangle(10, 10);


    /* create 3 lines showing the reference frame*/
    auto r_frame = shape_maker::frame(4.0);


    /* create Light Line*/
    auto r_Lightline = shape_maker::line(10);


    auto r_skyBox = shape_maker::cube(1);
    /*------------------------------------------------*/


    /* Render loop */
    for (int i = 0; glfwWindowShouldClose(window) == 0; i++)
    {
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto worldTrackBall = trackballs[0].matrix();
        auto lightTrackBall = trackballs[1].matrix();


        texture_shader.LoadProgram();
        r_skyBox.SetAsCurrentObjectToRender();
        r_skyBox.RenderTriangles();
        Shader::UnloadProgram();
        CheckGLErrors(__LINE__, __FILE__);


        Phong_shader.LoadProgram();
        Phong_shader.SetUniformMat4f("uV", viewMatrix);
        Phong_shader.SetUniformVec4f("uLdir", lightTrackBall * lightDirec);
        if (loadedModel.empty())
            Phong_shader.SetUniformMat4f("uT", worldTrackBall * trasMatrix);
        else
        {
            /*scale the object using the diagonal of the bounding box of the vertices position.
            This operation guarantees that the drawing will be inside the unit cube.*/
            float diag = loadedModel[0].bbox.diagonal();
            auto scalingdiagonal = glm::scale(identityMatrix, glm::vec3(1.f / diag, 1.f / diag, 1.f / diag));

            Phong_shader.SetUniformMat4f("uT", worldTrackBall * trasMatrix * scalingdiagonal);

            for (auto &renderablePice: loadedModel)
            {
                renderablePice.SetAsCurrentObjectToRender();
                /* every Renderable object has its own material. Here just the diffuse color is used.
                ADD HERE CODE TO PASS OTHE MATERIAL PARAMETERS.
                */
                Phong_shader.SetUniformVec3f("uDiffuseColor", renderablePice.material.diffuse);
                Phong_shader.SetUniformVec3f("uAmbientColor", renderablePice.material.ambient);
                Phong_shader.SetUniformVec3f("uSpecularColor", renderablePice.material.specular);
                Phong_shader.SetUniformVec3f("uEmissiveColor", renderablePice.material.emission);
                Phong_shader.SetUniform1f("uShininess", renderablePice.material.shininess);
                Phong_shader.SetUniform1f("uRefractionIndex", renderablePice.material.RefractionIndex);
                renderablePice.elements[0].Render();
                CheckGLErrors(__LINE__, __FILE__);
            }
        }

        CheckGLErrors(__LINE__, __FILE__);
        Shader::UnloadProgram();


        if (debugMode)
        {
            flatAlpha_shader.LoadProgram();

            flatAlpha_shader.SetUniformMat4f("uT", lightTrackBall);
            flatAlpha_shader.SetUniformVec4f("uColor", 1.0, 1.0, 1.0, 1.0);
            r_Lightline.SetAsCurrentObjectToRender();
            r_Lightline.RenderLine();
            CheckGLErrors(__LINE__, __FILE__);


            flatAlpha_shader.SetUniformMat4f("uT", worldTrackBall);

            flatAlpha_shader.SetUniformVec4f("uColor", -1.0, 0.0, 0.0, 1.0);
            r_frame.SetAsCurrentObjectToRender();
            r_frame.RenderLine();
            CheckGLErrors(__LINE__, __FILE__);


            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            flatAlpha_shader.SetUniformVec4f("uColor", 0.0, 0.0, 0.5, 0.5);
            r_plane.SetAsCurrentObjectToRender();
            r_plane.RenderTriangles();
            glDisable(GL_BLEND);
            CheckGLErrors(__LINE__, __FILE__);


            Shader::UnloadProgram();
        }


        /* Swap front and back buffers */
        glfwSwapBuffers(window);
        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
#endif
}
