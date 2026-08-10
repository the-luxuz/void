#pragma once

#include "local.h"
#include "parser.h"
#include "classes.h"
#include "programs.h"
#include "textures.h"
#include "inits.h"

//--------------------------------------------------------------------------
// global variables

// structures
extern camera cam;
extern Parser par;
extern Settings st;

// buffers
extern GLuint viewportFBO;
extern GLuint dynamic_cubemap;

extern GLuint envCubemap;

extern GLuint captureFBO;
extern GLuint captureRBO;

extern GLuint gridVAO;
extern GLuint gridVBO;

extern GLuint skyVAO;
extern GLuint skyVBO;

extern GLuint skyboxVAO;
extern GLuint skyboxVBO;

// viewport
extern ImVec2 viewportPos;
extern ImVec2 viewportSize;

// vectors
extern std::vector<Object> objects;
extern std::vector<Light> lights;
extern std::vector<Object> lightObjects;

// light view-proj matrices
extern glm::mat4 lightVP[8];

extern glm::mat4 view;
extern glm::mat4 projection;

// selected object and ray casting
extern int selectedObject;

extern glm::vec3 rayOrigin;
extern glm::vec3 rayDir;

// movement
extern float moveSpeed;
extern float mouseSensitivity;

// skybox
extern glm::mat4 captureProjection;
extern glm::mat4 captureViews[];

// grid
extern const float STEP;
extern const float cellSize;
extern int gridVertexCount;

// folder and textures
extern std::string dirPath;

extern std::filesystem::path rootFolder;
extern std::filesystem::path currentFolder;

extern std::vector<std::filesystem::path> pathTextures;

extern bool reloadFolder;
extern std::filesystem::path newFolder;
extern std::vector<std::filesystem::directory_entry> folderEntries;

extern std::string save_outPath;

// unordered mapo names
extern std::unordered_map<std::string, int> objectNameCounters;

// undo/redo
extern Object objCopy;
extern Light lightCopy;

extern bool hasObjectCopy;
extern bool hasLightCopy;

extern ObjectState prevState;
extern LightState prevLightState;

// camera pos
extern glm::vec3 cameraPos;

// windowed size
extern glm::ivec2 windowedSize;

// flags
extern bool dont_reload_cam;

extern bool face_normal;

extern bool normals_appear;
extern bool tangent_appear;
extern bool bitangent_appear;

extern bool normal_apper;
extern bool tangent_apper;
extern bool bitangent_apper;

extern bool tex_appear;

extern bool shadow_map_render;

extern bool quit;

// render modes
extern int render_modes;

// tools
enum class Tools {
	TRANSLATE,
	ROTATE,
	SCALE
};

extern Tools currentTool;

//--------------------------------------------------------------------------
// ImGui docking

void Docking();

//--------------------------------------------------------------------------
// Viewport update size

void ResizeViewport(int width, int height);

void ApplyWindowMode(SDL_Window* window);

//--------------------------------------------------------------------------
// Mapping

void FixCylinder(Object& obj);

void FixSphere(Object& obj);

void RestoreOriginalUV(Object& obj);

void GeneratePlanarUV(Object& obj, coord_types coordType);

void GenerateCubeUV(Object& obj);

void GenerateCylindricalUV(Object& obj, cylinder_axis axis);

void GenerateSphericalUV(Object& obj);

void UpdateMapping(Object& obj);

//--------------------------------------------------------------------------
// Reload folder

void ReloadFolder();

//--------------------------------------------------------------------------
// Obj names

std::string GenerateObjectName(const std::string& baseName);

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

Object* GetSelectedObject();

Light* GetSelectedLight();

glm::mat4 GetModelMatrix(Object& obj);

//--------------------------------------------------------------------------
// Ray-AABB intersection test

bool RayVsAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& boxMin, const glm::vec3& boxMax, float& t);

void BuildRay(float mouseX, float mouseY, glm::vec3& rayOrigin, glm::vec3& rayDir);

//--------------------------------------------------------------------------
// Select obj

void select_object(glm::vec3& rayOrigin, glm::vec3& rayDir);

//--------------------------------------------------------------------------
// Controls

void controls(const bool* keyboard, float angle_step, float radius_step, float dt);

//--------------------------------------------------------------------------
// Grid

void Create_Grid();

//--------------------------------------------------------------------------
// Lines

void Build_Normals(Object& obj);

void Build_TBN(Object& obj);

void CreateLineBuffer(GLuint& vao, GLuint& vbo, const std::vector<LineVertex>& data);

void Calculate_Average_Normals(Object& obj);

void Calculate_Average_TBN(Object& obj);

glm::vec3 Compute_Normal(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c);

//--------------------------------------------------------------------------
// Vertex and triangle helpers

void Add_Vertex(Object& obj, const Vertex& v);

void Add_Vertex(Light& lgh, const Vertex& v);

void Add_Tri(Object& obj, const Vertex& in_v0, const Vertex& in_v1, const Vertex& in_v2);

void Add_Tri(Light& lgh, const  Vertex& in_v0, const  Vertex& in_v1, const  Vertex& in_v2);

//--------------------------------------------------------------------------
// Create Lights

void CreateLight(const std::string& type);

//--------------------------------------------------------------------------
// Clean buffers and objects

void clean();
void DestroyBuffers(Object& obj);

template<typename T>

void DeleteObjectGPU(T& o) {

	if (o.ebo)
		glDeleteBuffers(1, &o.ebo);

	if (o.vbo)
		glDeleteBuffers(1, &o.vbo);

	if (o.vao)
		glDeleteVertexArrays(1, &o.vao);
}

//--------------------------------------------------------------------------
// Mesh replacement

void ReplaceMesh(Object& obj, const std::string& path);

//--------------------------------------------------------------------------
// Undo/Redo

void HandleInspectorUndo(Object& obj);

void HandleInspectorUndo(Light& light, Object& lightObj);