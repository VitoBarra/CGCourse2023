#include <GL/glew.h>
#include <GL/gl.h>
#include <algorithm>
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
bool debugMode ;


/* shadowMap */
float depth_bias;
float distance_light;
static int shadowMap_mode;
int kernel_size;
ShadowMap ShadowMap;
glm::mat4 lightView;

/* paramters of the VSM (it should be 0.5) */
float k_plane_approx;

/*blinn-Phong*/
int car_selected_shader;
bool car_usePhongar = false;
bool car_userMaterial;
int CarShininess;
glm::vec3 CarEmissive;
glm::vec3 CarAmbient;

int terrain_selected_shader;
bool terrain_usePhongar = false;
int terrainShininess;
glm::vec3 terrainEmissive;
glm::vec3 terrainAmbient;


/*transformation*/
glm::mat4 viewMatrix;
glm::mat4 transMatrix;
glm::mat4 perspProjection;
glm::mat4 identityMatrix;

/*Enviroment Texture*/
glm::mat4 skyBoxScale;


/*GUI*/
const int SHADER_NUM = 1;
const char* shaders_name[SHADER_NUM] = {"Blinn-Phong-Shadow"};
const char* trackBall_name[3] = {"Scene", "Light", "View"};

const char* shadowMapDim[] = {"32", "64", "128", "256", "512", "1024", "2048", "4096"};
const char* current_width = NULL;
const char* current_hight = NULL;

/*uniform texture*/
int DiffuseTexture_tu = 0;
int SkyBox_tu  = 1;
int ShadowMap_tu = 2;

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
            "  - shines: " << terrainShininess << std::endl;
    }
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (trackBall_index < 2)
        trackballs[trackBall_index].mouse_move(perspProjection, viewMatrix, xpos, ypos);
    else
        viewManipulator.mouse_move(xpos, ypos);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos;
        double ypos;
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
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (trackBall_index == 0)
        trackballs[0].mouse_scroll(xoffset, yoffset);
}



void Reset()
{
    trackballs[0].set_center_radius(glm::vec3(0, 0, 0), 2.f);
    trackballs[1].set_center_radius(glm::vec3(0, 0, 0), 2.f);
    for (auto& tracball : trackballs)
        tracball.reset();
    viewManipulator.reset();


    /* general value */
    debugMode = true;
    transMatrix = glm::mat4(1.0);

    /* control */
    trackBall_index = 0;

    /* shadowMap */
    depth_bias = 0.003f;
    distance_light = 10.0f;
    shadowMap_mode = 5;
    kernel_size = 5;
    current_width = shadowMapDim[5];
    current_hight = shadowMapDim[5];
    ShadowMap.create(std::stoi(current_width), std::stoi(current_hight));


    /* paramters of the VSM (it should be 0.5) */
    k_plane_approx = 0.5;

    /*blinn-Phong*/
    car_usePhongar = false;
    car_userMaterial = false;
    CarShininess = 30;
    CarEmissive = glm::vec3(0, 0, 0);
    CarAmbient = glm::vec3(0, 0, 0);

    terrain_usePhongar = false;
    terrainShininess = 100;
    terrainEmissive = glm::vec3(0, 0, 0);
    terrainAmbient = glm::vec3(0, 0, 0);

     car_selected_shader = 0;
     terrain_selected_shader = 0;




}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
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


void RenderCarModel(Shader shader, std::vector<Renderable> loadedModel, const glm::mat4& frameViewMatrix,
                    const glm::vec4 lightDirec)
{
    auto worldTrackBall = trackballs[0].matrix();
    auto lightTrackBall = trackballs[1].matrix();
    shader.LoadProgram();

    shader.SetUniform1i("uUsePhong", car_usePhongar);
    shader.SetUniformVec3f("uEmissiveColor", CarEmissive);
    shader.SetUniformVec3f("uAmbientColor", CarAmbient);
    shader.SetUniform1f("uShininess", CarShininess);

    shader.SetUniformMat4f("uV", frameViewMatrix);
    shader.SetUniformVec4f("uLdir", lightTrackBall * lightDirec);
    shader.SetUniform1i("uUseMaterial",car_userMaterial);

    shader.SetUniform1i("uDiffuseTexture", DiffuseTexture_tu);



    if (loadedModel.empty())
        shader.SetUniformMat4f("uT", worldTrackBall * transMatrix);
    else
    {
        /*scale the object using the diagonal of the bounding box of the vertices position.
            This operation guarantees that the drawing will be inside the unit cube.*/
        float diag = loadedModel[0].bbox.diagonal();
        auto scalingdiagonal = glm::scale(identityMatrix, glm::vec3(1.f / diag, 1.f / diag, 1.f / diag));

        shader.SetUniformMat4f("uT", worldTrackBall * transMatrix * scalingdiagonal);



        for (auto& renderablePice : loadedModel)
        {
            renderablePice.SetAsCurrentObjectToRender();
            renderablePice.material.diffuse_texture.bind(DiffuseTexture_tu);
            shader.SetUniformVec3f("uDiffuseColor", renderablePice.material.diffuse);
            shader.SetUniformVec3f("uSpecularColor", renderablePice.material.specular);
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

void RenderEnviroment(Shader skybox_shader, Renderable r_skyBox, glm::mat4 frame_viewMatrix)
{
    skybox_shader.LoadProgram();
    skybox_shader.SetUniformMat4f("uV", frame_viewMatrix);
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

    shader.SetUniform1i("uUsePhong", terrain_usePhongar);
    shader.SetUniformVec4f("uLdir", lightTrackBall * lightDirec);
    shader.SetUniformMat4f("uV", cur_viewMatrix);
    shader.SetUniformMat4f("uT", worldTrackBall * glm::scale(glm::mat4(1.0f), glm::vec3(50)));
    shader.SetUniformVec4f("uColor", 0.8f, 0.8f, 0.8f, 1);

    shader.SetUniformVec3f("uEmissiveColor", terrainEmissive);
    shader.SetUniformVec3f("uAmbientColor", terrainAmbient);
    shader.SetUniform1f("uShininess", terrainShininess);

    shader.SetUniform1i("uUseMaterial", true);

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
    shader.SetUniform1i("uShadowMode", shadowMap_mode);
    shader.SetUniform1i("uKernelSize", kernel_size);
}

glm::mat4 CalculateLightViewMatrix(Renderable& r_LightViewline)
{
    auto worldTrackBall = trackballs[0].matrix();
    auto lightTrackBall = trackballs[1].matrix();

    auto eye = glm::vec3(0, distance_light, 0.f);
    auto center = glm::vec3(0.f);
    auto up = glm::vec3(0.f, 0.f, -1.f);
    glm::mat4 lightViewMatrix;


    lightViewMatrix = glm::lookAt(eye, center, up) * glm::inverse(lightTrackBall);
    r_LightViewline = shape_maker::line(distance_light, center, eye);


    return lightViewMatrix;
}

void gui(Renderable& r_LightViewline)
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
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Shadow Map "))
    {
        if (ImGui::BeginCombo("width", current_width))
        {
            for (int w = 0; w < IM_ARRAYSIZE(shadowMapDim); w++)
            {
                bool is_selected = current_width == shadowMapDim[w];
                if (ImGui::Selectable(shadowMapDim[w], is_selected))
                {
                    current_width = shadowMapDim[w];

                    if (ShadowMap.SizeW != 1 << (5 + w))
                    {
                        ShadowMap.DepthBuffer.remove();
                        ShadowMap.create(1 << (5 + w), ShadowMap.SizeH);
                    }
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (ImGui::BeginCombo("height", current_hight))
        {
            for (int h = 0; h < IM_ARRAYSIZE(shadowMapDim); h++)
            {
                bool is_selected = (current_hight == shadowMapDim[h]);
                if (ImGui::Selectable(shadowMapDim[h], is_selected))
                {
                    current_hight = shadowMapDim[h];

                    if (ShadowMap.SizeH != 1 << (5 + h))
                    {
                        ShadowMap.DepthBuffer.remove();
                        ShadowMap.create(ShadowMap.SizeW, 1 << (5 + h));
                    }
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }


        if (ImGui::SliderFloat("distance", &distance_light, 2.f, 100.f))
            ShadowMap.set_projection(CalculateLightViewMatrix(r_LightViewline), distance_light, box3(1.0));
        ImGui::SliderFloat("  plane approx", &k_plane_approx, 0.0, 1.0);


        ImGui::InputFloat("depth bias", &depth_bias, 0.001, 0.01, "%.5f");
        if (shadowMap_mode == 5)
        {
            ImGui::InputInt("kernel size", &kernel_size, 2, 6);
            kernel_size = std::max(kernel_size, 3);
            if (kernel_size % 2 == 0)
                kernel_size++;
        }


        ImGui::EndMenu();
    }


    if (ImGui::BeginMenu("Car Shader"))
    {
        int s = 0;
        if (ImGui::ListBox("Cshader", &s, shaders_name, IM_ARRAYSIZE(shaders_name), SHADER_NUM))
            car_selected_shader = s;

        ImGui::Checkbox("use Phong", &car_usePhongar);
        if (ImGui::Checkbox("use Material", &car_userMaterial));
        if (ImGui::SliderInt("Shininess", &CarShininess, 2, 100));

        ImGui::Text("Emissive:");
        ImGui::SliderFloat("ce-Red", &CarEmissive[0], 0.0f, 1.0f);
        ImGui::SliderFloat("ce-Green", &CarEmissive[1], 0.0f, 1.0f);
        ImGui::SliderFloat("ce-Blue", &CarEmissive[2], 0.0f, 1.0f);

        ImGui::Text("Ambient:");
        ImGui::SliderFloat("ca-Red", &CarAmbient[0], 0.0f, 1.0f);
        ImGui::SliderFloat("ca-Green", &CarAmbient[1], 0.0f, 1.0f);
        ImGui::SliderFloat("ca-Blue", &CarAmbient[2], 0.0f, 1.0f);


        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Terrain Shader"))
    {
        int s = 0;
        if (ImGui::ListBox(" Tshader", &s, shaders_name, IM_ARRAYSIZE(shaders_name), SHADER_NUM))
            terrain_selected_shader = s;

        ImGui::Checkbox("use Phong", &terrain_usePhongar);
        if (ImGui::SliderInt("Shininess", &terrainShininess, 2, 100));

        ImGui::Text("Emissive:");
        ImGui::SliderFloat("te-Red", &terrainEmissive[0], 0.0f, 1.0f);
        ImGui::SliderFloat("te-Green", &terrainEmissive[1], 0.0f, 1.0f);
        ImGui::SliderFloat("te-Blue", &terrainEmissive[2], 0.0f, 1.0f);

        ImGui::Text("Ambient:");
        ImGui::SliderFloat("Ta-Red", &terrainAmbient[0], 0.0f, 1.0f);
        ImGui::SliderFloat("Ta-Green", &terrainAmbient[1], 0.0f, 1.0f);
        ImGui::SliderFloat("Ta-Blue", &terrainAmbient[2], 0.0f, 1.0f);


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

    ImGui::EndMainMenuBar();
}


#endif
void RenderToShadowMapTexture(Shader depth_shader, glm::mat4 frame_viewMatrix, glm::vec4 lightDirec, std::vector<Renderable> loadedModel, Renderable r_terrain, glm::mat4 lightView)
{

    ShadowMap.Bind();
    depth_shader.LoadProgram();
    depth_shader.SetUniformMat4f("uLightMatrix", lightView);
    depth_shader.SetUniform1f("uPlaneApprox", k_plane_approx);


    if (shadowMap_mode == 4 || shadowMap_mode == 5)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
    }


    RenderTerrain(depth_shader, r_terrain, frame_viewMatrix, lightDirec);
    RenderCarModel(depth_shader, loadedModel, frame_viewMatrix, lightDirec);

    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    ShadowMap.undind();
    Shader::UnloadProgram();
}

int main()
{
#ifdef COMPILE_PROF_CODE
    //    lez15("knife.obj");
        lez13();
        return 0;
#else
    GLFWwindow* window = GLFWWindowStarter::CreateWindow(1000, 800, "CG_Project");
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    glewInit();

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplOpenGL3_Init();
    ImGui_ImplGlfw_InitForOpenGL(window, true);


    /* set default value */
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


    std::string shaders_path = "../Shaders/Prof/";


    Shader flatAlpha_shader = *Shader::CreateShaderFromFile(shaders_path, "flat.vert", "flatAlpha.frag");
    flatAlpha_shader.RegisterUniformVariable("uP");
    flatAlpha_shader.RegisterUniformVariable("uV");
    flatAlpha_shader.RegisterUniformVariable("uT");
    flatAlpha_shader.RegisterUniformVariable("uColor");
    CheckGLErrors(__LINE__, __FILE__);

    Shader skybox_shader = *Shader::CreateShaderFromFile(shaders_path, "SkyBox.vert", "SkyBox.frag");
    skybox_shader.RegisterUniformVariable("uT");
    skybox_shader.RegisterUniformVariable("uP");
    skybox_shader.RegisterUniformVariable("uV");
    skybox_shader.RegisterUniformVariable("uSkybox");
    CheckGLErrors(__LINE__, __FILE__);

    Shader depth_shader = *Shader::CreateShaderFromFile(shaders_path, "depth_map.vert", "depth_map.frag");
    depth_shader.RegisterUniformVariable("uT");
    depth_shader.RegisterUniformVariable("uLightMatrix");
    depth_shader.RegisterUniformVariable("uPlaneApprox");


    shaders_path = "../Shaders/";


    Shader BlinnPhong_shadowMap_shader = *Shader::CreateShaderFromFile(shaders_path, "Blinn-Phong_ShadowMapping.vert",
                                                                       "Blinn-Phong_ShadowMapping.frag");
    BlinnPhong_shadowMap_shader.RegisterUniformVariable("uT");
    BlinnPhong_shadowMap_shader.RegisterUniformVariable("uP");
    BlinnPhong_shadowMap_shader.RegisterUniformVariable("uV");
    BlinnPhong_shadowMap_shader.RegisterUniformVariable("uLdir");

    BlinnPhong_shadowMap_shader.RegisterUniformVariable("uDiffuseColor");
    BlinnPhong_shadowMap_shader.RegisterUniformVariable("uSpecularColor");
    BlinnPhong_shadowMap_shader.RegisterUniformVariable("uAmbientColor");
    BlinnPhong_shadowMap_shader.RegisterUniformVariable("uEmissiveColor");
    BlinnPhong_shadowMap_shader.RegisterUniformVariable("uShininess");
    BlinnPhong_shadowMap_shader.RegisterUniformVariable("uRefractionIndex");
    BlinnPhong_shadowMap_shader.RegisterUniformVariable("uLightModel");
    BlinnPhong_shadowMap_shader.RegisterUniformVariable("uUsePhong");
    BlinnPhong_shadowMap_shader.RegisterUniformVariable("uDiffuseTexture");
    BlinnPhong_shadowMap_shader.RegisterUniformVariable("uUseMaterial");

    BlinnPhong_shadowMap_shader.RegisterUniformVariable("uShadowMap");
    BlinnPhong_shadowMap_shader.RegisterUniformVariable("uLightMatrix");
    BlinnPhong_shadowMap_shader.RegisterUniformVariable("uShadowMode");
    BlinnPhong_shadowMap_shader.RegisterUniformVariable("uKernelSize");


    // init shader
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




    BlinnPhong_shadowMap_shader.LoadProgram();
    // transformation matrix
    BlinnPhong_shadowMap_shader.SetUniformMat4f("uT", identityMatrix);
    BlinnPhong_shadowMap_shader.SetUniformMat4f("uV", viewMatrix);
    BlinnPhong_shadowMap_shader.SetUniformMat4f("uP", perspProjection);


    //blinn-phong paramether
    BlinnPhong_shadowMap_shader.SetUniformVec3f("uEmissiveColor", CarEmissive);
    BlinnPhong_shadowMap_shader.SetUniformVec3f("uAmbientColor", CarAmbient);
    BlinnPhong_shadowMap_shader.SetUniform1f("uShininess", CarShininess);
    BlinnPhong_shadowMap_shader.SetUniformVec3f("uDiffuseColor", glm::vec3(0.2));
    BlinnPhong_shadowMap_shader.SetUniform1i("uLightModel", car_usePhongar);
    BlinnPhong_shadowMap_shader.SetUniform1i("uUseMaterial", true);
    BlinnPhong_shadowMap_shader.SetUniform1i("uDiffuseTexture", DiffuseTexture_tu);

    // ShadowMap paramether
    BlinnPhong_shadowMap_shader.SetUniformMat4f("uLightMatrix", lightView);
    BlinnPhong_shadowMap_shader.SetUniform2i("uShadowMapSize", ShadowMap.SizeW, ShadowMap.SizeH);
    BlinnPhong_shadowMap_shader.SetUniform1f("uBias", depth_bias);
    BlinnPhong_shadowMap_shader.SetUniform1i("uKernelSize", kernel_size);
    BlinnPhong_shadowMap_shader.SetUniform1i("uShadowMode", shadowMap_mode);
    BlinnPhong_shadowMap_shader.SetUniform1i("uShadowMap", ShadowMap_tu);



    skybox_shader.LoadProgram();
    skybox_shader.SetUniformMat4f("uT", skyBoxScale);
    skybox_shader.SetUniformMat4f("uV", viewMatrix);
    skybox_shader.SetUniformMat4f("uP", perspProjection);
    skybox_shader.SetUniform1i("uSkybox", SkyBox_tu);
    CheckGLErrors(__LINE__, __FILE__);

    Shader::UnloadProgram();


    /*texture loading*/

    texture skybox;
    std::string path = "../Models/CubeTexture/Yokohama/";
    skybox.load_cubemap(path, "posx.jpg", "negx.jpg",
                        "posy.jpg", "negy.jpg",
                        "posz.jpg", "negz.jpg", SkyBox_tu);

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

        glActiveTexture(GL_TEXTURE0 + ShadowMap_tu);
        glBindTexture(GL_TEXTURE_2D, ShadowMap.DepthBuffer.id_depth);

    /* Render loop */
    for (int i = 0; glfwWindowShouldClose(window) == 0; i++)
    {
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        gui(r_LightViewline);


        auto frame_viewMatrix = viewManipulator.apply_to_view(viewMatrix);

        RenderEnviroment(skybox_shader, r_skyBox, frame_viewMatrix);


        glm::mat4 lightViewMatrix = CalculateLightViewMatrix(r_LightViewline);
        ShadowMap.set_projection(lightViewMatrix, distance_light, box3(2.0));
        auto lightView = ShadowMap.light_matrix();


        RenderToShadowMapTexture(depth_shader, frame_viewMatrix, lightDirec, loadedModel, r_terrain, lightView);

        glViewport(0, 0, 1000, 800);

        SetShadowParamether(BlinnPhong_shadowMap_shader, lightView);



        RenderTerrain(BlinnPhong_shadowMap_shader, r_terrain, frame_viewMatrix, lightDirec);
        RenderCarModel(BlinnPhong_shadowMap_shader, loadedModel, frame_viewMatrix, lightDirec);


        RenderDebug(flatAlpha_shader, r_plane, r_frame, r_Lightline, r_LightViewline, frame_viewMatrix);


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
