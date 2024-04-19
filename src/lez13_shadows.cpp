#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
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
#include <ShadowMap.h>


/*
GLM library for math  https://github.com/g-truc/glm
it's a header-only library. You can just copy the folder glm into 3dparty
and set the path properly.
*/
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_access.hpp>

/* light direction NumberOfIndices world space*/
glm::vec4 Ldir_13;

/* projector */
float depth_bias;
float distance_light;


ShadowMap Lproj;


/* trackballs for controlloing the scene (0) or the light direction (1) */
trackball trackballs[2];

/* which trackball is currently used */
int curr_tb_13;

/* projection matrix*/
glm::mat4 proj_13;

/* view matrix */
glm::mat4 view_13;

/* matrix stack*/
matrix_stack stack;

/* a frame buffer object for the offline rendering*/
frame_buffer_object fbo, fbo_blur;


/* Program Shaders used */
Shader depth_shader, shadow_shader, flat_shader, fsq_shader, blur_shader;

/* implementation of view controller */
/* azimuthal and elevation angle*/
view_manipulator viewManipulator;


/* callback function called when the mouse is moving */
static void cursor_position_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (ImGui::GetIO().WantCaptureMouse) return;
    if (curr_tb_13 < 2)
        trackballs[curr_tb_13].mouse_move(proj_13, view_13, xpos, ypos);
    else
        viewManipulator.mouse_move(xpos, ypos);
}

/* callback function called when a mouse button is pressed */
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (ImGui::GetIO().WantCaptureMouse) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        if (curr_tb_13 < 2)
            trackballs[curr_tb_13].mouse_press(proj_13, view_13, xpos, ypos);
        else
            viewManipulator.mouse_press(xpos, ypos);
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        if (curr_tb_13 < 2)
            trackballs[curr_tb_13].mouse_release();
        else
            viewManipulator.mouse_release();
    }
}

/* callback function called when a mouse wheel is rotated */
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    if (ImGui::GetIO().WantCaptureMouse) return;

    if (curr_tb_13 == 0)
        trackballs[0].mouse_scroll(xoffset, yoffset);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    /* every time any key is presse it switch from controlling trackball trackball[0] to trackball[1] and viceversa */
    if (action == GLFW_PRESS)
        curr_tb_13 = 1 - curr_tb_13;
}

void print_info() {}

/* which algorithm to use */
static int shadowMap_mode = 0;

/* paramters of the VSM (it should be 0.5) */
static float k_plane_approx = 0.5;

void gui()
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
    if (ImGui::BeginMenu("parameters"))
    {
        bool redo_fbo = false;
        const char *items[] = {"32", "64", "128", "256", "512", "1024", "2048", "4096"};
        static int item_current = 1;
        int xi, yi;
        if (ImGui::ListBox("sm width", &xi, items, IM_ARRAYSIZE(items), 8))
            if (Lproj.SizeW != 1 << (5 + xi))
            {
                Lproj.SizeW = 1 << (5 + xi);
                redo_fbo = true;
            }
        if (ImGui::ListBox("sm height", &yi, items, IM_ARRAYSIZE(items), 8))
            if (Lproj.SizeH != 1 << (5 + yi))
            {
                Lproj.SizeH = 1 << (5 + yi);
                redo_fbo = true;
            }
        if (ImGui::SliderFloat("distance", &distance_light, 2.f, 100.f))
            Lproj.set_projection(glm::lookAt(glm::vec3(0, distance_light, 0.f), glm::vec3(0.f, 0.f, 0.f),
                                             glm::vec3(0.f, 0.f, -1.f)) * glm::inverse(trackballs[1].matrix()),
                                 distance_light, box3(1.0));
        ImGui::SliderFloat("  plane approx", &k_plane_approx, 0.0, 1.0);
        if (redo_fbo)
        {
            fbo.remove();
            fbo.create(Lproj.SizeW, Lproj.SizeH, true);
            fbo_blur.remove();
            fbo_blur.create(Lproj.SizeW, Lproj.SizeH, true);
        }

        ImGui::InputFloat("depth bias", &depth_bias);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Trackball"))
    {
        if (ImGui::Selectable("control scene", curr_tb_13 == 0)) curr_tb_13 = 0;
        if (ImGui::Selectable("control light", curr_tb_13 == 1)) curr_tb_13 = 1;
        if (ImGui::Selectable("control view", curr_tb_13 == 2)) curr_tb_13 = 2;

        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}

void draw_torus(Shader &sh, Renderable r_torus)
{
    glUniformMatrix4fv(sh["uT"], 1, GL_FALSE, &stack.peak()[0][0]);
    r_torus.SetAsCurrentObjectToRender();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_torus.ind);
    glDrawElements(GL_TRIANGLES, r_torus.NumberOfIndices, GL_UNSIGNED_INT, 0);
}

void draw_plane(Shader &sh, Renderable r_plane)
{
    glUniformMatrix4fv(sh["uT"], 1, GL_FALSE, &stack.peak()[0][0]);
    r_plane.SetAsCurrentObjectToRender();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_plane.ind);
    glDrawElements(GL_TRIANGLES, r_plane.NumberOfIndices, GL_UNSIGNED_INT, 0);
}


void draw_pole(Shader &sh, Renderable r_sphere)
{
    r_sphere.SetAsCurrentObjectToRender();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_sphere.elements[0].ind);
    glDrawElements(r_sphere.elements[0].element_type, r_sphere.elements[0].vertexCount, GL_UNSIGNED_INT, 0);
}

void draw_sphere(Shader &sh, Renderable r_sphere)
{
    glUniformMatrix4fv(sh["uT"], 1, GL_FALSE, &stack.peak()[0][0]);
    r_sphere.SetAsCurrentObjectToRender();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_sphere.ind);
    glDrawElements(GL_TRIANGLES, r_sphere.NumberOfIndices, GL_UNSIGNED_INT, 0);
}

void draw_cube(Shader &sh, Renderable r_cube)
{
    glUniformMatrix4fv(sh["uT"], 1, GL_FALSE, &stack.peak()[0][0]);
    r_cube.SetAsCurrentObjectToRender();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_cube.ind);
    glDrawElements(GL_TRIANGLES, r_cube.NumberOfIndices, GL_UNSIGNED_INT, 0);
}

void draw_scene(Shader &sh, Renderable r_plane, Renderable r_torus, Renderable r_cube)
{
    if (sh["uDiffuseColor"])
        glUniform3f(sh["uDiffuseColor"], 0.6, 0.6, 0.6);
    stack.pushLastElement();
    stack.multiply(glm::scale(glm::mat4(1.f), glm::vec3(4.0, 4.0, 4.0)));
    draw_plane(sh, r_plane);
    stack.pop();

    if (sh["uDiffuseColor"])
        glUniform3f(sh["uDiffuseColor"], 0.0, 0.4, 0.5);
    stack.pushLastElement();
    stack.multiply(glm::translate(glm::mat4(1.f), glm::vec3(-0.6, 0.3, 0.0)));
    stack.multiply(glm::scale(glm::mat4(1.f), glm::vec3(0.2, 0.2, 0.2)));
    draw_plane(sh, r_plane);
    stack.pop();

    // DrawTriangleElements sphere
    //stack.pushLastElement();
    //stack.multiply(glm::translate(glm::mat4(1.f), glm::vec3(0.0, 0.5, 0.0)));
    //stack.multiply(glm::scale(glm::mat4(1.f), glm::vec3(0.5, 0.5, 0.5)));
    //draw_sphere(sh);
    //stack.pop();

    // DrawTriangleElements pole
    stack.pushLastElement();
    stack.multiply(glm::translate(glm::mat4(1.f), glm::vec3(0.0, 0.5, 0.0)));
    stack.multiply(glm::scale(glm::mat4(1.f), glm::vec3(0.1, 0.5, 0.1)));
    glUniformMatrix4fv(sh["uT"], 1, GL_FALSE, &stack.peak()[0][0]);
    //draw_sphere(sh);
    draw_cube(sh, r_cube);
    stack.pop();

    // torus
    stack.pushLastElement();
    stack.multiply(glm::translate(glm::mat4(1.f), glm::vec3(1.0, 0.5, 0.0)));
    stack.multiply(glm::scale(glm::mat4(1.f), glm::vec3(0.2, 0.2, 0.2)));
    draw_torus(sh, r_torus);
    stack.pop();
}

void draw_full_screen_quad(Renderable r_quad)
{
    r_quad.SetAsCurrentObjectToRender();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void draw_texture(GLint tex_id, Renderable r_quad)
{
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


void blur_texture(GLint tex_id, Renderable r_quad)
{
    GLint at;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &at);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    CheckGLErrors(__LINE__, __FILE__, true);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_blur.id_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_blur.id_texture, 0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glUseProgram(blur_shader.Program);
    glUniform2f(blur_shader["uBlur"], 0.0, 1.f / fbo_blur.h);
    glUniform1i(blur_shader["uTexture"], 3);
    draw_full_screen_quad(r_quad);
    CheckGLErrors(__LINE__, __FILE__, true);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_id, 0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D, fbo_blur.id_texture);
    glUniform2f(blur_shader["uBlur"], 1.f / fbo_blur.w, 0.0);
    draw_full_screen_quad(r_quad);
    CheckGLErrors(__LINE__, __FILE__, true);

    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(at);
}

int lez13(void)
{
    GLFWwindow *window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(1000, 800, "code_13_shadows", NULL, NULL);
    if (!window)
    {
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
    depth_shader.create_program(shaders_path + "depth_map.vert", shaders_path + "depth_map.frag");
    shadow_shader.create_program(shaders_path + "shadow_mapping.vert", shaders_path + "shadow_mapping.frag");
    fsq_shader.create_program(shaders_path + "fsq.vert", shaders_path + "fsq.frag");
    flat_shader.create_program(shaders_path + "flat.vert", shaders_path + "flat.frag");
    blur_shader.create_program(shaders_path + "fsq.vert", shaders_path + "blur.frag");

    /* Set the uT matrix to Identity */
    glUseProgram(depth_shader.Program);
    glUniformMatrix4fv(depth_shader["uT"], 1, GL_FALSE, &glm::mat4(1.0)[0][0]);
    glUseProgram(shadow_shader.Program);
    glUniformMatrix4fv(shadow_shader["uT"], 1, GL_FALSE, &glm::mat4(1.0)[0][0]);
    glUseProgram(flat_shader.Program);
    glUniformMatrix4fv(flat_shader["uT"], 1, GL_FALSE, &glm::mat4(1.0)[0][0]);
    glUseProgram(0);
    CheckGLErrors(__LINE__, __FILE__, true);
    /* create a  long line*/
    auto r_line = shape_maker::line(100.f);

    /* create 3 lines showing the reference frame*/
    auto r_frame = shape_maker::frame(4.0);

    /* create a rectangle*/
    Renderable r_plane;
    shape s_plane;
    shape_maker::rectangle(s_plane, 1, 1);
    s_plane.ToRenderable(r_plane);

    /* create a torus */
    Renderable r_torus;
    shape s_torus;
    shape_maker::torus(s_torus, 0.5, 2.0, 50, 50);
    s_torus.ToRenderable(r_torus);
    CheckGLErrors(__LINE__, __FILE__, true);

    /* create a cube */
    Renderable r_cube;
    shape s_cube;
    shape_maker::cube(s_cube);
    s_cube.compute_edge_indices_from_indices();
    s_cube.ToRenderable(r_cube);

    /* create a sphere */
    auto r_sphere = shape_maker::sphere();

    /* create a quad with size 2 centered to the origin and on the XY pane */
    /* this is a quad used for the "full screen quad" rendering */
    auto r_quad = shape_maker::quad();

    /* initial light direction */
    Ldir_13 = glm::vec4(0.0, 1.0, 0.0, 0.0);

    /* light projection */
    Lproj.SizeW = 512;
    Lproj.SizeH = 512;
    depth_bias = 0;
    distance_light = 2;
    k_plane_approx = 0.5;

    Lproj.view_matrix = glm::lookAt(glm::vec3(0, distance_light, 0.f), glm::vec3(0.f, 0.f, 0.f),
                                    glm::vec3(0.f, 0.f, -1.f));

    /* Transformation to setup the point of view on the scene */
    proj_13 = glm::frustum(-1.f, 1.f, -0.8f, 0.8f, 2.f, 100.f);
    view_13 = glm::lookAt(glm::vec3(0, 3, 4.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));

    glUseProgram(depth_shader.Program);
    glUniformMatrix4fv(depth_shader["uLightMatrix"], 1, GL_FALSE, &Lproj.light_matrix()[0][0]);
    glUniformMatrix4fv(depth_shader["uT"], 1, GL_FALSE, &glm::mat4(1.f)[0][0]);
    glUseProgram(0);
    CheckGLErrors(__LINE__, __FILE__, true);

    glUseProgram(shadow_shader.Program);
    glUniformMatrix4fv(shadow_shader["uP"], 1, GL_FALSE, &proj_13[0][0]);
    glUniformMatrix4fv(shadow_shader["uV"], 1, GL_FALSE, &view_13[0][0]);
    glUniformMatrix4fv(shadow_shader["uLightMatrix"], 1, GL_FALSE, &Lproj.light_matrix()[0][0]);
    glUniform1i(shadow_shader["uShadowMap"], 0);
    glUniform2i(shadow_shader["uShadowMapSize"], Lproj.SizeW, Lproj.SizeH);
    glUseProgram(0);
    CheckGLErrors(__LINE__, __FILE__, true);

    glUseProgram(flat_shader.Program);
    glUniformMatrix4fv(flat_shader["uP"], 1, GL_FALSE, &proj_13[0][0]);
    glUniformMatrix4fv(flat_shader["uV"], 1, GL_FALSE, &view_13[0][0]);
    glUniform3f(flat_shader["uColor"], 1.0, 1.0, 1.0);
    glUseProgram(0);
    glEnable(GL_DEPTH_TEST);
    CheckGLErrors(__LINE__, __FILE__, true);

    print_info();

    /* set the trackball position */
    trackballs[0].set_center_radius(glm::vec3(0, 0, 0), 2.f);
    trackballs[1].set_center_radius(glm::vec3(0, 0, 0), 2.f);
    viewManipulator.reset();
    curr_tb_13 = 0;

    /* define the viewport  */
    glViewport(0, 0, 1000, 800);

    CheckGLErrors(__LINE__, __FILE__, true);
    fbo.create(Lproj.SizeW, Lproj.SizeH, true);
    fbo_blur.create(Lproj.SizeW, Lproj.SizeH, true);


    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        glClearColor(1.0, 1.0, 1.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        gui();

        Ldir_13 = glm::vec4(0.f, 1.f, 0.f, 0.f);
        /* rotate the view accordingly to view_rot*/
        glm::mat4 curr_view = viewManipulator.apply_to_view(view_13);

        auto lightTrackball = trackballs[1].matrix();
        auto worldTrackball = trackballs[0].matrix();

        /* light direction transformed by the trackball trackball[1]*/
        glm::vec4 curr_Ldir = lightTrackball * Ldir_13;

        stack.pushLastElement();
        stack.multiply(worldTrackball);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo.id_fbo);
        glViewport(0, 0, Lproj.SizeW, Lproj.SizeH);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        glUseProgram(depth_shader.Program);

        auto lightViewMatrix = glm::lookAt(glm::vec3(0, distance_light, 0.f), glm::vec3(0.f, 0.f, 0.f),
                                           glm::vec3(0.f, 0.f, -1.f)) * glm::inverse(lightTrackball);
        Lproj.set_projection(lightViewMatrix, distance_light, box3(2.0));

        glUniformMatrix4fv(depth_shader["uLightMatrix"], 1, GL_FALSE, &Lproj.light_matrix()[0][0]);
        glUniformMatrix4fv(depth_shader["uT"], 1, GL_FALSE, &stack.peak()[0][0]);
        glUniform1f(depth_shader["uPlaneApprox"], k_plane_approx);


        if (shadowMap_mode == 4 || shadowMap_mode == 5)
        {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
        }

        draw_scene(depth_shader, r_plane, r_torus, r_cube);

        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (shadowMap_mode == 6)
        {
            blur_texture(fbo.id_texture, r_quad);
        }

        glViewport(0, 0, 1000, 800);

        glUseProgram(shadow_shader.Program);
        glUniformMatrix4fv(shadow_shader["uLightMatrix"], 1, GL_FALSE, &Lproj.light_matrix()[0][0]);
        glUniformMatrix4fv(shadow_shader["uV"], 1, GL_FALSE, &curr_view[0][0]);
        glUniformMatrix4fv(shadow_shader["uT"], 1, GL_FALSE, &stack.peak()[0][0]);
        glUniform1fv(shadow_shader["uBias"], 1, &depth_bias);
        glUniform2i(shadow_shader["uShadowMapSize"], Lproj.SizeW, Lproj.SizeH);
        glUniform1i(shadow_shader["uRenderMode"], shadowMap_mode);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fbo.id_texture);

        draw_scene(shadow_shader, r_plane, r_torus, r_cube);

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


        r_cube.SetAsCurrentObjectToRender();
        stack.pushLastElement();
        stack.multiply(glm::inverse(Lproj.light_matrix()));
        glUseProgram(flat_shader.Program);
        glUniform3f(flat_shader["uColor"], 0.0, 0.0, 1.0);
        glUniformMatrix4fv(flat_shader["uT"], 1, GL_FALSE, &stack.peak()[0][0]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_cube.elements[1].ind);
        glDrawElements(r_cube.elements[1].element_type, r_cube.elements[1].vertexCount, GL_UNSIGNED_INT, 0);
        stack.pop();

        // render the light direction
        stack.pushLastElement();
        stack.multiply(lightTrackball);

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
