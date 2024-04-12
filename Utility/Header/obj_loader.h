#pragma once


#include <tiny_obj_loader.h>
#include "Renderable.h"
#include <direct.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>


static void create_simple_shapes(std::vector<tinyobj::shape_t> &shapes, int vn, std::vector<std::vector<bool> > &keep) {

    keep.resize(shapes.size());
    for (auto &i: keep)
        i.resize(vn, false);

    for (unsigned int is = 0; is < shapes.size(); ++is) {
        std::vector<int> remap;
        remap.resize(vn, 0);

        for (size_t f = 0; f < shapes[is].mesh.indices.size(); f++) {
            int curr_vi = shapes[is].mesh.indices[f].vertex_index;
            keep[is][curr_vi] = true;
        }

        int delta = 0;
        for (size_t i = 0; i < remap.size(); ++i) {
            if (!keep[is][i])
                delta++;
            else
                remap[i] = delta;
        }

        for (auto &indice: shapes[is].mesh.indices) {
            int &curr_vi = indice.vertex_index;
            curr_vi -= remap[curr_vi];
        }

    }

}

struct v_nor_tex : public std::pair<int, int> {
    v_nor_tex() { first = second = -1; }

    v_nor_tex(int f, int s) {
        first = f;
        second = s;
    }
};

static void load_obj(std::vector<Renderable> &rs, std::string workingPath, const std::string &inputfile) {

    _chdir(workingPath.c_str());
    tinyobj::ObjReaderConfig reader_config = tinyobj::ObjReaderConfig();
    reader_config.mtl_search_path = "./"; // Path to material files

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(inputfile, reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader: " << reader.Error();
        }
        exit(1);
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto attrib = reader.GetAttrib();
    auto shapes = reader.GetShapes();
    auto materials = reader.GetMaterials();

    // create the vertex attributes
    std::vector<tinyobj::real_t> pos = attrib.GetVertices();
    std::vector<tinyobj::real_t> norms = attrib.normals;
    std::vector<tinyobj::real_t> t_coords = attrib.texcoords;
    std::vector<tinyobj::real_t> v_norm, v_pos, v_tcoord;


    if (!norms.empty() || !t_coords.empty()) {
        unsigned int nv = (unsigned int) pos.size() / 3;
        std::vector<int> skip_to, pos_id;
        std::vector<v_nor_tex> tn_id;

        tn_id.resize(nv, v_nor_tex());
        pos_id.resize(nv, 0);
        for (unsigned int ii = 0; ii < pos_id.size(); ii++) pos_id[ii] = ii;

        skip_to.resize(nv, -1);

        for (auto &shape: shapes) {
            std::vector<unsigned int> inds;
            for (auto &id: shape.mesh.indices) {
                int vi = id.vertex_index;
                while (skip_to[vi] != -1 && (tn_id[vi] != v_nor_tex()) &&
                       (tn_id[vi] != v_nor_tex(id.normal_index, id.texcoord_index))) {
                    vi = skip_to[vi];
                }
                if ((tn_id[vi] ==
                     v_nor_tex(id.normal_index, id.texcoord_index))) { // a vertex with the right normal is found
                    id.vertex_index = vi;
                    tn_id[vi] = v_nor_tex(id.normal_index, id.texcoord_index);
                } else {
                    if (tn_id[vi] == v_nor_tex()) { // a vertex with unassigned normal is found
                        id.vertex_index = vi;
                        tn_id[vi] = v_nor_tex(id.normal_index, id.texcoord_index);
                    } else {// a new vertex must be added
                        assert(skip_to[vi] == -1);
                        skip_to[vi] = (int) pos_id.size();
                        skip_to.push_back(-1);
                        pos_id.push_back(pos_id[id.vertex_index]); // make a new vertex with the same position..
                        id.vertex_index = (int) pos_id.size() - 1;
                        tn_id.push_back(
                                v_nor_tex(id.normal_index, id.texcoord_index));         // ..and the new normal/tcoord
                    }
                }
            }
        }

        // check
        for (auto &shape: shapes) {
            std::vector<unsigned int> inds;
            for (auto &indice: shape.mesh.indices) {
                assert(indice.vertex_index >= 0);
                assert(indice.vertex_index < pos_id.size());
                if (!norms.empty()) {
                    assert(indice.normal_index >= 0);
                    assert(tn_id[indice.normal_index].first >= 0);
                }
                if (!t_coords.empty())
                    assert(tn_id[indice.texcoord_index].second >= 0);
            }
        }

        for (int i: pos_id) {
            v_pos.push_back(pos[i * 3]);
            v_pos.push_back(pos[i * 3 + 1]);
            v_pos.push_back(pos[i * 3 + 2]);
        }
        if (!norms.empty())
            for (unsigned int i = 0; i < pos_id.size(); ++i) {
                v_norm.push_back(norms[tn_id[i].first * 3]);
                v_norm.push_back(norms[tn_id[i].first * 3 + 1]);
                v_norm.push_back(norms[tn_id[i].first * 3 + 2]);
            }

        if (!t_coords.empty())
            for (unsigned int i = 0; i < pos_id.size(); ++i) {
                v_tcoord.push_back(t_coords[tn_id[i].second * 2]);
                v_tcoord.push_back(t_coords[tn_id[i].second * 2 + 1]);
            }
    } else
        v_pos = pos; // silly: fix the wasteful copy

    // compute the bounding box of the vertices
    box3 bbox;
    for (unsigned int i = 0; i < v_pos.size() / 3; ++i)
        bbox.add(glm::vec3(v_pos[3 * i], v_pos[3 * i + 1], v_pos[3 * i + 2]));

    // split the shapes so that each shape has the same index material
    std::vector<tinyobj::shape_t> mshapes;
    std::vector<int> mat_ids;
    for (auto &shape: shapes) {
        unsigned int fi = 0;
        do {
            int mat_ind = shape.mesh.material_ids[fi];
            mshapes.push_back(tinyobj::shape_t());
            mat_ids.push_back(mat_ind);
            while (fi < shape.mesh.material_ids.size() &&
                   shape.mesh.material_ids[fi] == mat_ind) {
                mshapes.back().mesh.indices.push_back(shape.mesh.indices[fi * 3]);
                mshapes.back().mesh.indices.push_back(shape.mesh.indices[fi * 3 + 1]);
                mshapes.back().mesh.indices.push_back(shape.mesh.indices[fi * 3 + 2]);
                ++fi;
            }

        } while (fi < shape.mesh.material_ids.size());
    }

//	std::vector< std::vector< bool> > keep;
//	create_simple_shapes(mshapes,v_pos.size(),keep);
//  se si fa lo split bisogna creare gli array buffer per ogni shape
//  Ogni array buffer � dato da quello originale da cui si cancellano i vertici !keep[][] 
//  quando una shape � stata create possiamo calcolare il texture space

    // resize the vector of Renderable
    rs.resize(mshapes.size());

    // create the vertex array buffers and share them between all renderables
    rs[0].create();
    rs[0].AddVertexAttribute(&v_pos[0], (unsigned int) v_pos.size(), 0, 3);
    rs[0].bbox = bbox;

    if (!v_norm.empty())
        rs[0].AddVertexAttribute(&v_norm[0], (unsigned int) v_norm.size(), 2, 3);

    if (!v_tcoord.empty())
        rs[0].AddVertexAttribute(&v_tcoord[0], (unsigned int) v_tcoord.size(), 3, 2);

    for (unsigned int i = 1; i < mshapes.size(); ++i) {
        rs[i].create();
        rs[i].assign_vertex_attribute(rs[0].VertexAttributeBuffers[0], (unsigned int) v_pos.size() / 3, 0, 3, GL_FLOAT);

        if (!v_norm.empty())
            rs[i].assign_vertex_attribute(rs[0].VertexAttributeBuffers[1], (unsigned int) v_norm.size() / 3, 2, 3,
                                          GL_FLOAT);

        if (!v_tcoord.empty())
            rs[i].assign_vertex_attribute(rs[0].VertexAttributeBuffers[2], (unsigned int) v_tcoord.size() / 2, 3, 2,
                                          GL_FLOAT);
    }

    for (unsigned int is = 0; is < mshapes.size(); ++is) {
        std::vector<unsigned int> inds;
        for (auto &indice: mshapes[is].mesh.indices)
            inds.push_back(static_cast<unsigned int>(indice.vertex_index));
        rs[is].add_element_array(&inds[0], (int) mshapes[is].mesh.indices.size(), GL_TRIANGLES);
    }

    if (!materials.empty())
        for (unsigned int is = 0; is < mshapes.size(); ++is) {
            tinyobj::material_t &m = materials[mat_ids[is]];
            rs[is].material.name = m.name;
            memcpy_s(rs[is].material.ambient, sizeof(float) * 3, m.ambient, sizeof(float) * 3);
            memcpy_s(rs[is].material.diffuse, sizeof(float) * 3, m.diffuse, sizeof(float) * 3);
            memcpy_s(rs[is].material.specular, sizeof(float) * 3, m.specular, sizeof(float) * 3);
            memcpy_s(rs[is].material.transmittance, sizeof(float) * 3, m.transmittance, sizeof(float) * 3);
            memcpy_s(rs[is].material.emission, sizeof(float) * 3, m.emission, sizeof(float) * 3);
            rs[is].material.shininess = m.shininess;
            rs[is].material.ior = m.ior;
            rs[is].material.dissolve = m.dissolve;

            if (!m.diffuse_texname.empty())
                rs[is].material.diffuse_texture.load(m.diffuse_texname, 0);
            if (!m.ambient_texname.empty())
                rs[is].material.ambient_texture.load(m.ambient_texname, 1);
            if (!m.specular_texname.empty())
                rs[is].material.specular_texture.load(m.specular_texname, 2);
        }
}
