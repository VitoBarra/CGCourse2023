#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <iostream>
#include <random>
#include <direct.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "source.h"
#include "Renderable.h"
#include "debugging.h"
#include "shaders.h"
#include "simple_shapes.h"
#include "matrix_stack.h"
#include "trackball.h"
#include "view_manipulator.h"
#include "frame_buffer_object.h"

#include "obj_loader.h"

/*
GLM library for math  https://github.com/g-truc/glm
it's a header-only library. You can just copy the folder glm into 3dparty
and set the path properly.
*/
#include <glm/glm.hpp>
#include <glm/ext.hpp>

/* light direction NumberOfIndices world space*/
glm::vec4 Ldir_9;

/* projector */
float depth_bias;
float distance_light;

int g_buffer_size_x, g_buffer_size_y;
float radius, depthscale;

/* trackballs for controlloing the scene (0) or the light direction (1) */
trackball trackball[2];

/* which trackball is currently used */
int curr_tb_9;

/* projection matrix*/
glm::mat4 proj_9;

/* view matrix */
glm::mat4 view_9;

/* matrix stack*/
matrix_stack stack;

/* a frame buffer object for the offline rendering*/
frame_buffer_object fbo, fbo_ao, fbo_blur;

/* object that will be rendered NumberOfIndices this scene*/
std::vector<Renderable> r_knife;


/* Program Shaders used */
Shader ao_shader, g_buffer_shader, final_shader, flat_shader, fsq_shader, blur_shader;

/* implementation of view controller */

/* azimuthal and elevation angle*/
view_manipulator view_man;


/* callback function called when the mouse is moving */
static void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    if (curr_tb_9 < 2)
        trackball[curr_tb_9].mouse_move(proj_9, view_9, xpos, ypos);
    else
        view_man.mouse_move(xpos, ypos);
}

/* callback function called when a mouse button is pressed */
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    if (ImGui::GetIO().WantCaptureMouse) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        if (curr_tb_9 < 2)
            trackball[curr_tb_9].mouse_press(proj_9, view_9, xpos, ypos);
        else
            view_man.mouse_press(xpos, ypos);
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        if (curr_tb_9 < 2)
            trackball[curr_tb_9].mouse_release();
        else
            view_man.mouse_release();
    }
}

/* callback function called when a mouse wheel is rotated */
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    if (ImGui::GetIO().WantCaptureMouse) return;

    if (curr_tb_9 == 0)
        trackball[0].mouse_scroll(xoffset, yoffset);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    /* every time any key is presse it switch from controlling trackball trackball[0] to trackball[1] and viceversa */
    if (action == GLFW_PRESS)
        curr_tb_9 = 1 - curr_tb_9;

}

void print_info() {
}


static bool use_ao = 0;
static int fps;

void gui_setup() {
    ImGui::BeginMainMenuBar();

    ImGui::Text((std::string("FPS: ") + std::to_string(fps)).c_str());
    if (ImGui::BeginMenu("parameters")) {
        bool redo_fbo = false;
        const char *items[] = {"32", "64", "128", "256", "512", "1024", "2048", "4096"};
        static int item_current = 1;
        int xi, yi;
        ImGui::Checkbox("use AO", &use_ao);
        if (ImGui::ListBox("sm width", &xi, items, IM_ARRAYSIZE(items), 8))
            if (g_buffer_size_x != 1 << (5 + xi)) {
                g_buffer_size_x = 1 << (5 + xi);
                redo_fbo = true;
            }
        if (ImGui::ListBox("sm height", &yi, items, IM_ARRAYSIZE(items), 8))
            if (g_buffer_size_y != 1 << (5 + yi)) {
                g_buffer_size_y = 1 << (5 + yi);
                redo_fbo = true;
            }
        if (redo_fbo) {
            fbo.remove();
            fbo.create(g_buffer_size_x, g_buffer_size_y, true);
            fbo_ao.remove();
            fbo_ao.create(g_buffer_size_x, g_buffer_size_y, true);
            fbo_blur.remove();
            fbo_blur.create(g_buffer_size_x, g_buffer_size_y, true);
        }

        ImGui::SliderFloat("radius", &radius, 0.0, 50.f);
        ImGui::SliderFloat("depthscale", &depthscale, 0.001, 0.1);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Trackball")) {
        if (ImGui::Selectable("control scene", curr_tb_9 == 0)) curr_tb_9 = 0;
        if (ImGui::Selectable("control light", curr_tb_9 == 1)) curr_tb_9 = 1;
        if (ImGui::Selectable("control view", curr_tb_9 == 2)) curr_tb_9 = 2;

        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}


void draw_scene(Shader &sh) {
    stack.pushLastElement();
    float sf = 1.f / r_knife[0].bbox.diagonal();
    glm::vec3 c = r_knife[0].bbox.center();
    stack.multiply(glm::scale(glm::mat4(1.f), glm::vec3(sf, sf, sf)));
    stack.multiply(glm::translate(glm::mat4(1.f), -c));
    glUniformMatrix4fv(sh["uT"], 1, GL_FALSE, &stack.peak()[0][0]);
    r_knife[0].SetAsCurrentObjectToRender();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_knife[0].elements[0].ind);
    glDrawElements(r_knife[0].elements[0].element_type, r_knife[0].elements[0].vertexCount, GL_UNSIGNED_INT, 0);
    stack.pop();

}

void draw_full_screen_quad(Renderable r_quad) {
    r_quad.SetAsCurrentObjectToRender();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void draw_texture(GLint tex_id, Renderable r_quad) {
    GLint at;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &at);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glUseProgram(fsq_shader.Program);
    glUniform1i(fsq_shader["uTexture"], 3);
    draw_full_screen_quad(r_quad);
    glUseProgram(0);
    glActiveTexture(at);
}

void ssaoKernel() {
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
    std::default_random_engine generator;
    std::vector<glm::vec3> ssaoKernel;
    std::vector<float> _;
    srand(clock());
    for (unsigned int i = 0; i < 64; ++i) {
        glm::vec3 sample(
                randomFloats(generator) * 2.0 - 1.0,
                randomFloats(generator) * 2.0 - 1.0,
                randomFloats(generator) * 2.0 - 1.0
        );
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        ssaoKernel.push_back(sample);
    }

    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++) {
        glm::vec3 noise(
                randomFloats(generator) * 2.0 - 1.0,
                randomFloats(generator) * 2.0 - 1.0,
                0.0f);
        ssaoNoise.push_back(noise);
    }

    unsigned int noiseTexture;
    glGenTextures(1, &noiseTexture);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glUseProgram(ao_shader.Program);
    ao_shader.RegisterUniformVariable("uSamples");
    ao_shader.RegisterUniformVariable("uNoise");
    glUniform3fv(ao_shader["uSamples"], ssaoKernel.size(), &ssaoKernel[0].x);
    glUniform1i(ao_shader["uNoise"], 4);
    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
}

void blur_texture(GLint tex_id, Renderable r_quad) {
    GLint at;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &at);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    CheckGLErrors(__LINE__, __FILE__, true);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_blur.id_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_blur.id_tex, 0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glUseProgram(blur_shader.Program);
    glUniform2f(blur_shader["uBlur"], 0.0, 1.f / fbo_blur.h);
    glUniform1i(blur_shader["uTexture"], 3);
    draw_full_screen_quad(r_quad);
    CheckGLErrors(__LINE__, __FILE__, true);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_id, 0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D, fbo_blur.id_tex);
    glUniform2f(blur_shader["uBlur"], 1.f / fbo_blur.w, 0.0);
    draw_full_screen_quad(r_quad);
    CheckGLErrors(__LINE__, __FILE__, true);

    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(at);
}

int lez14(void) {
    GLFWwindow *window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    glfwWindowHint(GLFW_SAMPLES, 4);

    window = glfwCreateWindow(1000, 800, "code_14_ambient_obscurance", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    /* declare the callback functions on mouse events */
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    glewInit();


    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplOpenGL3_Init();
    ImGui_ImplGlfw_InitForOpenGL(window, true);

    printout_opengl_glsl_info();

    CheckGLErrors(__LINE__, __FILE__, true);

    /* load the Shaders */
    std::string shaders_path = "../Shaders/";
    ao_shader.create_program(shaders_path + "fsq.vert", shaders_path + "ao.frag");
    ssaoKernel();
    g_buffer_shader.create_program(shaders_path + "g_buffer.vert", shaders_path + "g_buffer.frag");
    final_shader.create_program(shaders_path + "fsq.vert", shaders_path + "final.frag");

    fsq_shader.create_program(shaders_path + "fsq.vert", shaders_path + "fsq.frag");
    flat_shader.create_program(shaders_path + "flat.vert", shaders_path + "flat.frag");
    blur_shader.create_program(shaders_path + "fsq.vert", shaders_path + "blur.frag");

    /* create a  long line*/
    auto r_line = shape_maker::line(100.f);

    /* create 3 lines showing the reference frame*/
    auto r_frame = shape_maker::frame(4.0);


    /* load the knife model */
    std::string models_path = "../Models/knife/";
    _chdir(models_path.c_str());

    load_obj(r_knife, "/.","/knife_50k.obj");

    /* create a quad with size 2 centered to the origin and on the XY pane */
    auto r_quad = shape_maker::quad();

    /* initial light direction */
    Ldir_9 = glm::vec4(0.0, 1.0, 0.0, 0.0);

    /* light projection */
    CheckGLErrors(__LINE__, __FILE__, true);
    g_buffer_size_x = 512;
    g_buffer_size_y = 512;
    CheckGLErrors(__LINE__, __FILE__, true);

    /* Transformation to setup the point of view on the scene */
    proj_9 = glm::frustum(-1.f, 1.f, -0.8f, 0.8f, 2.f, 15.f);
    view_9 = glm::lookAt(glm::vec3(0, 3, 4.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
    view_9 = glm::lookAt(glm::vec3(0, 0, 7.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));

    glUseProgram(g_buffer_shader.Program);
    glUniformMatrix4fv(g_buffer_shader["uP"], 1, GL_FALSE, &proj_9[0][0]);
    glUniformMatrix4fv(g_buffer_shader["uV"], 1, GL_FALSE, &view_9[0][0]);
    glUseProgram(0);

    CheckGLErrors(__LINE__, __FILE__, true);

    glUseProgram(flat_shader.Program);
    glUniformMatrix4fv(flat_shader["uP"], 1, GL_FALSE, &proj_9[0][0]);
    glUniformMatrix4fv(flat_shader["uV"], 1, GL_FALSE, &view_9[0][0]);
    glUniform3f(flat_shader["uColor"], 1.0, 1.0, 1.0);
    glUseProgram(0);
    glEnable(GL_DEPTH_TEST);
    CheckGLErrors(__LINE__, __FILE__, true);

    print_info();

    /* set the trackball position */
    trackball[0].set_center_radius(glm::vec3(0, 0, 0), 2.f);
    trackball[1].set_center_radius(glm::vec3(0, 0, 0), 2.f);
    view_man.reset();
    curr_tb_9 = 0;

    /* define the viewport  */
    glViewport(0, 0, 1000, 800);

    CheckGLErrors(__LINE__, __FILE__, true);
    fbo.create(g_buffer_size_x, g_buffer_size_y, true);
    fbo_blur.create(g_buffer_size_x, g_buffer_size_y, true);
    fbo_ao.create(g_buffer_size_x, g_buffer_size_y, true);


    texture ao_tex;
    ao_tex.create(g_buffer_size_x, g_buffer_size_y, 3);
    /* Loop until the user closes the window */
    double last_time = glfwGetTime();
    int n_frames = 0;

    while (!glfwWindowShouldClose(window)) {
        double current_time = glfwGetTime();
        n_frames++;
        if (current_time - last_time >= 1.0) {
            fps = n_frames;
            n_frames = 0;
            last_time += 1.0;
        }

        /* Render here */
        glClearColor(1.0, 0.6, 0.7, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        gui_setup();

        /* rotate the view accordingly to view_rot*/
        glm::mat4 curr_view = view_man.apply_to_view(view_9);

        /* light direction transformed by the trackball trackball[1]*/
        glm::vec4 curr_Ldir = trackball[1].matrix() * Ldir_9;

        stack.pushLastElement();
        stack.multiply(trackball[0].matrix());

        glBindFramebuffer(GL_FRAMEBUFFER, fbo.id_fbo);

        glBindTexture(GL_TEXTURE_2D, fbo.id_tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, g_buffer_size_x, g_buffer_size_y, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
        CheckGLErrors(__LINE__, __FILE__, true);

        GLenum bufferlist[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, bufferlist);
        glViewport(0, 0, g_buffer_size_x, g_buffer_size_y);
//		glViewport(0, 0, 1000, 800);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        CheckGLErrors(__LINE__, __FILE__, true);

        glUseProgram(g_buffer_shader.Program);
        glUniformMatrix4fv(g_buffer_shader["uV"], 1, GL_FALSE, &curr_view[0][0]);
        glUniformMatrix4fv(g_buffer_shader["uT"], 1, GL_FALSE, &stack.peak()[0][0]);

        draw_scene(g_buffer_shader);
        CheckGLErrors(__LINE__, __FILE__, true);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

//		draw_texture(fbo.id_tex1);
//goto swapbuffers;
//		glDrawBuffers(1, bufferlist);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_ao.id_fbo);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        glUseProgram(ao_shader.Program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fbo.id_tex);

        glUniform1i(ao_shader["uDepthMap"], 0);
        glUniform1f(ao_shader["uRadius"], radius);
        glUniform1f(ao_shader["uDepthScale"], depthscale);
        glUniform2f(ao_shader["uSize"], g_buffer_size_x, g_buffer_size_y);
        glUniform2f(ao_shader["uRND"], 2.0, 3.0);

        draw_full_screen_quad(r_quad);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        blur_texture(fbo_ao.id_tex, r_quad);
        blur_texture(fbo_ao.id_tex, r_quad);
        blur_texture(fbo_ao.id_tex, r_quad);

        glViewport(0, 0, 1000, 800);

// draw_texture(fbo_ao.id_tex);
// goto swapbuffers;

//		glViewport(0, 0, 1000, 800);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        glUseProgram(final_shader.Program);
        glUniform1i(final_shader["uUseAO"], use_ao);
        glUniform1i(final_shader["uNormalMap"], 0);
        glUniform1i(final_shader["uAOMap"], 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fbo.id_tex1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, fbo_ao.id_tex);

        glm::vec4 LVS = curr_view * curr_Ldir;
        glUniform3f(final_shader["uLVS"], LVS.x, LVS.y, LVS.z);

        draw_full_screen_quad(r_quad);


        // render the reference frame
        glUseProgram(flat_shader.Program);
        glUniformMatrix4fv(flat_shader["uV"], 1, GL_FALSE, &curr_view[0][0]);
        glUniformMatrix4fv(flat_shader["uT"], 1, GL_FALSE, &stack.peak()[0][0]);
        glUniform3f(flat_shader["uColor"], -1.0, 1.0, 1.0);

        r_frame.SetAsCurrentObjectToRender();
        glDrawArrays(GL_LINES, 0, 6);
        glUseProgram(0);

        CheckGLErrors(__LINE__, __FILE__, true);
        stack.pop();

        // render the light direction
        stack.pushLastElement();
        stack.multiply(trackball[1].matrix());

        glUseProgram(flat_shader.Program);
        glUniformMatrix4fv(flat_shader["uT"], 1, GL_FALSE, &stack.peak()[0][0]);
        glUniform3f(flat_shader["uColor"], 1.0, 1.0, 1.0);
        r_line.SetAsCurrentObjectToRender();
        glDrawArrays(GL_LINES, 0, 2);
        glUseProgram(0);
        swapbuffers:
        stack.pop();

        // glDisable(GL_DEPTH_TEST);
        // glViewport(0, 0, 512, 512);
        //	blur_texture(Lproj.tex.id);
        // draw_texture(Lproj.tex.id);
        //glEnable(GL_DEPTH_TEST);
        // draw_texture(fbo.id_tex);


        CheckGLErrors(__LINE__, __FILE__);


        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // show the shadow map
        //glViewport(0, 0, 200, 200);
        //glDisable(GL_DEPTH_TEST);
        //draw_texture(fbo.id_tex);
        //glEnable(GL_DEPTH_TEST);
        //glViewport(0, 0, 1000, 800);



        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}