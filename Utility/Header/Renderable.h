#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "texture.h"
#include <iostream>

struct box3 {
    /// min coordinate point
    glm::vec3 min;

    /// max coordinate point
    glm::vec3 max;

    /// The bounding box constructor (make it of size s centered in 0^3
    inline box3(float s) {
        min = glm::vec3(-s / 2.f);
        max = glm::vec3(s / 2.f);
    }

    /// The bounding box constructor (make it empty)
    inline box3() {
        min = glm::vec3(1.f);
        max = glm::vec3(-1.f);
    }

    /// Min Max constructor
    inline box3(const glm::vec3 &mi, const glm::vec3 &ma) {
        min = mi;
        max = ma;
    }

    /// The bounding box distructor
    inline ~box3() {}

    /** Modify the current bbox to contain also the passed point
    */
    void add(const glm::vec3 &p) {
        if (min.x > max.x) { min = max = p; }
        else {
            if (min.x > p.x) min.x = p.x;
            if (min.y > p.y) min.y = p.y;
            if (min.z > p.z) min.z = p.z;

            if (max.x < p.x) max.x = p.x;
            if (max.y < p.y) max.y = p.y;
            if (max.z < p.z) max.z = p.z;
        }
    }

    bool is_empty() const { return min == max; }

    float diagonal() const {
        return glm::length(min - max);
    }

    glm::vec3 center() const {
        return (min + max) * 0.5f;
    }

};


struct material {
    std::string name;

    float ambient[3];
    float diffuse[3];
    float specular[3];
    float transmittance[3];
    float emission[3];
    float shininess;
    float ior;       // index of refraction
    float dissolve;  // 1 == opaque; 0 == fully transparent

    texture ambient_texture, diffuse_texture, specular_texture;
};

class Renderable {

public:
    struct element_array {
        GLuint ind;
        GLuint element_type;
        GLsizei vertexCount;

        element_array() = default;

        void Render() const {
            glDrawElements(element_type, vertexCount, GL_UNSIGNED_INT, nullptr);
        }
    };

    // vertex array object
    GLuint VertexArrayID = -1;

    // vertex buffer objects
    std::vector<GLuint> VertexAttributeBuffers;

    // element array
    GLuint ind;

    // vector of element array
    std::vector<element_array> elements;

    // element arrays (backward compatible implementation)

    // primitive type
    unsigned int elem_type;

    // number of vertices
    unsigned int NumberOfVertices;

    // number of indices
    GLsizei NumberOfIndices;

    // bounding box of the shape
    box3 bbox;

    material material;

    Renderable() {
        glGenVertexArrays(1, &VertexArrayID);
    }

    void create() {}

    void SetAsCurrentObjectToRender() {
        glBindVertexArray(VertexArrayID);
    }

    GLuint assign_vertex_attribute(unsigned int va_id, unsigned int count,
                                   unsigned int attribute_index,
                                   unsigned int num_components,
                                   unsigned int TYPE,
                                   unsigned int stride = 0,
                                   unsigned int offset = 0) {


        glBindVertexArray(VertexArrayID);

        /* create a buffer for the render data in video RAM */
        VertexAttributeBuffers.push_back(va_id);

        glBindBuffer(GL_ARRAY_BUFFER, VertexAttributeBuffers.back());
        glEnableVertexAttribArray(attribute_index);

        /* specify the data format */
        glVertexAttribPointer(attribute_index, num_components, TYPE, false, stride, (void *) (size_t) offset);

        glBindVertexArray(0);
        return VertexAttributeBuffers.back();
    }

    template<class T>
    void AddBuffer(std::vector<T> values) {

        AddBuffer(&values[0], values.size());
    }

    template<class T>
    void AddBuffer(T *values, int count) {

        glBindVertexArray(VertexArrayID);

        /* create a buffer for the render data in video RAM */
        VertexAttributeBuffers.push_back(0);
        glCreateBuffers(1, &VertexAttributeBuffers.back());

        glBindBuffer(GL_ARRAY_BUFFER, VertexAttributeBuffers.back());

        /* declare what data in RAM are filling the buffering video RAM */
        glBufferData(GL_ARRAY_BUFFER, sizeof(T) * count, values, GL_STATIC_DRAW);
    }


    template<class T>
    GLuint AddVertexAttribute(std::vector<T> values,
                              const std::vector<GLuint> attribute_indices,
                              const std::vector<unsigned int> numElementForComponents,
                              unsigned int stride,
                              const std::vector<unsigned int> offsets
    ) {

        if (attribute_indices.size() != numElementForComponents.size() ||
            attribute_indices.size() != offsets.size()) {
            std::cerr << "Error: all input vectors must have the same size.\n";
            return 0;
        }

        AddBuffer(values);

        for (size_t i = 0; i < attribute_indices.size(); ++i) {
            glEnableVertexAttribArray(attribute_indices[i]);

            /* specify the data format */
            glVertexAttribPointer(attribute_indices[i], numElementForComponents[i], typeSwitch(values[0]), false,
                                  stride,
                                  (void *) (size_t) offsets[i]);
        }

        glBindVertexArray(0);
        return VertexAttributeBuffers.back();
    }

    template<class T>
    GLuint AddVertexAttribute(T *values, unsigned int count,
                              unsigned int attribute_index,
                              unsigned int numElementForComponent,
                              unsigned int stride = 0,
                              unsigned int offset = 0) {


        AddBuffer(values, count);
        glEnableVertexAttribArray(attribute_index);

        /* specify the data format */
        glVertexAttribPointer(attribute_index, numElementForComponent, typeSwitch(values[0]), false, stride,
                              (void *) (size_t) offset);

        glBindVertexArray(0);
        return VertexAttributeBuffers.back();
    }


    template<typename T>
    unsigned int typeSwitch(T t) {
        if (typeid(t) == typeid(int)) {
            return GL_INT;
        } else if (typeid(t) == typeid(unsigned int)) {
            return GL_UNSIGNED_INT;
        } else if (typeid(t) == typeid(short)) {
            return GL_SHORT;
        } else if (typeid(t) == typeid(unsigned short)) {
            return GL_UNSIGNED_SHORT;
        } else if (typeid(t) == typeid(float)) {
            return GL_FLOAT;
        } else if (typeid(t) == typeid(double)) {
            return GL_DOUBLE;
        } else {
            // Handle unsupported type
            std::cerr << "Unsupported type provided to typeSwitch function.\n";
            return -1;
        }
    }


    GLuint add_indices(unsigned int *indices, unsigned int count, unsigned int ELEM_TYPE) {
        std::cout << "deprecated (but working) : use add_element_array" << std::endl;
        add_element_array(indices, count, ELEM_TYPE);
        NumberOfIndices = count;
        elem_type = ELEM_TYPE;
        ind = elements.back().ind;
        return ind;
    };

    GLuint add_element_array(void *indices, unsigned int count, unsigned int ELEM_TYPE) {
        elements.push_back(element_array());
        glBindVertexArray(VertexArrayID);
        glGenBuffers(1, &elements.back().ind);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elements.back().ind);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * count, indices, GL_STATIC_DRAW);
        glBindVertexArray(0);
        elements.back().element_type = ELEM_TYPE;
        elements.back().vertexCount = count;
        return elements.back().ind;
    };

    GLuint add_indices(std::vector<GLuint> indices, unsigned int ELEM_TYPE) {
        return add_indices(&indices[0], indices.size(), ELEM_TYPE);;
    };

    GLuint add_element_array(std::vector<GLuint> indices, unsigned int ELEM_TYPE) {
        return add_element_array(&indices[0], indices.size(), ELEM_TYPE);
    };

    void RenderTriangles() {
        glDrawElements(GL_TRIANGLES, NumberOfIndices, GL_UNSIGNED_INT, nullptr);
    }



};