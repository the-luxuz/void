#pragma once

#include "local.h"
#include <glm/gtx/quaternion.hpp>
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

class TextureManager {

    static std::unordered_map<std::string, GLuint> textures;

public:
    static GLuint Get(const std::string& path);
    static void Clear();

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

struct Vertex {

    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;

    glm::vec2 originalUV = uv;

    glm::vec3 tangent;
    glm::vec3 bitangent;

};

struct ObjectState {

    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    std::string name;
    std::string mesh;

    GLuint obj_texture;
    std::string tex; // has the texture path

    glm::vec3 min, max;

    TextureMapping mapping;
    coord_types coordType;
    cylinder_axis cyaxisType;

    GLuint normalTexture;
    std::string norm_map;

    bool has_normal_map;

    float shininess;

};

struct LightState {

    glm::vec3 col;
    glm::vec3 att;

    float ambient;

    float inner;
    float outer;
    float falloff;

    float bias;
    float pcf;

    std::string type;

    GLuint typeID;

};

struct LineVertex {

    glm::vec3 position;
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

    bool light_active = true;
    bool normal_map_active = true;
    bool shadow_active = true;
    bool wireframe_active = false;
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

struct Object {

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;

    GLuint normalVAO;
    GLuint normalVBO;

    GLuint normalTBVAO;
    GLuint normalTBVBO;

    GLuint tangentVAO;
    GLuint tangentVBO;

    GLuint bitangentVAO;
    GLuint bitangentVBO;

    std::string name;

    std::string tex; // has the texture path

    GLuint obj_texture = 0;

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    std::vector<LineVertex> normalLines;

    std::vector<LineVertex> tangentLines;
    std::vector<LineVertex> bitangentLines;
    std::vector<LineVertex> normalLinesTBN;

    std::vector<Vertex> original_vertices;
    std::vector<GLuint> original_indices;

    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    std::string mesh; // has the .obj path

    glm::vec3 min, max;

    TextureMapping mapping = TextureMapping::UV;
    coord_types coordType = coord_types::XZ;
    cylinder_axis cyaxisType = cylinder_axis::Y;

    std::vector<ObjectState> undoStack;
    std::vector<ObjectState> redoStack;

    glm::mat4 model;
    glm::mat4 mvp;

    std::string type;

    GLuint normalTexture = 0;
    std::string norm_map;

    bool has_normal_map = false;

    float shininess = 20;
};

struct Light {

    std::string name;

    glm::vec3 pos;
    glm::vec3 dir;
    glm::vec3 col;
    glm::vec3 att;

    glm::quat rotation;

    float ambient;

    float inner;
    float outer;
    float falloff;

    float bias;
    float pcf;

    std::string type;

    GLuint typeID;

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    std::vector<LightState> undoStack;
    std::vector<LightState> redoStack;
};

inline LightState CaptureLightState(const Light& l) {

    return { l.col, l.att, l.ambient, l.inner, l.outer, l.falloff, l.bias, l.pcf, l.type, l.typeID };
}

inline void ApplyLightState(Light& l, const LightState& s) {

    l.col = s.col;
    l.att = s.att;

    l.ambient = s.ambient;

    l.inner = s.inner;
    l.outer = s.outer;

    l.falloff = s.falloff;

    l.bias = s.bias;
    l.pcf = s.pcf;

    l.type = s.type;
    l.typeID = s.typeID;
}

inline ObjectState CaptureState(const Object& o) {

    return { o.position, o.rotation, o.scale, o.name, o.mesh, o.obj_texture, o.tex, o.min, o.max, o.mapping, o.coordType, o.cyaxisType, o.normalTexture, o.norm_map, o.has_normal_map, o.shininess };
}

inline void ApplyState(Object& o, const ObjectState& s) {

    o.position = s.position;
    o.rotation = s.rotation;
    o.scale = s.scale;

	o.name = s.name;
	o.mesh = s.mesh;

	o.obj_texture = s.obj_texture;
	o.tex = s.tex;

	o.min = s.min;
	o.max = s.max;

	o.mapping = s.mapping;
	o.coordType = s.coordType;
	o.cyaxisType = s.cyaxisType;

	o.normalTexture = s.normalTexture;
	o.norm_map = s.norm_map;
	o.has_normal_map = s.has_normal_map;

	o.shininess = s.shininess;
}