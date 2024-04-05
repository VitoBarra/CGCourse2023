#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "debugging.h"
#include "Renderable.h"
#include "shaders.h"
#include <GL/glew.h>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>

void DrawTriangleElements(int n) {
    glDrawElements(GL_TRIANGLES, n, GL_UNSIGNED_INT, nullptr);

}

void DrawTriangleArray(int n) {
    glDrawArrays(GL_TRIANGLES, 0, n);
}