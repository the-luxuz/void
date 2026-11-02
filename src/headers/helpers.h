#pragma once

#include "classes.h"
#include "inits.h"
#include "local.h"
#include "parser.h"
#include "programs.h"
#include "scene_hierarchy.h"
#include "textures.h"

// structures
inline camera cam;
inline Parser par;
inline Settings st;

// buffers
inline GLuint viewportFBO = 0;
inline GLuint dynamic_cubemap = 0;

inline GLuint envCubemap = 0;

inline GLuint captureFBO = 0;
inline GLuint captureRBO = 0;

inline GLuint gridVAO = 0;
inline GLuint gridVBO = 0;

inline GLuint skyVAO = 0;
inline GLuint skyVBO = 0;

inline GLuint skyboxVAO = 0;
inline GLuint skyboxVBO = 0;

// viewport
inline ImVec2 viewportPos{};
inline ImVec2 viewportSize{};

// vectors
inline std::vector<Object> objects;
inline std::vector<Light> lights;
inline std::vector<Object> lightObjects;

inline std::vector<Object> saved_objects;
inline std::vector<Object> saved_light_objects;
inline std::vector<Light> saved_lights;

// light view-proj matrices
inline glm::mat4 lightVP[MAX_LIGHTS]{};

inline glm::mat4 view{1.0f};
inline glm::mat4 projection{1.0f};

// selected object and ray casting
inline int selectedObject = -1;

inline glm::vec3 rayOrigin{};
inline glm::vec3 rayDir{};

// movement
inline float moveSpeed = 1000.0f;
inline float mouseSensitivity = 0.004f;

// skybox
inline glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
inline glm::mat4 captureViews[] = {
    glm::lookAt(glm::vec3(0), glm::vec3(1, 0, 0), glm::vec3(0, -1, 0)),
    glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0)),
    glm::lookAt(glm::vec3(0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
    glm::lookAt(glm::vec3(0), glm::vec3(0, -1, 0), glm::vec3(0, 0, -1)),
    glm::lookAt(glm::vec3(0), glm::vec3(0, 0, 1), glm::vec3(0, -1, 0)),
    glm::lookAt(glm::vec3(0), glm::vec3(0, 0, -1), glm::vec3(0, -1, 0))};

// grid
inline constexpr float STEP = 25.0f;
inline constexpr float cellSize = 90.0f;
inline int gridVertexCount = 0;

// folder and textures
inline std::string dirPath = "data/textures";

inline std::filesystem::path rootFolder = "../data";
inline std::filesystem::path currentFolder = rootFolder;

inline std::vector<std::filesystem::path> pathTextures;

inline bool reloadFolder = false;
inline std::filesystem::path newFolder;
inline std::vector<std::filesystem::directory_entry> folderEntries;

inline std::string save_outPath;

// unordered mapo names
inline std::unordered_map<std::string, int> objectNameCounters;

// undo/redo
inline Object objCopy;
inline Light lightCopy;

inline bool hasObjectCopy = false;
inline bool hasLightCopy = false;

inline ObjectState prevState;
inline LightState prevLightState;

// camera pos
inline glm::vec3 cameraPos{};

// windowed size
inline glm::ivec2 windowedSize{};

// flags
inline bool dont_reload_cam = false;

inline bool face_normal = false;

inline bool normals_appear = false;

inline bool shadow_map_render = true;

inline bool quit = false;

// render modes
inline int render_modes = 0;

// tools
enum class Tools {
    TRANSLATE,
    ROTATE,
    SCALE
};

enum class view_Mode {
    EDITOR,
    PLAY
};

inline view_Mode currentViewMode = view_Mode::EDITOR;

inline view_Mode prevViewMode = currentViewMode;

inline Tools currentTool = Tools::TRANSLATE;

inline bool camera_update = true;

inline bool free_cam = false;

inline bool viewport_scroll_active = false;

inline VideoQuad videoQuad;

inline SDL_AudioSpec spec{};

//--------------------------------------------------------------------------
// Load scene in the good way

bool load_scene_ok(const char *scene);

//--------------------------------------------------------------------------
// ImGui docking

inline bool animationWindow = false;

void Docking();

//--------------------------------------------------------------------------
// Viewport update size

void ResizeViewport(int width, int height);

void ApplyWindowMode(SDL_Window *window);

//--------------------------------------------------------------------------
// Mapping

void FixCylinder(Object &obj);

void FixSphere(Object &obj);

void RestoreOriginalUV(Object &obj);

void GeneratePlanarUV(Object &obj, coord_types coordType);

void GenerateCubeUV(Object &obj);

void GenerateCylindricalUV(Object &obj, cylinder_axis axis);

void GenerateSphericalUV(Object &obj);

void UpdateMapping(Object &obj);

//--------------------------------------------------------------------------
// Reload folder

void ReloadFolder();

//--------------------------------------------------------------------------
// Obj names

std::string GenerateObjectName(const std::string &baseName);

//--------------------------------------------------------------------------
// Reload textures path

void read_textures_path();

//--------------------------------------------------------------------------
// Skybox and cubemap

void ConvertHDRToCubemap();

void gen_cubemap();

void GenSkybox();

//--------------------------------------------------------------------------
// Getters

glm::vec3 get_cam_pos();

Object *GetSelectedObject();

Light *GetSelectedLight();
float GetLightRange(const Light& light);
void SetLightRange(Light &light, float range);

//--------------------------------------------------------------------------
// Ray-AABB intersection test

bool RayVsAABB(const glm::vec3 &rayOrigin, const glm::vec3 &rayDir, const glm::vec3 &boxMin, const glm::vec3 &boxMax, float &t);

void BuildRay(float mouseX, float mouseY, glm::vec3 &rayOrigin, glm::vec3 &rayDir);

//--------------------------------------------------------------------------
// Select obj

void select_object(glm::vec3 &rayOrigin, glm::vec3 &rayDir);

//--------------------------------------------------------------------------
// Controls

void controls(const bool *keyboard, float angle_step, float radius_step, float dt);

//--------------------------------------------------------------------------
// Grid

void Create_Grid();

//--------------------------------------------------------------------------
// Lines

void Build_Normals(Object &obj);

void Build_TBN(Object &obj);

void CreateLineBuffer(GLuint &vao, GLuint &vbo, const std::vector<LineVertex> &data);

void Calculate_Average_Normals(Object &obj);

void Calculate_Average_TBN(Object &obj);

glm::vec3 Compute_Normal(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c);

//--------------------------------------------------------------------------
// Vertex and triangle helpers

void Add_Vertex(Object &obj, const Vertex &v);

void Add_Vertex(Light &lgh, const Vertex &v);

void Add_Tri(Object &obj, const Vertex &in_v0, const Vertex &in_v1, const Vertex &in_v2);

void Add_Tri(Light &lgh, const Vertex &in_v0, const Vertex &in_v1, const Vertex &in_v2);

//--------------------------------------------------------------------------
// Create Lights

Object CreateEmptyObject();

void CreateLight(const std::string &type);

//--------------------------------------------------------------------------
// Clean buffers and objects

void clean();
void DestroyBuffers(Object &obj);

//--------------------------------------------------------------------------
// Mesh replacement

void ReplaceMesh(Object &obj, const std::string &path);

//--------------------------------------------------------------------------
// Undo/Redo

void HandleInspectorUndo(Object &obj);

void HandleInspectorUndo(Light &light, Object &lightObj);

//--------------------------------------------------------------------------
// Physics

void AddForce(Object &obj, const glm::vec3 &force);

//--------------------------------------------------------------------------
// Collisions

bool CanCollide(const Object &a, const Object &b);

Contact Sphere_vs_Sphere(const Object &a, const Object &b);
Contact Sphere_vs_Plane(const Object &a, const Object &b);
Contact Sphere_vs_AABB(const Object &a, const Object &b);
Contact AABB_vs_AABB(const Object &a, const Object &b);
Contact AABB_vs_Plane(const Object &a, const Object &b);

void PositionalCorrection(Object &a, Object &b, Contact &c);

void ResolveVelocity(Object &a, Object &b, Contact &contact);

float GetWorldRadius(const Object &obj);

//--------------------------------------------------------------------------

void save_scene_state();

void load_scene_state();

Object *search_obj_by_name(const char *name);

glm::vec2 TextureTiling(Object &obj, glm::vec3 worldSize);

//--------------------------------------------------------------
// animations

Light *FindLightByID(EntityID id);

void ApplyTrack(const AnimationTrack &track, float time);

void UpdateAnimationPlayer(AnimationPlayer &player, float dt);

void EvaluateAnimationPlayer(AnimationPlayer &player);

void EvaluateCurrentAnimation();

std::string AnimationTrackTargetName(const AnimationTrack &track);

void PlayAllAnims();

void StopAllAnims();

//--------------------------------------------------------------
// snap

float Snap(float time, float step);

void BuildImportedTBN(Object &obj);
