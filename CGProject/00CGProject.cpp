#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>
#include <string>
#include "debugging.h"
#include "texture.h"
#include "Renderable.h"
#include "shaders.h"
#include "simple_shapes.h"
#include "GLFWWindowStarter.h"
#include "trackball.h"
#include "view_manipulator.h"
#include "ShadowMap.h"


#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include "obj_loader.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
//#include "imgui/backends/imgui_impl_opengl3_loader.h"

#ifdef COMPILE_PROF_CODE
#include "source.h"
#endif

#ifndef COMPILE_PROF_CODE

trackball trackballs[2];
view_manipulator viewManipulator;
int trackBall_index;
bool debugMode = true;


/* shadowMap */
static int shadowMap_mode = 3;
float depth_bias = 0.003f;
float distance_light = 2.0f;
ShadowMap ShadowMap;
glm::mat4 lightView;
/* paramters of the VSM (it should be 0.5) */
static float k_plane_approx = 0.5;
/*Phong*/

int CarShininess = 10;
glm::vec3 CarEmissive = glm::vec3(0, 0, 0);
glm::vec3 CarAmbient = glm::vec3(0, 0, 0);

int terrainShininess = 100;
glm::vec3 terrainEmissive = glm::vec3(0, 0, 0);
glm::vec3 terrainAmbient = glm::vec3(0, 0, 0);

int car_selected_shader = 2;
int terrain_selected_shader = 2;

/*transformation*/
glm::mat4 viewMatrix;
glm::mat4 transMatrix;
glm::mat4 perspProjection;
glm::mat4 identityMatrix;
/*Enviroment Texture*/
glm::mat4 skyBoxScale;


/*debug*/
const char *debug_lightView_name[2] = {"No transf", "With transf"};
int lightViewType = 0;

/*GUI*/
const char *shaders_name[3] = {"Phong", "hadow", "Phong-Shadow"};
const char *trackBall_name[3] = {"Scene", "Light", "View"};
/* which algorithm to use */

void PrintState()
{
    if (debugMode)
    {
        std::cout << "------------------------------------------------------------" << std::endl;
        std::cout << "Current Trackball: " << trackBall_name[trackBall_index] << std::endl;
        std::cout << "ShadowMap Paramether:" << std::endl <<
                "  - TextureSize: " << " W:" << ShadowMap.SizeH << " H:" << ShadowMap.SizeW << std::endl <<
                "  - DepthBias: " << depth_bias << std::endl <<
                "  - Distance Light: " << distance_light << std::endl <<
                "Car Shader: " << shaders_name[car_selected_shader] << std::endl <<
                "  - shines: " << CarShininess << std::endl <<
                "Terrain Shader: " << shaders_name[terrain_selected_shader] << std::endl <<
                "  - shines: " << terrainShininess << std::endl <<
                "ViewLight: " << debug_lightView_name[lightViewType] << std::endl;
    }
}

static void cursor_position_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (trackBall_index < 2)
        trackballs[trackBall_index].mouse_move(perspProjection, viewMatrix, xpos, ypos);
    else
        viewManipulator.mouse_move(xpos, ypos);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        if (trackBall_index < 2)
            trackballs[trackBall_index].mouse_press(perspProjection, viewMatrix, xpos, ypos);
        else
            viewManipulator.mouse_press(xpos, ypos);
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        if (trackBall_index < 2)
            trackballs[trackBall_index].mouse_release();
        else
            viewManipulator.mouse_release();
    }
}

/* callback function called when a mouse wheel is rotated */
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    if (trackBall_index == 0)
        trackballs[0].mouse_scroll(xoffset, yoffset);
}


void Reset()
{
    trackballs[0].set_center_radius(glm::vec3(0, 0, 0), 2.f);
    trackballs[1].set_center_radius(glm::vec3(0, 0, 0), 2.f);
    for (auto &tracball: trackballs)
        tracball.reset();
    viewManipulator.reset();

    lightViewType = 1;

    trackBall_index = 0;
    distance_light = 2;
    depth_bias = 0.003f;
    CarShininess = 10;
    terrainShininess = 100;
    int mult = 1;
    ShadowMap.create(mult * 512, mult * 512);
    transMatrix = glm::mat4(1.0);

    debugMode = true;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    bool repeated = action == GLFW_REPEAT || action == GLFW_PRESS;
    bool pressed = action == GLFW_PRESS;

    if (repeated)
    {
        if (key == GLFW_KEY_A)
            transMatrix = glm::translate(transMatrix, glm::vec3(-0.1, 0.0, 0.0));
        if (key == GLFW_KEY_S)
            transMatrix = glm::translate(transMatrix, glm::vec3(0.0, 0.0, -0.1));
        if (key == GLFW_KEY_D)
            transMatrix = glm::translate(transMatrix, glm::vec3(0.1, 0.0, 0.0));
        if (key == GLFW_KEY_W)
            transMatrix = glm::translate(transMatrix, glm::vec3(0.0, 0.0, 0.1));
    }

    if (pressed)
    {
        if (key == GLFW_KEY_Q)
            trackBall_index = (trackBall_index + 1) % 3;
        if (key == GLFW_KEY_E)
            debugMode = !debugMode;
        if (key == GLFW_KEY_R)
            Reset();
        PrintState();
    }
}


void RenderCarModel(Shader shader, std::vector<Renderable> loadedModel, glm::vec4 lightDirec,
                    glm::mat4 cur_viewMatrix)
{
    auto worldTrackBall = trackballs[0].matrix();
    auto lightTrackBall = trackballs[1].matrix();
    shader.LoadProgram();

    shader.SetUniformVec3f("uEmissiveColor", CarEmissive);
    shader.SetUniformVec3f("uAmbientColor", CarAmbient);
    shader.SetUniform1f("uShininess", CarShininess);

    shader.SetUniformMat4f("uV", cur_viewMatrix);
    shader.SetUniformVec4f("uLdir", lightTrackBall * lightDirec);

    if (loadedModel.empty())
        shader.SetUniformMat4f("uT", worldTrackBall * transMatrix);
    else
    {
        /*scale the object using the diagonal of the bounding box of the vertices position.
            This operation guarantees that the drawing will be inside the unit cube.*/
        float diag = loadedModel[0].bbox.diagonal();
        auto scalingdiagonal = glm::scale(identityMatrix, glm::vec3(1.f / diag, 1.f / diag, 1.f / diag));

        shader.SetUniformMat4f("uT", worldTrackBall * transMatrix * scalingdiagonal);

        for (auto &renderablePice: loadedModel)
        {
            renderablePice.SetAsCurrentObjectToRender();
            /* every Renderable object has its own material. Here just the diffuse color is used.
                ADD HERE CODE TO PASS OTHE MATERIAL PARAMETERS.
                */
            shader.SetUniformVec3f("uDiffuseColor", renderablePice.material.diffuse);
            //Phong_shader.SetUniformVec3f("uAmbientColor", renderablePice.material.ambient);
            shader.SetUniformVec3f("uSpecularColor", renderablePice.material.specular);
            //Phong_shader.SetUniformVec3f("uEmissiveColor", renderablePice.material.emission);
            //shader.SetUniform1f("uShininess", renderablePice.material.shininess);
            shader.SetUniform1f("uRefractionIndex", renderablePice.material.RefractionIndex);
            renderablePice.elements[0].Render();
            CheckGLErrors(__LINE__, __FILE__);
        }
    }

    CheckGLErrors(__LINE__, __FILE__);
    Shader::UnloadProgram();
}

void RenderDebug(Shader flatAlpha_shader, Renderable r_plane, Renderable r_frame, Renderable r_Lightline,
                 Renderable r_LightViewline,
                 glm::mat4 cur_viewMatrix)
{
    auto worldTrackBall = trackballs[0].matrix();
    auto lightTrackBall = trackballs[1].matrix();
    if (debugMode)
    {
        flatAlpha_shader.LoadProgram();
        flatAlpha_shader.SetUniformMat4f("uV", cur_viewMatrix);


        flatAlpha_shader.SetUniformMat4f("uT", lightTrackBall);
        flatAlpha_shader.SetUniformVec4f("uColor", 1.0, 1.0, 1.0, 1.0);
        r_Lightline.SetAsCurrentObjectToRender();
        r_Lightline.RenderLine();
        CheckGLErrors(__LINE__, __FILE__);


        flatAlpha_shader.SetUniformVec4f("uColor", 1.0, 0.0, 1.0, 1.0);
        flatAlpha_shader.SetUniformMat4f("uT", identityMatrix);
        r_LightViewline.SetAsCurrentObjectToRender();
        r_LightViewline.RenderLine();
        CheckGLErrors(__LINE__, __FILE__);


        flatAlpha_shader.SetUniformMat4f("uT", worldTrackBall);
        flatAlpha_shader.SetUniformVec4f("uColor", -1.0, 0.0, 0.0, 1.0);
        r_frame.SetAsCurrentObjectToRender();
        r_frame.RenderLine();
        CheckGLErrors(__LINE__, __FILE__);

        glm::mat4 UpMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 2, 0.0));

        flatAlpha_shader.SetUniformMat4f("uT", worldTrackBall * UpMatrix);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        flatAlpha_shader.SetUniformVec4f("uColor", 0.0, 0.0, 0.5, 0.5);
        r_plane.SetAsCurrentObjectToRender();
        r_plane.RenderTriangles();
        glDisable(GL_BLEND);
        CheckGLErrors(__LINE__, __FILE__);


        Shader::UnloadProgram();
    }
}

void RenderEnviroment(Shader texture_shader, Renderable r_skyBox)
{
    texture_shader.LoadProgram();
    r_skyBox.SetAsCurrentObjectToRender();
    r_skyBox.RenderTriangles();
    Shader::UnloadProgram();
    CheckGLErrors(__LINE__, __FILE__);
}

void RenderTerrain(Shader shader, Renderable r_terrain, glm::mat4 cur_viewMatrix, glm::vec4 lightDirec)
{
    auto worldTrackBall = trackballs[0].matrix();
    auto lightTrackBall = trackballs[1].matrix();
    shader.LoadProgram();

    shader.SetUniformVec4f("uLdir", lightTrackBall * lightDirec);
    shader.SetUniformMat4f("uV", cur_viewMatrix);
    shader.SetUniformMat4f("uT", worldTrackBall * glm::scale(glm::mat4(1.0f), glm::vec3(50)));
    shader.SetUniformVec4f("uColor", 0.8f, 0.8f, 0.8f, 1);

    shader.SetUniformVec3f("uEmissiveColor", terrainEmissive);
    shader.SetUniformVec3f("uAmbientColor", terrainAmbient);
    shader.SetUniform1f("uShininess", terrainShininess);
    r_terrain.SetAsCurrentObjectToRender();
    r_terrain.RenderTriangles();
    CheckGLErrors(__LINE__, __FILE__);
}


void SetShadowParamether(Shader shader, glm::mat4 lightView)
{
    shader.LoadProgram();
    shader.SetUniformMat4f("uLightMatrix", lightView);
    shader.SetUniform2i("uShadowMapSize", ShadowMap.SizeW, ShadowMap.SizeH);
    shader.SetUniform1f("uBias", depth_bias);
    shader.SetUniform1i("uRenderMode", shadowMap_mode);
}

glm::mat4 CalculateLightViewMatrix(Renderable &r_LightViewline)
{
    auto worldTrackBall = trackballs[0].matrix();
    auto lightTrackBall = trackballs[1].matrix();

    auto eye = glm::vec3(0, distance_light, 0.f);
    auto center = glm::vec3(0.f);
    auto up = glm::vec3(0.f, 0.f, -1.f);
    glm::mat4 lightViewMatrix;

    if (lightViewType == 0)
    {
        lightViewMatrix = glm::lookAt(eye, center, up) * glm::inverse(lightTrackBall);
        r_LightViewline = shape_maker::line(distance_light, center, eye);
    }
    else if (lightViewType == 1)
    {
        auto newEye = glm::vec3(worldTrackBall * transMatrix * glm::vec4(eye, 1.f));
        auto newCenter = glm::vec3(worldTrackBall * transMatrix * glm::vec4(center, 1.f));
        //This shoud align the light camera and the plane
        lightViewMatrix = glm::lookAt(newEye, newCenter, up) * glm::inverse(lightTrackBall);
        r_LightViewline = shape_maker::line(distance_light, newCenter, newEye);
    }

    return lightViewMatrix;
}

void gui(Renderable &r_LightViewline)
{
    ImGui::BeginMainMenuBar();

    if (ImGui::BeginMenu("Shadow mode"))
    {
        if (ImGui::Selectable("none", shadowMap_mode == 0)) shadowMap_mode = 0;
        if (ImGui::Selectable("Basic shadow mapping", shadowMap_mode == 1)) shadowMap_mode = 1;
        if (ImGui::Selectable("bias", shadowMap_mode == 2)) shadowMap_mode = 2;
        if (ImGui::Selectable("slope bias", shadowMap_mode == 3)) shadowMap_mode = 3;
        if (ImGui::Selectable("back faces", shadowMap_mode == 4)) shadowMap_mode = 4;
        if (ImGui::Selectable("PCF", shadowMap_mode == 5)) shadowMap_mode = 5;
        if (ImGui::Selectable("Variance SM", shadowMap_mode == 6)) shadowMap_mode = 6;
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Shadow Map "))
    {
        const char *items[] = {"32", "64", "128", "256", "512", "1024", "2048", "4096"};
        int h, w;
        if (ImGui::ListBox("width", &w, items, IM_ARRAYSIZE(items), 8))
            if (ShadowMap.SizeW != 1 << (5 + w))
            {
                ShadowMap.DepthBuffer.remove();
                ShadowMap.create(1 << (5 + w), ShadowMap.SizeH);
            }
        if (ImGui::ListBox("height", &h, items, IM_ARRAYSIZE(items), 8))
            if (ShadowMap.SizeH != 1 << (5 + h))
            {
                ShadowMap.DepthBuffer.remove();
                ShadowMap.create(ShadowMap.SizeW, 1 << (5 + h));
            }

        if (ImGui::SliderFloat("distance", &distance_light, 2.f, 100.f))
            ShadowMap.set_projection(CalculateLightViewMatrix(r_LightViewline), distance_light, box3(1.0));
        ImGui::SliderFloat("  plane approx", &k_plane_approx, 0.0, 1.0);


        ImGui::InputFloat("depth bias", &depth_bias, 0, 0, "%.5f");
        ImGui::EndMenu();
    }


    if (ImGui::BeginMenu("Car Shader"))
    {
        int s = 0;
        if (ImGui::ListBox("Cshader", &s, shaders_name, IM_ARRAYSIZE(shaders_name), 3))
            car_selected_shader = s;
        if (car_selected_shader == 2)
        {
            if (ImGui::SliderInt("Shininess", &CarShininess, 2, 100));

            ImGui::Text("Emissive:");
            ImGui::SliderFloat("ce-Red", &CarEmissive[0], 0.0f, 1.0f);
            ImGui::SliderFloat("ce-Green", &CarEmissive[1], 0.0f, 1.0f);
            ImGui::SliderFloat("ce-Blue", &CarEmissive[2], 0.0f, 1.0f);

            ImGui::Text("Ambient:");
            ImGui::SliderFloat("ca-Red", &CarAmbient[0], 0.0f, 1.0f);
            ImGui::SliderFloat("ca-Green", &CarAmbient[1], 0.0f, 1.0f);
            ImGui::SliderFloat("ca-Blue", &CarAmbient[2], 0.0f, 1.0f);
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Terrain Shader"))
    {
        int s = 0;
        if (ImGui::ListBox(" Tshader", &s, shaders_name, IM_ARRAYSIZE(shaders_name), 3))
            terrain_selected_shader = s;
        if (terrain_selected_shader == 2)
        {
            if (ImGui::SliderInt("Shininess", &terrainShininess, 2, 100));

            ImGui::Text("Emissive:");
            ImGui::SliderFloat("te-Red", &terrainEmissive[0], 0.0f, 1.0f);
            ImGui::SliderFloat("te-Green", &terrainEmissive[1], 0.0f, 1.0f);
            ImGui::SliderFloat("te-Blue", &terrainEmissive[2], 0.0f, 1.0f);

            ImGui::Text("Ambient:");
            ImGui::SliderFloat("Ta-Red", &terrainAmbient[0], 0.0f, 1.0f);
            ImGui::SliderFloat("Ta-Green", &terrainAmbient[1], 0.0f, 1.0f);
            ImGui::SliderFloat("Ta-Blue", &terrainAmbient[2], 0.0f, 1.0f);
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Control"))
    {
        int i;
        ImGui::Text("Trackball:");
        if (ImGui::ListBox("TrackB", &i, trackBall_name, IM_ARRAYSIZE(trackBall_name), 3))
            trackBall_index = i;


        ImGui::Text("System:");
        if (ImGui::Button("Reset"))
            Reset();

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Debug"))
    {
        int i;
        if (ImGui::ListBox("LightView", &i, debug_lightView_name, IM_ARRAYSIZE(debug_lightView_name), 2))
            lightViewType = i;

        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
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
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    glewInit();

    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplOpenGL3_Init();
    ImGui_ImplGlfw_InitForOpenGL(window, true);


    /* inizialize mause interaction */
    Reset();


    /* initial light direction */
    glm::vec4 lightDirec = glm::vec4(0.0, 1.0, 0.0, 0.0);
    /* Transformation to setup the point of viewMatrix on the scene */
    identityMatrix = glm::mat4(1.0);
    perspProjection = glm::perspective(glm::radians(45.F), 1.33F, 0.1F, 100.F);
    viewMatrix = glm::lookAt(glm::vec3(0, 5, 10.F), glm::vec3(0.F, 0.F, 0.F), glm::vec3(0.F, 1.F, 0.F));
    skyBoxScale = glm::scale(glm::mat4(1.f), glm::vec3(40.0, 40.0, 40.0));

    /* define the viewport  */
    glViewport(0, 0, 1000, 800);


    printout_opengl_glsl_info();
    std::cout << std::endl;


    glEnable(GL_DEPTH_TEST);
    CheckGLErrors(__LINE__, __FILE__);

    /* load from file */
    std::vector<Renderable> loadedModel;
    load_obj(loadedModel, "../Models/Datsun_280Z", "datsun_280Z.obj");


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

    Shader depth_shader = *Shader::CreateShaderFromFile(shaders_path, "depth_map.vert", "depth_map.frag");
    depth_shader.RegisterUniformVariable("uT");
    depth_shader.RegisterUniformVariable("uLightMatrix");
    depth_shader.RegisterUniformVariable("uPlaneApprox");

    Shader shadowMap_shader = *Shader::CreateShaderFromFile(shaders_path, "shadow_mapping.vert", "shadow_mapping.frag");
    shadowMap_shader.RegisterUniformVariable("uT");
    shadowMap_shader.RegisterUniformVariable("uP");
    shadowMap_shader.RegisterUniformVariable("uV");
    shadowMap_shader.RegisterUniformVariable("uShadowMap");
    shadowMap_shader.RegisterUniformVariable("uLightMatrix");
    shadowMap_shader.RegisterUniformVariable("uShadowMapSize");
    shadowMap_shader.RegisterUniformVariable("uBias");
    shadowMap_shader.RegisterUniformVariable("uRenderMode");
    shadowMap_shader.RegisterUniformVariable("uDiffuseColor");

    Shader Phong_shadowMap_shader = *Shader::CreateShaderFromFile(shaders_path, "Phong_shadow_mapping.vert",
                                                                  "Phong_shadow_mapping.frag");

    Phong_shadowMap_shader.RegisterUniformVariable("uT");
    Phong_shadowMap_shader.RegisterUniformVariable("uP");
    Phong_shadowMap_shader.RegisterUniformVariable("uV");
    Phong_shadowMap_shader.RegisterUniformVariable("uShadowMap");
    Phong_shadowMap_shader.RegisterUniformVariable("uLightMatrix");
    Phong_shadowMap_shader.RegisterUniformVariable("uShadowMapSize");
    Phong_shadowMap_shader.RegisterUniformVariable("uBias");
    Phong_shadowMap_shader.RegisterUniformVariable("uRenderMode");

    Phong_shadowMap_shader.RegisterUniformVariable("uDiffuseColor");
    Phong_shadowMap_shader.RegisterUniformVariable("uSpecularColor");
    Phong_shadowMap_shader.RegisterUniformVariable("uAmbientColor");
    Phong_shadowMap_shader.RegisterUniformVariable("uEmissiveColor");
    Phong_shadowMap_shader.RegisterUniformVariable("uShininess");
    Phong_shadowMap_shader.RegisterUniformVariable("uRefractionIndex");


    /* Set shader matrix */
    Phong_shader.LoadProgram();
    Phong_shader.SetUniform1i("uShadingMode", 2);
    Phong_shader.SetUniformMat4f("uT", identityMatrix);
    Phong_shader.SetUniformMat4f("uV", viewMatrix);
    Phong_shader.SetUniformMat4f("uP", perspProjection);
    Phong_shader.SetUniformVec3f("uDiffuseColor", glm::vec3(0.2));
    CheckGLErrors(__LINE__, __FILE__);

    flatAlpha_shader.LoadProgram();
    flatAlpha_shader.SetUniformMat4f("uT", identityMatrix);
    flatAlpha_shader.SetUniformMat4f("uV", viewMatrix);
    flatAlpha_shader.SetUniformMat4f("uP", perspProjection);
    flatAlpha_shader.SetUniformVec4f("uColor", -1.0, 0.0, 0.0, 1.0);
    CheckGLErrors(__LINE__, __FILE__);


    depth_shader.LoadProgram();
    depth_shader.SetUniformMat4f("uT", identityMatrix);
    depth_shader.SetUniformMat4f("uLightMatrix", lightView);
    depth_shader.SetUniform1f("uPlaneApprox", k_plane_approx);


    int shadowMapUt = 0;
    shadowMap_shader.LoadProgram();
    shadowMap_shader.SetUniformMat4f("uT", identityMatrix);
    shadowMap_shader.SetUniformMat4f("uV", viewMatrix);
    shadowMap_shader.SetUniformMat4f("uP", perspProjection);
    shadowMap_shader.SetUniformMat4f("uLightMatrix", lightView);
    shadowMap_shader.SetUniform1i("uShadowMap", shadowMapUt);
    shadowMap_shader.SetUniform2i("uShadowMapSize", ShadowMap.SizeW, ShadowMap.SizeH);
    shadowMap_shader.SetUniform1f("uBias", depth_bias);
    shadowMap_shader.SetUniform1i("uRenderMode", shadowMap_mode);
    shadowMap_shader.SetUniformVec3f("uDiffuseColor", glm::vec3(0.2));
    CheckGLErrors(__LINE__, __FILE__);


    Phong_shadowMap_shader.LoadProgram();
    Phong_shadowMap_shader.SetUniformMat4f("uT", identityMatrix);
    Phong_shadowMap_shader.SetUniformMat4f("uV", viewMatrix);
    Phong_shadowMap_shader.SetUniformMat4f("uP", perspProjection);
    Phong_shadowMap_shader.SetUniformMat4f("uLightMatrix", lightView);
    Phong_shadowMap_shader.SetUniform1i("uShadowMap", shadowMapUt);
    Phong_shadowMap_shader.SetUniform2i("uShadowMapSize", ShadowMap.SizeW, ShadowMap.SizeH);
    Phong_shadowMap_shader.SetUniform1f("uBias", depth_bias);
    Phong_shadowMap_shader.SetUniform1i("uRenderMode", shadowMap_mode);
    Phong_shadowMap_shader.SetUniformVec3f("uEmissiveColor", CarEmissive);
    Phong_shadowMap_shader.SetUniformVec3f("uAmbientColor", CarAmbient);
    Phong_shadowMap_shader.SetUniform1f("uShininess", CarShininess);
    Phong_shadowMap_shader.SetUniformVec3f("uDiffuseColor", glm::vec3(0.2));


    int textureUnit = 1;
    texture_shader.LoadProgram();
    texture_shader.SetUniformMat4f("uT", skyBoxScale);
    texture_shader.SetUniformMat4f("uV", viewMatrix);
    texture_shader.SetUniformMat4f("uP", perspProjection);
    texture_shader.SetUniform1i("uSkybox", textureUnit);
    CheckGLErrors(__LINE__, __FILE__);

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
    auto r_plane = shape_maker::Rectangle(1, 1);
    auto r_terrain = shape_maker::Rectangle(1, 1);


    /* create 3 lines showing the reference frame*/
    auto r_frame = shape_maker::frame(4.0);


    /* create Light Line*/
    auto r_Lightline = shape_maker::line(4);
    auto r_LightViewline = shape_maker::line(4);


    auto r_skyBox = shape_maker::cube(1);
    /*------------------------------------------------*/


    /* Render loop */
    for (int i = 0; glfwWindowShouldClose(window) == 0; i++)
    {
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        gui(r_LightViewline);

        auto worldTrackBall = trackballs[0].matrix();
        auto lightTrackBall = trackballs[1].matrix();

        auto cur_viewMatrix = viewManipulator.apply_to_view(viewMatrix);

        glm::mat4 lightViewMatrix = CalculateLightViewMatrix(r_LightViewline);
        ShadowMap.set_projection(lightViewMatrix, distance_light, box3(2.0));
        auto lightView = ShadowMap.light_matrix();


        RenderEnviroment(texture_shader, r_skyBox);

        ShadowMap.Bind();
        depth_shader.LoadProgram();
        depth_shader.SetUniformMat4f("uT", worldTrackBall);
        depth_shader.SetUniformMat4f("uLightMatrix", lightView);
        depth_shader.SetUniform1f("uPlaneApprox", k_plane_approx);


        if (shadowMap_mode == 4 || shadowMap_mode == 5)
        {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
        }


        RenderTerrain(depth_shader, r_terrain, cur_viewMatrix, lightDirec);
        RenderCarModel(depth_shader, loadedModel, lightDirec, cur_viewMatrix);

        glDisable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        Shader::UnloadProgram();


        glViewport(0, 0, 1000, 800);


        SetShadowParamether(shadowMap_shader, lightView);
        SetShadowParamether(Phong_shadowMap_shader, lightView);

        glActiveTexture(GL_TEXTURE0 + shadowMapUt);
        glBindTexture(GL_TEXTURE_2D, ShadowMap.DepthBuffer.id_depth);

        if (terrain_selected_shader == 0)
            RenderTerrain(Phong_shader, r_terrain, cur_viewMatrix, lightDirec);
        if (terrain_selected_shader == 1)
            RenderTerrain(shadowMap_shader, r_terrain, cur_viewMatrix, lightDirec);
        if (terrain_selected_shader == 2)
            RenderTerrain(Phong_shadowMap_shader, r_terrain, cur_viewMatrix, lightDirec);


        if (car_selected_shader == 0)
            RenderCarModel(Phong_shader, loadedModel, lightDirec, cur_viewMatrix);
        else if (car_selected_shader == 1)
            RenderCarModel(shadowMap_shader, loadedModel, lightDirec, cur_viewMatrix);
        else if (car_selected_shader == 2)
            RenderCarModel(Phong_shadowMap_shader, loadedModel, lightDirec, cur_viewMatrix);


        RenderDebug(flatAlpha_shader, r_plane, r_frame, r_Lightline, r_LightViewline, cur_viewMatrix);


        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        /* Swap front and back buffers */
        glfwSwapBuffers(window);
        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
#endif
}
