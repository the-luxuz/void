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

enum class ColliderType {
    None,
    Plane,
    Sphere,
    AABB,
    OBB
};

struct Contact {

    bool hit;

    glm::vec3 normal;

    float penetration;

    glm::vec3 point;
};

struct Collider {

    ColliderType type = ColliderType::AABB;

	uint32_t layer = 1 << 0;        // layer of the object, used for collision filtering
	uint32_t mask = 0xFFFFFFFF;    // with which layers it can collide

    bool isTrigger = false;

    // Sphere
    float radius;

    // Capsule
    float height = 2.0f;

    // AABB/OBB
    glm::vec3 halfExtents = glm::vec3(0.5f);
};

struct Vertex {

    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;

    glm::vec2 originalUV = uv;

    glm::vec3 tangent;
    glm::vec3 bitangent;

};

struct Physics {

    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec3 acceleration = glm::vec3(0.0f);

    glm::vec3 force = glm::vec3(0.0f);

    float inverseMass = 1.0f;

    float restitution = 0.5f;
    float friction = 0.5f;
    float drag = 0.1f;

    bool useGravity = true;
    bool isStatic = false;
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

	Physics physics;
	bool hasPhysics;

    Collider collider;
    bool hasCollider = false;
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

    bool physics_active = true;

	bool collision_active = true;
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

    Physics physics;
    bool hasPhysics = false;

    Collider collider;
	bool hasCollider = false;
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

    return { o.position, o.rotation, o.scale, o.name, o.mesh, o.obj_texture, o.tex, o.min, o.max, o.mapping, o.coordType, o.cyaxisType, o.normalTexture, o.norm_map, o.has_normal_map, o.shininess, o.physics, o.hasPhysics, o.collider, o.hasCollider };
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

	o.physics = s.physics;
	o.hasPhysics = s.hasPhysics;

    o.collider = s.collider;
    o.hasCollider = s.hasCollider;
}

struct Shader_Uniform {

    GLint grid_MVP;

	GLint program_texture;

    GLint depthdata[MAX_LIGHTS];

    GLint light_type[MAX_LIGHTS];
    GLint light_pos[MAX_LIGHTS];
    GLint light_dir[MAX_LIGHTS];
    GLint light_col[MAX_LIGHTS];
    GLint light_att[MAX_LIGHTS];
    GLint light_ambient[MAX_LIGHTS];
    GLint light_bias[MAX_LIGHTS];
    GLint light_pcf[MAX_LIGHTS];
    GLint light_inner[MAX_LIGHTS];
    GLint light_outer[MAX_LIGHTS];
    GLint light_falloff[MAX_LIGHTS];

	GLint lightVP;

	GLint normalMap;

	GLint numLights;

	GLint tex_appear;
	GLint normal_apper;
	GLint tangent_apper;
	GLint bitangent_apper;

	GLint light_active;
	GLint normal_map_active;
	GLint shadow_active;

	GLint cameraPos;

	GLint shininess;

	GLint has_normal_map;

    GLint program_line_color;

    GLint skybox_view;

	GLint skybox_projection;

	GLint is_hdr;

	GLint program_skybox_skybox;

	bool class_uniforms_initialized = false;

};