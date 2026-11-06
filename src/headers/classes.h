#pragma once

#include "local.h"
#include "video_player.h"

#include <algorithm>
#include <glm/gtx/quaternion.hpp>
#include <unordered_map>
#include <variant>

using EntityID = uint64_t;

inline EntityID nextEntityID = 1;

EntityID GenerateEntityID();

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
    static GLuint Get(const std::string &path, bool sRGB = false);
    static GLuint GetEmbedded(const std::string &key, const unsigned char *data, size_t size, bool sRGB = false);
    static GLuint GetEmbeddedRawBGRA(const std::string &key, int width, int height, const unsigned char *data, bool sRGB = false);
    static void Clear();
};

struct VideoQuad {

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;

    float vertices[16] = {
  // position // uv
  -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f};

    unsigned int indices[6] = {0, 1, 2, 2, 3, 0};
};

struct Program {

    GLuint id;

    GLint mvpLoc;
    GLint textureLoc;
};

enum class window_mode {
    WINDOWED,
    WINDOWED_BORDERLESS,
    FULLSCREEN
};

enum class ColliderType {
    None,
    Plane,
    Sphere,
    AABB
};

enum CollisionLayer {
    World = 1 << 0,
    Player = 1 << 1,
    Enemy = 1 << 2,
    Bullets = 1 << 3
};

struct Contact {

    bool hit;

    glm::vec3 normal;

    float penetration;

    glm::vec3 point;
};

struct Collider {

    ColliderType type = ColliderType::AABB;

    uint32_t layer = CollisionLayer::World; // layer of the object, used for collision filtering
    uint32_t mask = CollisionLayer::World;  // with which layers it can collide

    bool isTrigger = false;

    // Sphere
    float radius = 0.5f;

    // AABB
    // glm::vec3 halfExtents = glm::vec3(0.5f);
};

struct ColliderTransform {

    glm::vec3 offset = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 size = glm::vec3(1.0f);
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

    glm::vec3 gravity = glm::vec3(0.f, -1000.f, 0.f);

    float inverseMass = 1.0f;

    float restitution = 0.5f;
    float drag = 8.0f;

    bool useGravity = true;
    bool isStatic = false;

    // 0 disables the horizontal speed limit.
    float maxHorizontalSpeed = 0.0f;
};

struct ObjectState {

    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
    EntityID parentID = 0;

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

    bool hasNormalMap;

    float shininess;

    Physics physics;
    bool hasPhysics;

    Collider collider;
    ColliderTransform colliderTransform;
    bool hasCollider = false;

    bool hasMaterial = true;

    bool hasMapping = true;

    bool hasAnimation = true;

    bool tiling = false;

    bool visible = true;

    bool renderBothSides = false;
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

    bool hasAnimation = true;
};

struct LineVertex {

    glm::vec3 position;
};

struct Material {

    std::string name;

    // Albedo / diffuse
    std::string albedoPath;
    GLuint albedoTexture = 0;

    // Normal / bump
    std::string normalPath;
    GLuint normalTexture = 0;

    std::string roughnessPath;
    GLuint roughnessTexture = 0;
    std::string metallicPath;
    GLuint metallicTexture = 0;
    std::string emissionPath;
    GLuint emissionTexture = 0;
    std::string opacityPath;
    GLuint opacityTexture = 0;

    // Kd del MTL
    glm::vec3 diffuse = glm::vec3(1.0f);
    glm::vec3 specular = glm::vec3(0.5f);
    glm::vec3 emission = glm::vec3(0.0f);
    float shininess = 32.0f;
    float opacity = 1.0f;
    float roughness = 0.5f;
    float metallic = 0.0f;
};

struct Submesh {
    GLuint firstIndex = 0;
    GLuint indexCount = 0;
    Material material;
    glm::vec3 min = glm::vec3(0.0f);
    glm::vec3 max = glm::vec3(0.0f);
};

struct Settings {

    window_mode win_mode;

    window_mode last_win_mode;

    std::string win_name;

    std::string skybox;

    int w = 1280;
    int h = 720;

    int windowed_w = 0;
    int windowed_h = 0;

    float fov;

    bool VSync;

    bool light_active = false;
    bool normal_map_active = false;
    bool shadow_active = false;
    bool wireframe_active = false;

    bool physics_active = true;

    bool collision_active = true;
};

struct camera {

    camera() : fov(90), nearPlane(.5f), farPlane(5000.0f), cam_position(100, 100, 100), cam_target(0, 0, 0), cam_up(0, 1, 0) { }

    float fov;

    float nearPlane;
    float farPlane;

    float alpha, beta, radius;

    glm::vec3 cam_position;
    glm::vec3 cam_target;
    glm::vec3 cam_up;
};

struct Object {

    EntityID id = 0;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;

    GLuint normalVAO = 0;
    GLuint normalVBO = 0;

    GLuint normalTBVAO = 0;
    GLuint normalTBVBO = 0;

    GLuint tangentVAO = 0;
    GLuint tangentVBO = 0;

    GLuint bitangentVAO = 0;
    GLuint bitangentVBO = 0;

    std::string name;

    std::string tex; // has the texture path

    GLuint obj_texture = 0;

    glm::vec2 textureTiling = glm::vec2(1.0f);

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Submesh> submeshes;

    std::vector<LineVertex> normalLines;

    std::vector<LineVertex> tangentLines;
    std::vector<LineVertex> bitangentLines;
    std::vector<LineVertex> normalLinesTBN;

    std::vector<Vertex> original_vertices;
    std::vector<GLuint> original_indices;

    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    EntityID parentID = 0;
    uint64_t shadowGeometryRevision = 0;

    float importScale = 1.0f;

    std::string mesh; // has the .obj path

    uint32_t importedMeshIndex = 0;

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

    bool hasNormalMap = false;

    float shininess = 20;

    Physics physics;
    bool hasPhysics = false;

    Collider collider;
    ColliderTransform colliderTransform;
    bool hasCollider = false;

    bool hasMaterial = true;

    bool hasMapping = true;

    bool hasAnimation = true;

    bool tiling = false;

    bool visible = true;

    bool renderBothSides = false;
};

inline bool IsEmptyObject(const Object &object) {
    return object.mesh.empty() && object.vertices.empty();
}

struct Light {

    EntityID id = 0;

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

    bool hasAnimation = true;
};

inline LightState CaptureLightState(const Light &l) {

    return {l.col, l.att, l.ambient, l.inner, l.outer, l.falloff, l.bias, l.pcf, l.type, l.typeID, l.hasAnimation};
}

inline void ApplyLightState(Light &l, const LightState &s) {

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

    l.hasAnimation = s.hasAnimation;
}

inline ObjectState CaptureState(const Object &o) {

    return {o.position, o.rotation, o.scale, o.parentID, o.name, o.mesh, o.obj_texture, o.tex, o.min, o.max, o.mapping, o.coordType, 
        o.cyaxisType, o.normalTexture, o.norm_map, o.hasNormalMap, o.shininess, o.physics,  o.hasPhysics, o.collider, o.colliderTransform, 
        o.hasCollider, o.hasMaterial, o.hasMapping, o.hasAnimation, o.tiling, o.visible, o.renderBothSides};
}

inline void ApplyState(Object &o, const ObjectState &s) {

    o.position = s.position;
    o.rotation = s.rotation;
    o.scale = s.scale;
    o.parentID = s.parentID;

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
    o.hasNormalMap = s.hasNormalMap;

    o.shininess = s.shininess;

    o.physics = s.physics;
    o.hasPhysics = s.hasPhysics;

    o.collider = s.collider;
    o.colliderTransform = s.colliderTransform;
    o.hasCollider = s.hasCollider;

    o.hasMaterial = s.hasMaterial;
    o.hasMapping = s.hasMapping;

    o.hasAnimation = s.hasAnimation;

    o.tiling = s.tiling;

    o.visible = s.visible;

    o.renderBothSides = s.renderBothSides;
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
    GLint roughnessMap;
    GLint metallicMap;
    GLint emissionMap;
    GLint opacityMap;

    GLint numLights;

    GLint light_active;
    GLint normal_map_active;
    GLint shadow_active;

    GLint cameraPos;

    GLint hasNormalMap;
    GLint hasRoughnessMap;
    GLint hasMetallicMap;
    GLint packedMetallicRoughnessMap;
    GLint hasEmissionMap;
    GLint hasOpacityMap;
    GLint materialRoughness;
    GLint materialMetallic;
    GLint materialEmission;
    GLint materialOpacity;

    GLint program_line_color;

    GLint skybox_view;

    GLint skybox_projection;

    GLint is_hdr;

    GLint program_skybox_skybox;

    GLint color;

    GLint useTexture;

    GLint textureTiling;

    GLint mapping;

    GLint min;
    GLint max;

    GLint worldSize;

    GLint tiling;

    GLint renderBothSides;

    bool class_uniforms_initialized = false;
};

//--------------------------------------------------------------
// animations

enum class AnimationTargetType {
    Object,
    Light
};

enum class AnimationProperty {

    // object/light object

    Position,
    Rotation,
    Scale,

    // light

    Color,
    Ambient,
    Attenuation,
    Inner,
    Outer,
    Falloff,

    // object/light object

    Visible,

    // object

    Texture
};

using AnimationValue = std::variant<float, int, bool, glm::vec3, glm::quat, std::string>;

struct AnimationKey {

    float time = 0.0f;
    AnimationValue value;
};

struct AnimationTrack {

    AnimationTargetType targetType;

    EntityID targetID;

    AnimationProperty property;

    std::vector<AnimationKey> keys;
};

struct AnimationClip {

    std::string name;

    float duration = 5.0f;

    bool loop = true;

    std::vector<AnimationTrack> tracks;
};

inline std::vector<AnimationClip> animations;

using AnimationPlayerID = uint32_t;

struct AnimationPlayer {

    AnimationPlayerID id;

    int current_animation = -1;

    float time = 0.0f;

    bool playing = false;

    float speed = 1.f;
};

struct AnimationEditorState {

    float timelineViewMin = 0.0f;
    float timelineViewMax = 5.0f;

    float timelinePixelsPerSecond = 100.0f;
    float timelineScrollTime = 0.0f;
    float timelineTrackScrollY = 0.0f;

    int selectedTrack = -1;

    int selectedKey = -1;

    bool draggingKey = false;

    float dragOffsetTime = 0.0f;

    float animationSnap = 0.10f;

    bool animationSnapEnable = true;
};

inline AnimationEditorState animationEditor;

inline std::vector<AnimationPlayer> animationPlayers;
inline AnimationPlayerID nextAnimationPlayerID = 1;

inline AnimationPlayer animationPreviewPlayer;

float InterpolateFloat(const AnimationKey &a, const AnimationKey &b, float time);

glm::vec3 InterpolateVec3(const AnimationKey &a, const AnimationKey &b, float time);

glm::quat InterpolateQuat(const AnimationKey &a, const AnimationKey &b, float time);

const AnimationKey *GetPrevKey(const AnimationTrack &track, float time);

const AnimationKey *GetNextKey(const AnimationTrack &track, float time);

AnimationTrack *FindTrack(AnimationClip &clip, AnimationTargetType targetType, EntityID targetID,
    AnimationProperty property);

AnimationTrack &GetOrCreateTrack(AnimationClip &clip, AnimationTargetType targetType, EntityID targetID,
     AnimationProperty property);

void RecalculateAnimationDuration(AnimationClip &clip);

template <typename T> void AddKey(AnimationTrack &track, float time, const T &value) {

    for (AnimationKey &key : track.keys) {

  if (std::abs(key.time - time) < 0.0001f) {

  key.value = AnimationValue(value);

  return;
  }
    }

    track.keys.push_back({time, AnimationValue(value)});

    std::sort(track.keys.begin(), track.keys.end(),
  [](const AnimationKey &a, const AnimationKey &b) { return a.time < b.time; });
}

AnimationValue EvaluateTrack(const AnimationTrack &track, float time);

const char *AnimationPropertyName(AnimationProperty property);

float TimelineTimeToX(float time, float startX, float pixelsPerSecond, float scrollTime);

float TimelineXToTime(float x, float startX, float pixelsPerSecond, float scrollTime);

AnimationPlayerID PlayAnimation(int animationIndex);

int FindAnimationByName(const std::string &name);
std::string AnimationReferenceIdentifier(int animationIndex);

//--------------------------------------------------------------
// events

inline VideoPlayer videoPlayer;
inline bool isCinematicActive = false;

enum class EventType {
    PlaySound,
    StopSound,
    PlayAnimation,
    StopAnimation,
    StartCinematic
};

struct Event {

    virtual ~Event() = default;

    virtual EventType GetType() const = 0;
};

struct PlaySoundEvent : public Event {

    PlaySoundEvent(std::string p, float v) : path(p), volume(v) {}

    EventType GetType() const override {

        return EventType::PlaySound;
    }

    std::string path;
    float volume;
};

int FindAnimations(std::string name, EntityID objId);

//
// struct StopSoundEvent : public Event{
//
//    StopSoundEvent(std::string p, float v) : path(p), volume(v) {}
//
//    EventType GetType() const override {
// 
//      return EventType::StopSound;
//    }
//
//    std::string path;
//    float volume;
//};

struct PlayAnimationEvent : public Event {

    PlayAnimationEvent(const std::string &animation) : animation(animation) {}

    EventType GetType() const override {

        return EventType::PlayAnimation;
    }

    std::string animation;
};

struct PlayCinematicEvent : public Event {

    PlayCinematicEvent(const std::string &p) : path(p) {}

    EventType GetType() const override {

         return EventType::StartCinematic;
    }

    std::string path;
};

struct EventManager {

    void Throw(std::unique_ptr<Event> event) {

        events.push_back(std::move(event));
    }

    void HandleEvent(Event &event) {

        switch (event.GetType()) {

            case EventType::PlaySound: {
            
                PlaySoundEvent &soundEvent = static_cast<PlaySoundEvent &>(event);
                
                // missing
                
                break;
            }
            
            case EventType::PlayAnimation: {
            
                PlayAnimationEvent &animEvent = static_cast<PlayAnimationEvent &>(event);
                
                int animationIndex = FindAnimationByName(animEvent.animation);
                
                if (animationIndex >= 0)
                  PlayAnimation(animationIndex);
                
                break;
            }
            
            case EventType::StartCinematic: {
            
                PlayCinematicEvent &cinematicEvent = static_cast<PlayCinematicEvent &>(event);
                
                videoPlayer.Open(cinematicEvent.path.c_str());
                videoPlayer.Play();
                
                isCinematicActive = true;
                
                break;
            }
          }
    }

    void Dispatch() {

        for (auto &event : events) {

            HandleEvent(*event);
        }

          events.clear();
    }

  private:
    std::vector<std::unique_ptr<Event>> events;
};

inline EventManager eventManager;
