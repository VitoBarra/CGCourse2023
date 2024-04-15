#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "shaders.h"
#include "simple_shapes.h"
#include "matrix_stack.h"

/*
GLM library for math  https://github.com/g-truc/glm
it's a header-only library. You can just copy the folder glm into 3dparty
and set the path properly.
*/
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>


/* projection matrix*/
glm::mat4 proj_14;

/* view matrix and view_frame*/
glm::mat4 view_14, view_frame;

/* a bool variable that indicates if we are currently rotating the trackball*/
bool is_trackball_dragged;

/* p0 and p1 points on the sphere */
glm::vec3 p0, p1;

/* matrix to transform the scene according to the trackball: rotation only*/
glm::mat4 trackball_matrix;

/* matrix to transform the scene according to the trackball: scaling only*/
glm::mat4 scaling_matrix;
float scaling_factor;


/* transform from viewpoert to window coordinates NumberOfIndices thee view reference frame */
glm::vec2 viewport_to_view(double pX, double pY) {
    glm::vec2 res;
    res.x = -1.f + ((float) pX / 1000) * (1.f - (-1.f));
    res.y = -0.8f + ((800 - (float) pY) / 800) * (0.8f - (-0.8f));
    return res;
}

/*
Computes the first intersection point between a ray and a sphere
o: origin of the ray
d: direction of the ray
c: center of the sphere
radius: radius of the sphere
*/
bool ray_sphere_intersection(glm::vec3 &int_point, glm::vec3 o, glm::vec3 d, glm::vec3 c, float radius) {
    glm::vec3 oc = o - c;
    float A = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
    float B = 2 * glm::dot(d, oc);
    float C = glm::dot(oc, oc) - radius * radius;

    float dis = B * B - 4 * A * C;

    if (dis > 0) {
        float t0 = (-B - sqrt(dis)) / (2 * A);
        float t1 = (-B + sqrt(dis)) / (2 * A);
        float t = std::min<float>(t0, t1);
        int_point = o + glm::vec3(t * d[0], t * d[1], t * d[2]);
        return true;
    }
    return false;
}

/* handles the intersection between the position under the mouse and the sphere.
*/
bool cursor_sphere_intersection(glm::vec3 &int_point, double xpos, double ypos) {
    glm::vec2 pos2 = viewport_to_view(xpos, ypos);

    glm::vec3 o = view_frame * glm::vec4(glm::vec3(0.f, 0.f, 0.f), 1.f);
    glm::vec3 d = view_frame * glm::vec4(glm::vec3(pos2, -2.f), 0.f);
    glm::vec3 c = view_frame * glm::vec4(glm::vec3(0.f, 0.f, -10.f), 1.f);

    bool hit = ray_sphere_intersection(int_point, o, d, c, 2.f);
    if (hit)
        int_point -= c;

    /* this was left to "return true" in class.. It was a gigantic bug with almost never any consequence, except while
    click near the silhouette of the sphere.*/
    return hit;
}

/* callback function called when the mouse is moving */
static void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
    if (!is_trackball_dragged)
        return;

    if (cursor_sphere_intersection(p1, xpos, ypos)) {
        glm::vec3 rotation_vector = glm::cross(glm::normalize(p0), glm::normalize(p1));

        /* avoid near null rotation axis*/
        if (glm::length(rotation_vector) > 0.01) {
            float alpha = glm::asin(glm::length(rotation_vector));
            glm::mat4 delta_rot = glm::rotate(glm::mat4(1.f), alpha, rotation_vector);
            trackball_matrix = delta_rot * trackball_matrix;

            /*p1 becomes the p0 value for the next movement */
            p0 = p1;
        }
    }
}

/* callback function called when a mouse button is pressed */
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        glm::vec3 int_point;

        if (mods & GLFW_MOD_CONTROL) {
            // from viewport to world space
            float depthvalue;
            glReadPixels((int) xpos, 800 - (int) ypos, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depthvalue);
            glm::vec4 ndc = glm::vec4(-1.f + xpos / 1000.f * 2, -1.f + (800 - ypos) / 800.f * 2.f,
                                      -1.f + depthvalue * 2.f, 1.f);
            glm::vec4 hit1 = glm::inverse(proj_14 * view_14) * ndc;
            hit1 /= hit1.w;
            std::cout << " hit point " << glm::to_string(hit1) << std::endl;

            // from viewport to world space with unProject
            glm::vec3 hit = glm::unProject(glm::vec3(xpos, 800 - ypos, depthvalue), view_14, proj_14,
                                           glm::vec4(0, 0, 1000, 800));
            std::cout << " hit point " << glm::to_string(hit) << std::endl;

            // read back the color from the color buffer and compute the index
            GLubyte colu[4];
            glReadPixels(xpos, 800 - ypos, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &colu[0]);
            std::cout << " rgba  " << (int) colu[0] << " " << (int) colu[1] << " " << (int) colu[2] << " "
                      << (int) colu[3] << std::endl;

            int id = colu[0] + (colu[1] << 8) + (colu[2] << 16);
            std::cout << "selected ID: " << id << std::endl;
        } else if (cursor_sphere_intersection(int_point, xpos, ypos)) {
            p0 = int_point;
            is_trackball_dragged = true;
        }
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        is_trackball_dragged = false;
}

/* callback function called when a mouse wheel is rotated */
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    scaling_factor *= (yoffset > 0) ? 1.1f : 0.97f;
    scaling_matrix = glm::scale(glm::mat4(1.f), glm::vec3(scaling_factor, scaling_factor, scaling_factor));
}

void print_info() {

    std::cout << "press left mouse button to control the trackball\n" << "keep Ctrl pressed for picking\n";
}

int lez8(void) {
    GLFWwindow *window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(1000, 800, "code_8_picking", NULL, NULL);
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

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    glewInit();

    printout_opengl_glsl_info();


    /* load the Shaders */
    std::string shaders_path = "../Shaders/";
    Shader basic_shader = *Shader::CreateShaderFromFile(shaders_path, "Basic.vert", "FlatShading.frag");
    basic_shader.RegisterUniformVariable("uP");
    basic_shader.RegisterUniformVariable("uV");
    basic_shader.RegisterUniformVariable("uT");
    basic_shader.RegisterUniformVariable("uColor");
    check_shader(basic_shader.VertexShader);
    check_shader(basic_shader.FragmentShader);
    validate_shader_program(basic_shader.Program);

    Shader flat_shader = *Shader::CreateShaderFromFile(shaders_path, "Basic.vert", "flatAlpha.frag");;
    flat_shader.RegisterUniformVariable("uP");
    flat_shader.RegisterUniformVariable("uV");
    flat_shader.RegisterUniformVariable("uT");
    flat_shader.RegisterUniformVariable("uColor");
    check_shader(flat_shader.VertexShader);
    check_shader(flat_shader.FragmentShader);
    validate_shader_program(flat_shader.Program);

    /* Set the uT matrix to Identity */
    glUseProgram(basic_shader.Program);
    glUniformMatrix4fv(basic_shader["uT"], 1, GL_FALSE, &glm::mat4(1.0)[0][0]);
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


    /* create a  sphere   centered at the origin with radius 1*/
    Renderable r_sphere = shape_maker::sphere();

    /* create 3 lines showing the reference frame*/
    auto r_frame = shape_maker::frame(4.0);

    /* crete a rectangle*/
    Renderable r_plane;
    shape s_plane;
    shape_maker::rectangle(s_plane, 10, 10);
    s_plane.compute_edge_indices_from_indices();
    s_plane.ToRenderable(r_plane);

    /* Transformation to setup the point of view on the scene */
    proj_14 = glm::frustum(-1.f, 1.f, -0.8f, 0.8f, 2.f, 20.f);
    view_14 = glm::lookAt(glm::vec3(0, 6, 8.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
    view_frame = glm::inverse(view_14);

    glUseProgram(basic_shader.Program);
    glUniformMatrix4fv(basic_shader["uP"], 1, GL_FALSE, &proj_14[0][0]);
    glUniformMatrix4fv(basic_shader["uV"], 1, GL_FALSE, &view_14[0][0]);
    glUseProgram(0);

    glUseProgram(flat_shader.Program);
    glUniformMatrix4fv(flat_shader["uP"], 1, GL_FALSE, &proj_14[0][0]);
    glUniformMatrix4fv(flat_shader["uV"], 1, GL_FALSE, &view_14[0][0]);
    glUniform4f(flat_shader["uColor"], 1.0, 1.0, 1.0, 1.f);
    glUseProgram(0);
    glEnable(GL_DEPTH_TEST);

    system("CLS");
    print_info();


    matrix_stack stack;

    trackball_matrix = scaling_matrix = glm::mat4(1.f);
    scaling_factor = 1.f;

    /* define the viewport  */
    glViewport(0, 0, 1000, 800);

    /* avoid rendering back faces */
    // uncomment to see the plane disappear when rotating it
    // glEnable(GL_CULL_FACE);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window)) {
        /* Render here */
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        CheckGLErrors(__LINE__, __FILE__);

        stack.pushLastElement();
        stack.multiply(scaling_matrix * trackball_matrix);

        /* show the plane in flat-wire (filled triangles plus triangle contours) */
        // step 1: render the plane flat
        glUseProgram(flat_shader.Program);
        r_plane.SetAsCurrentObjectToRender();
        stack.pushLastElement();
        glUniformMatrix4fv(flat_shader["uT"], 1, GL_FALSE, &stack.peak()[0][0]);
        glUniform4f(basic_shader["uColor"], 1.0, 1.0, 1.0, 1.0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_plane.elements[1].ind);
        glDrawElements(GL_LINES, r_plane.elements[1].vertexCount, GL_UNSIGNED_INT, 0);


        glUseProgram(basic_shader.Program);

        // enable polygon offset functionality
        glEnable(GL_POLYGON_OFFSET_FILL);

        // set offset function
        glPolygonOffset(1.0, 1.0);

        glUniformMatrix4fv(basic_shader["uT"], 1, GL_FALSE, &stack.peak()[0][0]);

        glUniform3f(basic_shader["uColor"], 0.8f, 0.8f, 0.8f);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_plane.ind);
        glDrawElements(GL_TRIANGLES, r_plane.NumberOfIndices, GL_UNSIGNED_INT, 0);

        // disable polygon offset
        glDisable(GL_POLYGON_OFFSET_FILL);
        stack.pop();
        //  end flat wire

        glUseProgram(basic_shader.Program);
        glUniformMatrix4fv(basic_shader["uT"], 1, GL_FALSE, &stack.peak()[0][0]);
        glUniform3f(basic_shader["uColor"], -1.0, 0.0, 1.0);
        r_frame.SetAsCurrentObjectToRender();
        glDrawArrays(GL_LINES, 0, 6);
        glUseProgram(0);


        r_cube.SetAsCurrentObjectToRender();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_cube.ind);


        glUseProgram(flat_shader.Program);

        for (unsigned int id = 0; id < cubes_pos.size(); ++id) {
            float r = ((id + 200) & 0x000000FF) / 255.f;
            float g = (((id + 200) & 0x0000FF00) >> 8) / 255.f;
            float b = (((id + 200) & 0x00FF0000) >> 16) / 255.f;
            stack.pushLastElement();
            stack.multiply(glm::translate(glm::mat4(1.f), cubes_pos[id]));
            stack.multiply(glm::scale(glm::mat4(1.f), glm::vec3(0.5f, 0.5f, 0.5f)));
            glUniformMatrix4fv(flat_shader["uT"], 1, GL_FALSE, &stack.peak()[0][0]);
            glUniform4f(flat_shader["uColor"], r, g, b, 1.0);
            glDrawElements(GL_TRIANGLES, r_cube.NumberOfIndices, GL_UNSIGNED_INT, 0);
            stack.pop();
        }
        glUseProgram(0);
        stack.pop();

        CheckGLErrors(__LINE__, __FILE__);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}