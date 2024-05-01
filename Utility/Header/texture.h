#pragma once
#include <cassert>

#include <stb_image.h>
#include <GL/glew.h>

struct texture {
    texture() {}

    ~texture() {}

    int x_size, y_size;
    int n_components;
    GLuint id;

    static GLenum SelectChannelValue(int n_components)
    {
        switch (n_components)
        {
            case 1:
                return GL_RED;
            case 3:
                return GL_RGB;
            case 4:
                return GL_RGBA;
            default:
                assert(0);
        }
    }


    void bind(GLuint textureUnit)
    {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, id);
    }

    GLuint load(std::string name, GLuint textureUnit)
    {
        unsigned char *data;
        stbi_set_flip_vertically_on_load(true);
        data = stbi_load(name.c_str(), &x_size, &y_size, &n_components, 0);
        //stbi__vertical_flip(data, x_size, y_size, n_components);
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x_size, y_size,
                     0, SelectChannelValue(n_components), GL_UNSIGNED_BYTE,
                     data);
        stbi_image_free(data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        return id;
    }

    GLuint create(int x_size, int y_size, GLuint channels)
    {
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);


        glTexImage2D(GL_TEXTURE_2D, 0, SelectChannelValue(channels), x_size, y_size, 0, channels, GL_UNSIGNED_BYTE,
                     NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        return id;
    }

    GLuint load_cubemap(std::string path, std::string posx, std::string negx,
                        std::string posy, std::string negy,
                        std::string posz, std::string negz,
                        GLuint textureUnit)
    {
        return load_cubemap(path + posx, path + negx,
                            path + posy, path + negy,
                            path + posz, path + negz, textureUnit);
    }


    GLuint load_cubemap(std::string posx, std::string negx,
                        std::string posy, std::string negy,
                        std::string posz, std::string negz,
                        GLuint textureUnit)
    {
        unsigned char *faces[6];
        faces[0] = stbi_load(posx.c_str(), &x_size, &y_size, &n_components, 0);
        faces[1] = stbi_load(negx.c_str(), &x_size, &y_size, &n_components, 0);
        faces[2] = stbi_load(posy.c_str(), &x_size, &y_size, &n_components, 0);
        faces[3] = stbi_load(negy.c_str(), &x_size, &y_size, &n_components, 0);
        faces[4] = stbi_load(posz.c_str(), &x_size, &y_size, &n_components, 0);
        faces[5] = stbi_load(negz.c_str(), &x_size, &y_size, &n_components, 0);

        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_CUBE_MAP, id);
        for (unsigned int i = 0; i < 6; ++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, x_size, y_size, 0,
                         SelectChannelValue(n_components), GL_UNSIGNED_BYTE,
                         faces[i]);

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        for (auto &face: faces)
            stbi_image_free(face);
        return id;
    }


    GLuint create_cubemap(int x_size, int y_size, int n_components)
    {
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_CUBE_MAP, id);
        for (unsigned int i = 0; i < 6; ++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, x_size, y_size, 0,
                         SelectChannelValue(n_components), GL_UNSIGNED_BYTE,
                         0);

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        return id;
    }
};
