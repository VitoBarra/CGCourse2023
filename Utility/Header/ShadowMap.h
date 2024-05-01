#pragma once

#include <glm/glm.hpp>
#include <frame_buffer_object.h>


class ShadowMap {
public:
    glm::mat4 view_matrix, proj_matrix;
    texture tex;
    frame_buffer_object DepthBuffer;
    // size of the shadow map in texels
    int SizeW, SizeH;


    void create(int w ,int h)
    {
        SizeW = w;
        SizeH = h;
        DepthBuffer.create(w, h, true);
    }

    glm::mat4 set_projection(glm::mat4 _view_matrix,float distance_light, box3 box)
    {
        view_matrix = _view_matrix;

        /* TBD: set the view volume properly so that they are a close fit of the
        bounding box passed as paramter */
        proj_matrix = glm::ortho(-4.f, 4.f, -4.f, 4.f, 0.f, distance_light * 2.f);
        //		proj_matrix = glm::perspective(3.14f/2.f,1.0f,0.1f, distance_light*2.f);
        return proj_matrix;
    }

    glm::mat4 light_matrix()
    {
        return proj_matrix * view_matrix;
    }

    void Bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, DepthBuffer.id_fbo);
        glViewport(0, 0, SizeW, SizeH);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        CheckGLErrors(__LINE__, __FILE__);
    }
    void undind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        CheckGLErrors(__LINE__, __FILE__);
    }
};
