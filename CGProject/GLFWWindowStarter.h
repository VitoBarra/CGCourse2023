

#include <GLFW/glfw3.h>

class GLFWWindowStarter {
private:
public :
    static GLFWwindow *CreateWindow(int width, int height, const char *name) {
        return CreateWindow(width, height, name, nullptr, nullptr);
    }

    static GLFWwindow *CreateWindow(int width, int height, const char *name, GLFWmonitor *monitor, GLFWwindow *share) {

        GLFWwindow *window;
        /* Initialize the library */
        if (!glfwInit())
            return nullptr;

        /* Create a windowed mode window and its OpenGL context */
        window = glfwCreateWindow(width, height, name, monitor, share);
        if (!window) {
            glfwTerminate();
            return nullptr;
        }

        /* declare the callback functions on mouse events */
        if (glfwRawMouseMotionSupported())
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);


        /* Make the window's context current */
        glfwMakeContextCurrent(window);
        return window;
    }
};