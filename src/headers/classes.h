#pragma once

#include "local.h"
#include <unordered_map>

enum class TextureMapping {
    UV,
    Planar,
    Cubic,
    Cylindrical,
    Spherical
};

enum class coord_types {
    XZ,
    XY,
    YZ
};

enum class cylinder_axis {
    X,
    Y,
    Z
};

struct camera {

    camera() : fov(90), nearPlane(.1f), farPlane(5000.0f), cam_position(100, 100, 100), cam_target(0, 0, 0), cam_up(0, 1, 0) {}

    float fov;

    float nearPlane;
    float farPlane;

    float alpha, beta, radius;

    glm::vec3 cam_position;
    glm::vec3 cam_target;
    glm::vec3 cam_up;

};

struct Vertex {

    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;

    glm::vec2 originalUV = uv;

};

struct ObjectState {

	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;

    std::string name;
    std::string mesh;

    GLuint obj_texture = 0;

};

struct Object {

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;

    std::string name;

    std::string tex; // has the texture path

    GLuint obj_texture = 0;

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    std::vector<Vertex> original_vertices;
    std::vector<GLuint> original_indices;

    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    std::string mesh; // has the .obj path

    glm::vec3 min, max;

    TextureMapping mapping = TextureMapping::UV;
    coord_types coordType = coord_types::XZ;
    cylinder_axis cyaxisType = cylinder_axis::Y;

    std::vector<ObjectState> undoStack;
    std::vector<ObjectState> redoStack;
};

inline ObjectState CaptureState(const Object& o) {

    return { o.position, o.rotation, o.scale, o.name, o.mesh, o.obj_texture };
}

inline void ApplyState(Object& o, const ObjectState& s) {

    o.position = s.position;
    o.rotation = s.rotation;
    o.scale = s.scale;

	o.name = s.name;
	o.mesh = s.mesh;

	o.obj_texture = s.obj_texture;
}

class TextureManager {

    static std::unordered_map<std::string, GLuint> textures;

public:
    static GLuint Get(const std::string& path);

};

struct Program {

    GLuint id;

    GLint mvpLoc;
    GLint textureLoc;
};

enum  class window_mode {
    WINDOWED,
    WINDOWED_BORDERLESS,
    FULLSCREEN
};

struct Settings {

    window_mode win_mode;

    window_mode last_win_mode;

    std::string win_name;

    std::string skybox;

    int w = 1280;
    int h = 720;

    float fov;

    bool VSync;
};