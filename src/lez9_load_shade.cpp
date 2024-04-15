#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <iostream>
#include <algorithm>

#include "source.h"
#include "Renderable.h"
#include "debugging.h"
#include "shaders.h"
#include "simple_shapes.h"
#include "matrix_stack.h"
#include "trackball.h"

#include "obj_loader.h"


/*
GLM library for math  https://github.com/g-truc/glm
it's a header-only library. You can just copy the folder glm into 3dparty
and set the path properly.
*/
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>

/* light direction NumberOfIndices world space*/
glm::vec4 Ldir_14;

trackball trackball_9[2];
int curr_tb_9;

/* projection matrix*/
glm::mat4 proj_9;

/* view matrix */
glm::mat4 view_9;


void draw_line_9(glm::vec4 l) {
    glColor3f(1, 1, 1);
    glBegin(GL_LINES);
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(100 * l.x, 100 * l.y, 100 * l.z);
    glEnd();
}

/* callback function called when the mouse is moving */
static void cursor_position_callback_9(GLFWwindow *window, double xpos, double ypos) {
    trackball_9[curr_tb_9].mouse_move(proj_9, view_9, xpos, ypos);
}

/* callback function called when a mouse button is pressed */
void mouse_button_callback_9(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        trackball_9[curr_tb_9].mouse_press(proj_9, view_9, xpos, ypos);
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        trackball_9[curr_tb_9].mouse_release();
    }
}

/* callback function called when a mouse wheel is rotated */
void scroll_callback_9(GLFWwindow *window, double xoffset, double yoffset) {
    if (curr_tb_9 == 0)
        trackball_9[0].mouse_scroll(xoffset, yoffset);
}

void key_callback_9(GLFWwindow *window, int key, int scancode, int action, int mods) {
    /* every time any key is presse it switch from controlling trackball trackball[0] to trackball[1] and viceversa */
    if (action == GLFW_PRESS)
        curr_tb_9 = 1 - curr_tb_9;

}

void print_info_9() {

    std::cout << "press left mouse button to control the trackball\n";
    std::cout << "press any key to switch between world and light control\n";
}

int lez9() {
    GLFWwindow *window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(1000, 800, "code_9_load_shade", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    /* declare the callback functions on mouse events */
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    glfwSetCursorPosCallback(window, cursor_position_callback_9);
    glfwSetMouseButtonCallback(window, mouse_button_callback_9);
    glfwSetScrollCallback(window, scroll_callback_9);
    glfwSetKeyCallback(window, key_callback_9);

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    glewInit();

    printout_opengl_glsl_info();


    /* load the Shaders */
    std::string shaders_path = "../Shaders/";
    Shader diffuse_shader;
    diffuse_shader.create_program(shaders_path + "DiffusiveColor.vert",
                                  shaders_path + "ColorWithAlpha.frag");
    diffuse_shader.RegisterUniformVariable("uP");
    diffuse_shader.RegisterUniformVariable("uV");
    diffuse_shader.RegisterUniformVariable("uT");
    diffuse_shader.RegisterUniformVariable("uDiffuseColor");
    diffuse_shader.RegisterUniformVariable("uLdir");
    check_shader(diffuse_shader.VertexShader);
    check_shader(diffuse_shader.FragmentShader);
    validate_shader_program(diffuse_shader.Program);

    Shader flat_shader;
    flat_shader.create_program(shaders_path + "DiffusiveColor.vert",
                               shaders_path + "flatAlpha.frag");
    flat_shader.RegisterUniformVariable("uP");
    flat_shader.RegisterUniformVariable("uV");
    flat_shader.RegisterUniformVariable("uT");
    flat_shader.RegisterUniformVariable("uColor");
    check_shader(flat_shader.VertexShader);
    check_shader(flat_shader.FragmentShader);
    validate_shader_program(flat_shader.Program);

    /* Set the uT matrix to Identity */
    glUseProgram(diffuse_shader.Program);
    glUniformMatrix4fv(diffuse_shader["uT"], 1, GL_FALSE, &glm::mat4(1.0)[0][0]);
    glUseProgram(flat_shader.Program);
    glUniformMatrix4fv(flat_shader["uT"], 1, GL_FALSE, &glm::mat4(1.0)[0][0]);
    glUseProgram(0);

    CheckGLErrors(__LINE__, __FILE__);

    /* create a  cube   centered at the origin with side 2*/
    auto r_cube = shape_maker::cube(0.5f, 0.3f, 0.0);
    std::vector<glm::vec3> cubes_pos;
    cubes_pos.push_back(glm::vec3(2, 0, 0));
    cubes_pos.push_back(glm::vec3(0, 2, 0));
    cubes_pos.push_back(glm::vec3(-2, 0, 0));
    cubes_pos.push_back(glm::vec3(0, -2, 0));


    /* create a  long line*/
    auto r_line = shape_maker::line(100.f);

    /* create a  sphere   centered at the origin with radius 1*/
    auto r_sphere = shape_maker::sphere();

    /* create 3 lines showing the reference frame*/
    auto r_frame = shape_maker::frame(4.0);

    /* crete a rectangle*/
    Renderable r_plane;
    shape s_plane;
    shape_maker::rectangle(s_plane, 10, 10);
    s_plane.compute_edge_indices_from_indices();
    s_plane.ToRenderable(r_plane);

    /* load from file */
    std::vector<Renderable> r_cb;
    load_obj(r_cb, "../Models/Datsun_280Z", "Datsun_280Z.obj");


    /* initial light direction */
    Ldir_14 = glm::vec4(0.0, 1.0, 0.0, 0.0);

    /* Transformation to setup the point of view on the scene */
    proj_9 = glm::frustum(-1.f, 1.f, -0.8f, 0.8f, 2.f, 20.f);
    view_9 = glm::lookAt(glm::vec3(0, 6, 8.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));


    glUseProgram(diffuse_shader.Program);
    glUniformMatrix4fv(diffuse_shader["uP"], 1, GL_FALSE, &proj_9[0][0]);
    glUniformMatrix4fv(diffuse_shader["uV"], 1, GL_FALSE, &view_9[0][0]);
    glUseProgram(0);

    glUseProgram(flat_shader.Program);
    glUniformMatrix4fv(flat_shader["uP"], 1, GL_FALSE, &proj_9[0][0]);
    glUniformMatrix4fv(flat_shader["uV"], 1, GL_FALSE, &view_9[0][0]);
    glUniform4f(flat_shader["uColor"], 1.0, 1.0, 1.0, 1.f);
    glUseProgram(0);
    glEnable(GL_DEPTH_TEST);

    system("CLS");
    print_info_9();


    matrix_stack stack;

    /* set the trackball position */
    trackball_9[0].set_center_radius(glm::vec3(0, 0, 0), 2.f);
    trackball_9[1].set_center_radius(glm::vec3(0, 0, 0), 2.f);
    curr_tb_9 = 0;

    /* define the viewport  */
    glViewport(0, 0, 1000, 800);

    /* avoid rendering back faces */
    // uncomment to see the plane disappear when rotating it
    // glEnable(GL_CULL_FACE);

    /* Loop until the user closes the window */
    while (glfwWindowShouldClose(window) == 0) {
        /* Render here */
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        CheckGLErrors(__LINE__, __FILE__);

        /* light direction transformed by the trackball trackball[1]*/
        glm::vec4 curr_Ldir = trackball_9[1].matrix() * Ldir_14;

        stack.pushLastElement();
        stack.multiply(trackball_9[0].matrix());

        /* show the plane NumberOfIndices flat-wire (filled triangles plus triangle contours) */
        // step 1: render the edges
        glUseProgram(flat_shader.Program);
        r_plane.SetAsCurrentObjectToRender();
        stack.pushLastElement();
        glUniformMatrix4fv(flat_shader["uT"], 1, GL_FALSE, &stack.peak()[0][0]);
        glUniform4f(flat_shader["uColor"], 1.0, 1.0, 1.0, 1.0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_plane.elements[1].ind);
        glDrawElements(GL_LINES, r_plane.elements[1].vertexCount, GL_UNSIGNED_INT, 0);

        //step 2: render the triangles
        glUseProgram(diffuse_shader.Program);

        // enable polygon offset functionality
        glEnable(GL_POLYGON_OFFSET_FILL);

        // set offset function
        glPolygonOffset(1.0, 1.0);

        glUniformMatrix4fv(diffuse_shader["uT"], 1, GL_FALSE, &stack.peak()[0][0]);

        glUniform3f(diffuse_shader["uDiffuseColor"], 0.8f, 0.8f, 0.8f);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_plane.ind);
        glDrawElements(GL_TRIANGLES, r_plane.NumberOfIndices, GL_UNSIGNED_INT, 0);

        // disable polygon offset
        glDisable(GL_POLYGON_OFFSET_FILL);
        stack.pop();
        //  end flat-wire rendering of the plane

        // render the reference frame
        glUseProgram(diffuse_shader.Program);
        glUniformMatrix4fv(diffuse_shader["uT"], 1, GL_FALSE, &stack.peak()[0][0]);
        // a negative x component is used to tell the Shader to use the vertex color as is (that is, no lighting is computed)
        glUniform3f(diffuse_shader["uDiffuseColor"], -1.0, 0.0, 1.0);
        r_frame.SetAsCurrentObjectToRender();
        glDrawArrays(GL_LINES, 0, 6);
        glUseProgram(0);

        glUseProgram(diffuse_shader.Program);
        glUniform4fv(diffuse_shader["uLdir"], 1, &curr_Ldir[0]);

        // uncomment to DrawTriangleElements the sphere
        //r_sphere.RegisterUniformVariable();
        //stack.pushLastElement();
        //stack.multiply(glm::scale(glm::mat4(1.f), glm::vec3(0.3, 0.3, 0.3)));
        //glDrawElements(r_sphere.elements[0].element_type, r_sphere.elements[0].count, GL_UNSIGNED_INT, 0);
        //stack.pop();

        /*render the loaded object.
        The object is made of several meshes (== objbects of type "Renderable")
        */
        if (!r_cb.empty()) {
            stack.pushLastElement();

            /*scale the object using the diagonal of the bounding box of the vertices position.
            This operation guarantees that the drawing will be inside the unit cube.*/
            float diag = r_cb[0].bbox.diagonal();
            stack.multiply(glm::scale(glm::mat4(1.f), glm::vec3(1.f / diag, 1.f / diag, 1.f / diag)));

            glUniformMatrix4fv(diffuse_shader["uT"], 1, GL_FALSE, &stack.peak()[0][0]);

            for (auto & is : r_cb) {
                is.SetAsCurrentObjectToRender();
                /* every Renderable object has its own material. Here just the diffuse color is used.
                ADD HERE CODE TO PASS OTHE MATERIAL PARAMETERS.
                */
                glUniform3fv(diffuse_shader["uDiffuseColor"], 1, &is.material.diffuse[0]);

                glDrawElements(is.elements[0].element_type, is.elements[0].vertexCount, GL_UNSIGNED_INT, 0);
            }
            stack.pop();
            glUseProgram(0);
        }

        stack.pop();

        r_line.SetAsCurrentObjectToRender();
        glUseProgram(flat_shader.Program);
        stack.pushLastElement();
        stack.multiply(trackball_9[1].matrix());

        glUniformMatrix4fv(flat_shader["uT"], 1, GL_FALSE, &stack.peak()[0][0]);

        glUniform4f(flat_shader["uColor"], 1.0, 1.0, 1.0, 1.0);

        glDrawArrays(GL_LINES, 0, 2);

        stack.pop();
        glUseProgram(0);

        CheckGLErrors(__LINE__, __FILE__);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}