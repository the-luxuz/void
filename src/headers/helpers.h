#pragma once

#include "local.h"
#include "parser.h"
#include "classes.h"
#include "programs.h"
#include "textures.h"
#include "inits.h"

extern GLuint viewportFBO;
extern GLuint dynamic_cubemap;

extern GLuint envCubemap;

extern GLuint captureFBO;
extern GLuint captureRBO;

extern camera cam;
extern Parser par;
extern Settings st;

extern bool quit;

extern const float STEP;

extern ImVec2 viewportPos;
extern ImVec2 viewportSize;

extern GLuint gridVAO;
extern GLuint gridVBO;

extern int gridVertexCount;

extern float moveSpeed;
extern float mouseSensitivity;

extern int selectedObject;

extern GLuint skyVAO;
extern GLuint skyVBO;

extern glm::mat4 view;
extern glm::mat4 projection;

extern glm::vec3 rayOrigin;
extern glm::vec3 rayDir;

extern std::vector<Object> objects;

extern GLuint skyboxVAO;
extern GLuint skyboxVBO;

extern glm::mat4 captureProjection;

extern glm::mat4 captureViews[];

extern std::vector<std::filesystem::path> pathTextures;

extern std::string dirPath;

extern std::filesystem::path rootFolder;
extern std::filesystem::path currentFolder;

extern std::unordered_map<std::string, int> objectNameCounters;

extern bool reloadFolder;
extern std::filesystem::path newFolder;

extern std::vector<std::filesystem::directory_entry> folderEntries;

extern glm::ivec2 windowedSize;

extern std::string save_outPath;

extern glm::vec3 cameraPos;

extern bool dont_reload_cam;

extern bool face_normal;

extern bool normals_appear;
extern bool tangent_appear;
extern bool bitangent_appear;

extern std::vector<Light> lights;

extern std::vector<Object> lightObjects;

extern glm::mat4 lightVP[8];

extern bool normal_apper;
extern bool tangent_apper;
extern bool bitangent_apper;

extern bool tex_appear;

extern int render_modes;

extern bool shadow_map_render;

void DeleteObjectGPU(Object& o);

void clean();

void Docking();

void ResizeViewport(int width, int height);

void ApplyWindowMode(SDL_Window* window);

void FixCylinder(Object& obj);

void FixSphere(Object& obj);

void RestoreOriginalUV(Object& obj);

void GeneratePlanarUV(Object& obj, coord_types coordType);

void GenerateCubeUV(Object& obj);
void GenerateCylindricalUV(Object& obj, cylinder_axis axis);

void GenerateSphericalUV(Object& obj);

void UpdateMapping(Object& obj);

void ReloadFolder();

std::string GenerateObjectName(const std::string& baseName);

void read_textures_path();

Object* GetSelectedObject();

glm::mat4 GetModelMatrix(Object& obj);

void gen_cubemap();

void ConvertHDRToCubemap();

void GenSkybox();

glm::vec3 get_cam_pos();

bool RayVsAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& boxMin, const glm::vec3& boxMax, float& t);

void BuildRay(float mouseX, float mouseY, glm::vec3& rayOrigin, glm::vec3& rayDir);

void select_object(glm::vec3& rayOrigin, glm::vec3& rayDir);

void controls(const bool* keyboard, float angle_step, float radius_step, float dt);

void Create_Grid();

void Build_Normals(Object& obj);

void Build_TBN(Object& obj);

void CreateLineBuffer(GLuint& vao, GLuint& vbo, const std::vector<LineVertex>& data);

void Calculate_Average_Normals(Object& obj);

void Calculate_Average_TBN(Object& obj);

glm::vec3 Compute_Normal(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c);

void Add_Vertex(Object& obj, const Vertex& v);

void Add_Vertex(Light& lgh, const Vertex& v);

void Add_Tri(Object& obj, const Vertex& in_v0, const Vertex& in_v1, const Vertex& in_v2);

void Add_Tri(Light& lgh, const  Vertex& in_v0, const  Vertex& in_v1, const  Vertex& in_v2);