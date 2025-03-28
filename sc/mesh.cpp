#ifndef MESH_CPP
#define MESH_CPP

#include <string>
#include <vector>

#ifndef HEADER
    #define HEADER
    #include "../vk/vklib.h"
    #undef HEADER
#endif

namespace sc {

// represents a vertex.
struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 uv;
};

// represents a mesh object.
class Mesh {

    vk::Buffer* verts;
    vk::Buffer* idx;
    uint32_t nverts;
    uint32_t nidx;

public:
    Mesh(vk::Device&, std::string);
    ~Mesh();

    void bind(vk::CommandBuffer&);
    void draw(vk::CommandBuffer&);

};

}; // end of instance.h file
#ifndef HEADER

#include <iostream>
#include <fstream>
#include <map>
#include <cstring>

namespace sc {

// loads a mesh from a .obj file, into a pair of `vk::Buffer`s.
Mesh::Mesh (vk::Device& device, std::string filename) {

    // initialize cpu-side buffers
    std::vector<Vertex> _verts;
    std::vector<uint32_t> _idx;
    
    // open file
    std::ifstream objfile("assets/" + filename);

    // temporary buffers, for index-lookup
    std::vector<glm::vec3> tmp_pos;
    std::vector<glm::vec3> tmp_norm;
    std::vector<glm::vec2> tmp_uv;

    std::map<std::tuple<uint32_t, uint32_t, uint32_t>, uint32_t> tmp_idx = {};

    std::string lh;

    // parse through the file, loading the positions of the v, vn, vt and f records.
    while (objfile >> lh) {

        if (lh[0] == '#') {
            objfile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        else if (lh == "vn") {
            float x, y, z;
            objfile >> x;
            objfile >> y;
            objfile >> z;
            tmp_norm.push_back(glm::vec3(x, y, z));
        }
        else if (lh == "vt") {
            float x, y;
            objfile >> x;
            objfile >> y;
            tmp_uv.push_back(glm::vec2(x, y));
        }
        else if (lh == "v") {
            float x, y, z;
            objfile >> x;
            objfile >> y;
            objfile >> z;
            tmp_pos.push_back(glm::vec3(x, y, z));
        }
        else if (lh == "f") {
            for (int _ = 0; _ < 3; _++) {
                // assume triangle faces, 3 verticies per face.
                std::tuple<int, int, int> fv;
                std::string v_str;
                std::string vt_str;
                std::string vn_str;

                std::getline(objfile,  v_str, '/');
                std::getline(objfile, vt_str, '/');
                objfile >> vn_str;

                // obj files are 1-indexed
                std::get<0>(fv) = std::stoi( v_str) -1;
                std::get<1>(fv) = std::stoi(vt_str) -1;
                std::get<2>(fv) = std::stoi(vn_str) -1;

                // check if we already made this vertex
                if (true || !tmp_idx.contains(fv)) {
                    // we need to create a new vertex, and push that
                    Vertex p;
                    p.pos = tmp_pos[std::get<0>(fv)];
                    p.uv = tmp_uv[std::get<1>(fv)];
                    p.norm = tmp_norm[std::get<2>(fv)];

                    _verts.push_back(p);

                    tmp_idx[fv] = _verts.size() - 1;
                }
                _idx.push_back( tmp_idx[fv] );
            }
        }
    }

    nverts = _verts.size();
    nidx = _idx.size();

    // printf("loaded model %s with %d verts and %d indecies (%d faces)\n", filename.c_str(), nverts, nidx, nidx/3);

    // create the buffers -- for now, they're host visible
    verts = new vk::Buffer(device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _verts.size() * sizeof(Vertex));
    
    idx = new vk::Buffer(device, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _idx.size() * sizeof(uint32_t));

    // copy into the buffers
    verts->staged([&](void* data){
        std::memcpy(data, _verts.data(), _verts.size() * sizeof(Vertex));
    });

    idx->staged([&](void* data){
        std::memcpy(data, _idx.data(), _idx.size() * sizeof(uint32_t));
    });
}

// binds this mesh's buffers to the commandbuffer
void Mesh::bind(vk::CommandBuffer& cmd) {
    cmd.bindVertexInput({verts});
    cmd.bindVertexIndices(*idx, VK_INDEX_TYPE_UINT32);
}

// draws this mesh
void Mesh::draw(vk::CommandBuffer& cmd) {
    bind(cmd);
    cmd.drawIndexed(nidx, 1);
}

// destructor
Mesh::~Mesh(){
    delete verts;
    delete idx;
}

};
#endif
#endif