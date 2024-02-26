#include <imgui.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "debugging.h"

void SetUpTriangleVertex();

GLuint CreateShader(GLuint positionAttribIndex, GLuint colorAttribIndex);

void DrawElements(int n);


int MyFirstTriangle(void) {

    GLFWwindow *window;
    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(1000, 800, "MyFirstTriangle", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    glewInit();

    printout_opengl_glsl_info();

    SetUpTriangleVertex();
    GLuint program_shader = CreateShader(0, 1);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window)) {
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program_shader);

        /* issue the DrawElements command*/
        DrawElements(6);

        glUseProgram(0);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);


        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void DrawElements(int n) {
    //glDrawArrays(GL_TRIANGLES, 0, 5);
    glDrawElements(GL_TRIANGLES, n, GL_UNSIGNED_INT, nullptr);

}

GLuint CreateShader(GLuint positionAttribIndex, GLuint colorAttribIndex) {
    /*  \BEGIN IGNORATE DA QUI IN POI */
    /* create a vertex Shader */
    std::string vertex_shader_src = "#version 330\n \
        in vec2 aPosition;\
        in vec3 aColor;\
        out vec3 vColor;\
        void main(void)\
        {\
         gl_Position = vec4(aPosition, 0.0, 1.0);\
         vColor = aColor;\
        }\
       ";
    const GLchar *vs_source = (const GLchar *) vertex_shader_src.c_str();
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vs_source, NULL);
    glCompileShader(vertex_shader);
    check_shader(vertex_shader);

    /* create a fragment Shader */
    std::string fragment_shader_src = "#version 330 \n \
        out vec4 color;\
        in vec3 vColor;\
        void main(void)\
        {\
            color = vec4(vColor, 1.0);\
        }";
    const GLchar *fs_source = (const GLchar *) fragment_shader_src.c_str();
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fs_source, NULL);
    glCompileShader(fragment_shader);
    check_shader(fragment_shader);

    GLuint program_shader = glCreateProgram();
    glAttachShader(program_shader, vertex_shader);
    glAttachShader(program_shader, fragment_shader);
    glBindAttribLocation(program_shader, positionAttribIndex, "aPosition");
    glBindAttribLocation(program_shader, colorAttribIndex, "aColor");
    glLinkProgram(program_shader);
    return program_shader;
}


void SetUpTriangleVertex() {

/* create render data in RAM */
    GLuint positionAttribIndex = 0;
    GLuint ColorAttribIndex = 1;
    float positionsAndColors[] =
            {0.0, 0.0, 1.0, 0.0, 0.0,  // 1st vertex
             0.5, 0.0, 0.0, 1.0, 0.0, // 2nd vertex
             0.5, 0.5, 0.0, 0.0, 1.0, // 3rd vertex
             -0.5, 0.0, 0.0, 1.0, 0.0, // 4rd vertex
             -0.5, -0.5, 0.0, 0.0, 1.0 // 5rd vertex
            };
    /* create a buffer for the render data in video RAM */
    GLuint positionsAndColorBuffer;
    glCreateBuffers(1, &positionsAndColorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, positionsAndColorBuffer);

    /* declare what data in RAM are filling the bufferin video RAM */
    glBufferData(GL_ARRAY_BUFFER, sizeof(positionsAndColors), positionsAndColors, GL_STATIC_DRAW);
    glEnableVertexAttribArray(positionAttribIndex);
    glEnableVertexAttribArray(ColorAttribIndex);

    /* specify the data format */
    glVertexAttribPointer(positionAttribIndex, 2, GL_FLOAT, false, sizeof(float) * 5, 0);
    glVertexAttribPointer(ColorAttribIndex, 3, GL_FLOAT, false, sizeof(float) * 5,
                          reinterpret_cast<const void *>(sizeof(float) * 2));


    GLuint indices[] = {0, 1, 2, 0, 3, 4};
    GLuint indexBuffer;
    glCreateBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

}