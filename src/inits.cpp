#include "headers/inits.h"
#include "headers/scene_hierarchy.h"

#include <filesystem>
#include <limits>
#include <unordered_map>

//--------------------------------------------------------------------------
// init functions

void init_uniforms() {

    // grid shader uniforms

    shader_uniform.grid_MVP = glGetUniformLocation(program_grid, "uMVP");

    // main shader uniforms

    shader_uniform.program_texture = glGetUniformLocation(program, "tex");

    shader_uniform.color = glGetUniformLocation(program, "color");

    shader_uniform.useTexture = glGetUniformLocation(program, "useTexture");

    shader_uniform.tiling = glGetUniformLocation(program, "tiling");

    shader_uniform.mapping = glGetUniformLocation(program, "mapping");

    shader_uniform.min = glGetUniformLocation(program, "objMin");
    shader_uniform.max = glGetUniformLocation(program, "objMax");

    shader_uniform.worldSize = glGetUniformLocation(program, "worldSize");

    shader_uniform.textureTiling = glGetUniformLocation(program, "textureTiling");

    shader_uniform.lightVP = glGetUniformLocation(program, "lightVP");

    shader_uniform.normalMap = glGetUniformLocation(program, "normalMap");
    shader_uniform.roughnessMap = glGetUniformLocation(program, "roughnessMap");
    shader_uniform.metallicMap = glGetUniformLocation(program, "metallicMap");
    shader_uniform.emissionMap = glGetUniformLocation(program, "emissionMap");
    shader_uniform.opacityMap = glGetUniformLocation(program, "opacityMap");

    shader_uniform.numLights = glGetUniformLocation(program, "numLights");

    shader_uniform.light_active = glGetUniformLocation(program, "light_active");
    shader_uniform.normal_map_active = glGetUniformLocation(program, "normal_map_active");
    shader_uniform.shadow_active = glGetUniformLocation(program, "shadow_active");

    shader_uniform.cameraPos = glGetUniformLocation(program, "cameraPos");

    shader_uniform.hasNormalMap = glGetUniformLocation(program, "hasNormalMap");
    shader_uniform.hasRoughnessMap = glGetUniformLocation(program, "hasRoughnessMap");
    shader_uniform.hasMetallicMap = glGetUniformLocation(program, "hasMetallicMap");
    shader_uniform.hasEmissionMap = glGetUniformLocation(program, "hasEmissionMap");
    shader_uniform.hasOpacityMap = glGetUniformLocation(program, "hasOpacityMap");
    shader_uniform.materialRoughness = glGetUniformLocation(program, "materialRoughness");
    shader_uniform.materialMetallic = glGetUniformLocation(program, "materialMetallic");
    shader_uniform.materialEmission = glGetUniformLocation(program, "materialEmission");
    shader_uniform.materialOpacity = glGetUniformLocation(program, "materialOpacity");

    // line shader uniforms

    shader_uniform.program_line_color = glGetUniformLocation(program_line, "color");

    // skybox uniforms

    shader_uniform.skybox_view = glGetUniformLocation(program_skybox, "view");

    shader_uniform.skybox_projection = glGetUniformLocation(program_skybox, "projection");

    shader_uniform.is_hdr = glGetUniformLocation(program_skybox, "is_hdr");

    shader_uniform.program_skybox_skybox = glGetUniformLocation(program_skybox, "skybox");
}

void init_class_uniforms() {

    for (int i = 0; i < MAX_LIGHTS; i++) {

        std::string idx = std::to_string(i);

        shader_uniform.depthdata[i] = glGetUniformLocation(program, ("depthData[" + idx + "]").c_str());

        shader_uniform.light_type[i] = glGetUniformLocation(program, ("lights[" + idx + "].type").c_str());
        shader_uniform.light_pos[i] = glGetUniformLocation(program, ("lights[" + idx + "].position").c_str());
        shader_uniform.light_dir[i] = glGetUniformLocation(program, ("lights[" + idx + "].direction").c_str());
        shader_uniform.light_col[i] = glGetUniformLocation(program, ("lights[" + idx + "].color").c_str());
        shader_uniform.light_att[i] = glGetUniformLocation(program, ("lights[" + idx + "].attenuation").c_str());
        shader_uniform.light_ambient[i] = glGetUniformLocation(program, ("lights[" + idx + "].ambient").c_str());
        shader_uniform.light_bias[i] = glGetUniformLocation(program, ("lights[" + idx + "].bias").c_str());
        shader_uniform.light_pcf[i] = glGetUniformLocation(program, ("lights[" + idx + "].pcf").c_str());
        shader_uniform.light_inner[i] = glGetUniformLocation(program, ("lights[" + idx + "].inner").c_str());
        shader_uniform.light_outer[i] = glGetUniformLocation(program, ("lights[" + idx + "].outer").c_str());
        shader_uniform.light_falloff[i] = glGetUniformLocation(program, ("lights[" + idx + "].falloff").c_str());
    }
}

void init_viewport() {

    for (int i = 0; i < MAX_LIGHTS; i++) {

        glGenFramebuffers(1, &depthFBO[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, depthFBO[i]);

        Create_depth_tex(viewport_width, viewport_height, i);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    glGenFramebuffers(1, &viewportFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, viewportFBO);

    Create_viewport_tex(viewport_width, viewport_height);

    CreateViewportDepthTexture(st.w, st.h);

    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer incompleto\n";

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void init_grid() {

    Create_Grid();
}

void init_skybox(const char *skyboxFile, bool create_vaos) {

    if (create_vaos)
        GenSkybox();

    Load_Skybox(skyboxFile);

    if (create_vaos)
        gen_cubemap();

    ConvertHDRToCubemap();
}

void init_camera() {

    cam.fov = st.fov;

    glm::vec3 offset = cam.cam_position - cam.cam_target;

    cam.radius = glm::length(offset);

    cam.alpha = acos(offset.y / cam.radius);
    cam.beta = atan2(offset.z, offset.x);
}

void init_obj(const char *objFile, Object &obj) {

    tinyobj::attrib_t attrib;

    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string err;

    const std::filesystem::path objectPath(objFile);

    const std::string materialDirectory = objectPath.parent_path().generic_string() + "/";

    bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, objFile, materialDirectory.c_str());

    if (!err.empty())
        std::cout << err << std::endl;

    if (!success)
        return;

    obj.vertices.clear();
    obj.indices.clear();
    obj.submeshes.clear();

    struct VertexKey {

        int position;
        int texcoord;
        int normal;

        bool operator==(const VertexKey &) const = default;
    };

    struct VertexKeyHash {

        size_t operator()(const VertexKey &key) const {

            size_t hash = std::hash<int>{}(key.position);

            hash ^= std::hash<int>{}(key.texcoord) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<int>{}(key.normal) + 0x9e3779b9 + (hash << 6) + (hash >> 2);

            return hash;
        }
    };

    std::unordered_map<VertexKey, GLuint, VertexKeyHash> vertexLookup;

    auto addVertex = [&](const tinyobj::index_t &index) {

        const VertexKey key{index.vertex_index, index.texcoord_index, index.normal_index};

        if (const auto found = vertexLookup.find(key); found != vertexLookup.end())
            return found->second;

        Vertex vertex{};

        vertex.position = {attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1], attrib.vertices[3 * index.vertex_index + 2]};

        if (index.texcoord_index >= 0)
            vertex.uv = {attrib.texcoords[2 * index.texcoord_index + 0], attrib.texcoords[2 * index.texcoord_index + 1]};

        vertex.originalUV = vertex.uv;

        if (index.normal_index >= 0)
            vertex.normal = {attrib.normals[3 * index.normal_index + 0], attrib.normals[3 * index.normal_index + 1], attrib.normals[3 * index.normal_index + 2]};

        const GLuint vertexIndex = static_cast<GLuint>(obj.vertices.size());

        obj.vertices.push_back(vertex);

        vertexLookup.emplace(key, vertexIndex);

        return vertexIndex;
    };

    auto createMaterial = [&](int materialId) {

        Material material{};

        if (materialId < 0 || materialId >= static_cast<int>(materials.size()))
            return material;

        const auto &source = materials[materialId];

        material.name = source.name;

        material.diffuse = {source.diffuse[0], source.diffuse[1], source.diffuse[2]};
        material.specular = {source.specular[0], source.specular[1], source.specular[2]};
        material.emission = {source.emission[0], source.emission[1], source.emission[2]};
        material.shininess = source.shininess;
        material.opacity = source.dissolve;
        material.roughness = source.roughness > 0.0f ? source.roughness : glm::clamp(std::sqrt(2.0f / (source.shininess + 2.0f)), 0.04f, 1.0f);
        material.metallic = glm::clamp(source.metallic, 0.0f, 1.0f);

        auto resolveMaterialTexture = [&](const std::string &reference) -> std::filesystem::path {

            if (reference.empty())
                return {};

            std::string fixedReference = reference;

            std::replace(fixedReference.begin(), fixedReference.end(), '\\', '/');

            std::filesystem::path ref(fixedReference);

            if (ref.is_absolute()) {

                if (std::filesystem::exists(ref))
                    return ref.lexically_normal();

                std::filesystem::path fallback = objectPath.parent_path() / ref.filename();

                if (std::filesystem::exists(fallback))
                    return fallback.lexically_normal();

                return ref.lexically_normal();
            }

            std::filesystem::path resolved = objectPath.parent_path() / ref;

            return resolved.lexically_normal();
        };

        auto loadTexture = [&](const std::string &reference, std::string &path, GLuint &texture, bool sRGB,
                               const char *semantic) {
            if (reference.empty())
                return;

            const std::filesystem::path resolved = resolveMaterialTexture(reference);

            if (!std::filesystem::exists(resolved)) {

                std::cout << "Material '" << source.name << "': missing " << semantic << " texture: "  << resolved.generic_string() << '\n';
                return;
            }

            path = resolved.generic_string();
            texture = TextureManager::Get(path, sRGB);
        };

        loadTexture(source.diffuse_texname, material.albedoPath, material.albedoTexture, false, "albedo");
        loadTexture(!source.normal_texname.empty() ? source.normal_texname : source.bump_texname, material.normalPath, material.normalTexture, false, "normal");
        loadTexture(source.roughness_texname, material.roughnessPath, material.roughnessTexture, false, "roughness");
        loadTexture(source.metallic_texname, material.metallicPath, material.metallicTexture, false, "metallic");
        loadTexture(source.emissive_texname, material.emissionPath, material.emissionTexture, false, "emission");
        loadTexture(source.alpha_texname, material.opacityPath, material.opacityTexture, false, "opacity");

        return material;
    };

    glm::vec3 min(FLT_MAX);
    glm::vec3 max(-FLT_MAX);

    std::vector<Material> importedMaterials(materials.size());
    std::vector<bool> materialLoaded(materials.size(), false);

    for (const auto &shape : shapes) {

        const auto &indices = shape.mesh.indices;

        int activeMaterialId = std::numeric_limits<int>::min();

        Submesh *activeSubmesh = nullptr;

        for (size_t face = 0; face * 3 + 2 < indices.size(); ++face) {

            const int materialId = face < shape.mesh.material_ids.size() ? shape.mesh.material_ids[face] : -1;

            if (materialId != activeMaterialId) {

                Submesh submesh{};

                submesh.firstIndex = static_cast<GLuint>(obj.indices.size());

                if (materialId >= 0 && materialId < static_cast<int>(materials.size())) {

                    const size_t index = static_cast<size_t>(materialId);

                    if (!materialLoaded[index]) {

                        importedMaterials[index] = createMaterial(materialId);

                        materialLoaded[index] = true;
                    }

                    submesh.material = importedMaterials[index];
                }
                submesh.min = glm::vec3(FLT_MAX);
                submesh.max = glm::vec3(-FLT_MAX);
                obj.submeshes.push_back(std::move(submesh));
                activeSubmesh = &obj.submeshes.back();
                activeMaterialId = materialId;
            }

            for (size_t corner = 0; corner < 3; ++corner) {
                const GLuint vertexIndex = addVertex(indices[face * 3 + corner]);
                obj.indices.push_back(vertexIndex);
                activeSubmesh->indexCount++;
                const glm::vec3 &position = obj.vertices[vertexIndex].position;
                activeSubmesh->min = glm::min(activeSubmesh->min, position);
                activeSubmesh->max = glm::max(activeSubmesh->max, position);
            }
        }
    }

    BuildImportedTBN(obj);

    for (Vertex &v : obj.vertices) {

        min = glm::min(min, v.position);
        max = glm::max(max, v.position);
    }

    obj.min = min;
    obj.max = max;

    glm::vec3 size = max - min;

    obj.collider.radius = std::max(size.x, std::max(size.y, size.z)) * 0.5f;
}

void PrepareImportedObject(Object &obj) {

    if (obj.vertices.empty())
        return;

    const glm::vec3 center = (obj.min + obj.max) * 0.5f;
    const glm::vec3 sourceSize = obj.max - obj.min;

    const float largestDimension = std::max(sourceSize.x, std::max(sourceSize.y, sourceSize.z));

    for (Vertex &vertex : obj.vertices)
        vertex.position -= center;

    for (Submesh &submesh : obj.submeshes) {

        submesh.min -= center;
        submesh.max -= center;
    }

    obj.min -= center;
    obj.max -= center;

    obj.importScale = largestDimension > 0.0001f ? 100.0f / largestDimension : 1.0f;
    obj.collider.radius *= obj.importScale;
}

bool init_lights(Object &obj) {

    init_obj("../data/objects/sphere.obj", obj);

    return true;
}

namespace {

    void RestoreMaterialTextures(Material& material) {

        material.albedoTexture = TextureManager::Get(material.albedoPath, false);
        material.normalTexture = TextureManager::Get(material.normalPath, false);
        material.roughnessTexture = TextureManager::Get(material.roughnessPath, false);
        material.metallicTexture = TextureManager::Get(material.metallicPath, false);
        material.emissionTexture = TextureManager::Get(material.emissionPath, false);
        material.opacityTexture = TextureManager::Get(material.opacityPath, false);
    }

    void RestoreSavedMaterials(Object& object, const std::vector<Material>& materials) {

        if (materials.empty())
            return;

        if (materials.size() != object.submeshes.size()) {

            std::cout << "Saved material count does not match OBJ submeshes for " << object.mesh << '\n';

            return;
        }

        for (size_t i = 0; i < materials.size(); ++i) {

            object.submeshes[i].material = materials[i];

            RestoreMaterialTextures(object.submeshes[i].material);
        }
    }
} // namespace

void init() {

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // if(!dont_reload_cam)
    // init_camera();

    init_uniforms();

    CreateVideoQuad(videoQuad);

    // if (!videoPlayer.Open("../data/cinematics/test.mp4")) {

    //	std::cout << "VIDEO FAILED\n";

    //} else {

    //	std::cout << "VIDEO OK\n";

    //	videoPlayer.Play();

    //	isCinematicActive = true;
    //}

    init_grid();

    init_skybox(st.skybox.c_str(), true);

    init_viewport();

    read_textures_path();

    ReloadFolder();

    objects.clear();

    lightObjects.clear();

    lights.clear();

    animations.clear();

    EntityID maxID = 0;

    for (auto &p : par.objects) {

        Object obj;

        const bool isEmpty = p.mesh.empty();
        if (!isEmpty) {
            init_obj(p.mesh.c_str(), obj);
            buffer(obj);
        }

        obj.id = p.id;
        obj.parentID = p.parentID;

        maxID = std::max(maxID, obj.id);

        obj.name = p.obj_name;
        obj.mesh = p.mesh;
        obj.tex = p.tex;
        obj.norm_map = p.normal_map;

        obj.position = p.position;
        obj.rotation = p.rotation;
        obj.scale = p.scale;

        obj.coordType = p.coordType;
        obj.cyaxisType = p.cyaxisType;
        obj.mapping = p.mapping;

        obj.hasNormalMap = p.hasNormalMap;

        obj.max = p.max;
        obj.min = p.min;

        obj.obj_texture = TextureManager::Get(obj.tex, false);

        obj.original_vertices = obj.vertices;
        obj.original_indices = obj.indices;

        obj.shininess = p.shininess;

        obj.normalTexture = TextureManager::Get(obj.norm_map, false);
        obj.hasNormalMap = obj.hasNormalMap && obj.normalTexture != 0;

        if (!isEmpty)
            RestoreSavedMaterials(obj, p.materials);

        obj.physics = p.physics;
        obj.hasPhysics = p.hasPhysics;

        obj.collider = p.collider;
        obj.colliderTransform = p.colliderTransform;
        obj.hasCollider = p.hasCollider;

        obj.hasMaterial = p.hasMaterial;
        obj.hasMapping = p.hasMapping;
        obj.hasAnimation = p.hasAnimation;

        obj.tiling = p.tiling;

        obj.visible = p.visible;

        obj.model = GetModelMatrix(obj);

        if (!isEmpty)
            UpdateMapping(obj);

        objects.push_back(std::move(obj));
    }

    for (Object& object : objects) {

        if (object.parentID != 0 && (!FindObjectByID(object.parentID) || object.parentID == object.id || IsDescendantOf(object.parentID, object.id)))
            object.parentID = 0;
    }

    const size_t loadedLightCount = std::min({par.lights.size(), par.lights_objects.size(), static_cast<size_t>(MAX_LIGHTS)});

    for (size_t i = 0; i < loadedLightCount; ++i) {

        Light light;

        Object obj;

        if (init_lights(obj)) {

            light.id = par.lights[i].id;

            light.name = par.lights[i].name;

            light.pos = par.lights[i].pos;
            light.dir = par.lights[i].dir;
            light.col = par.lights[i].col;
            light.att = par.lights[i].att;

            light.rotation = par.lights[i].rotation;

            light.ambient = par.lights[i].amb;

            light.bias = par.lights[i].bias;
            light.pcf = par.lights[i].pcf;

            light.inner = par.lights[i].inner;
            light.outer = par.lights[i].outer;
            light.falloff = par.lights[i].falloff;

            light.type = par.lights[i].type;

            if (light.type == "POINT")
                light.typeID = 0;

            else if (light.type == "DIR")
                light.typeID = 1;

            else if (light.type == "SPOT")
                light.typeID = 2;

            const Parser::Transform& savedObject = par.lights_objects[i];
            obj.id = savedObject.id;
            obj.parentID = savedObject.parentID;
            obj.name = savedObject.obj_name;
            obj.mesh = savedObject.mesh;
            obj.tex = savedObject.tex;
            obj.norm_map = savedObject.normal_map;
            obj.position = savedObject.position;
            obj.rotation = savedObject.rotation;
            obj.scale = savedObject.scale;
            obj.min = savedObject.min;
            obj.max = savedObject.max;
            obj.shininess = savedObject.shininess;
            obj.mapping = savedObject.mapping;
            obj.coordType = savedObject.coordType;
            obj.cyaxisType = savedObject.cyaxisType;
            obj.physics = savedObject.physics;
            obj.hasPhysics = savedObject.hasPhysics;
            obj.collider = savedObject.collider;
            obj.colliderTransform = savedObject.colliderTransform;
            obj.hasCollider = savedObject.hasCollider;
            obj.hasMaterial = savedObject.hasMaterial;
            obj.hasMapping = savedObject.hasMapping;
            obj.hasAnimation = savedObject.hasAnimation;
            obj.tiling = savedObject.tiling;
            obj.visible = savedObject.visible;
            obj.obj_texture = TextureManager::Get(obj.tex, false);
            obj.normalTexture = TextureManager::Get(obj.norm_map, false);
            obj.hasNormalMap = savedObject.hasNormalMap && obj.normalTexture != 0;

            maxID = std::max(maxID, light.id);
            maxID = std::max(maxID, obj.id);

            obj.model = GetModelMatrix(obj);

            buffer(obj);

            lights.push_back(std::move(light));

            lightObjects.push_back(std::move(obj));
        }
    }

    animations = std::move(par.anim);

    animationPlayers = std::move(par.animPlayer);
    animationEditor = par.animEditor;

    nextAnimationPlayerID = 1;
    for (const AnimationPlayer& player : animationPlayers)
        nextAnimationPlayerID = std::max(nextAnimationPlayerID, player.id + 1);

    nextEntityID = maxID + 1;
}

bool open_init(const char *settings, const char *scene, Settings &st) {

    selectedObject = -1;

    for (auto &o : objects) {

        o.undoStack.clear();
        o.redoStack.clear();

        DestroyBuffers(o);

        o.vao = 0;
        o.vbo = 0;
        o.ebo = 0;

        o.normalVAO = 0;
        o.normalVBO = 0;

        o.normalTBVAO = 0;
        o.normalTBVBO = 0;

        o.tangentVAO = 0;
        o.tangentVBO = 0;

        o.bitangentVAO = 0;
        o.bitangentVBO = 0;
    }

    for (auto &o : lightObjects) {

        o.undoStack.clear();
        o.redoStack.clear();

        DestroyBuffers(o);

        o.vao = 0;
        o.vbo = 0;
        o.ebo = 0;

        o.normalVAO = 0;
        o.normalVBO = 0;

        o.normalTBVAO = 0;
        o.normalTBVBO = 0;

        o.tangentVAO = 0;
        o.tangentVBO = 0;

        o.bitangentVAO = 0;
        o.bitangentVBO = 0;
    }

    for (auto &o : lights) {

        o.undoStack.clear();
        o.redoStack.clear();
    }

    objects.clear();
    lightObjects.clear();
    lights.clear();

    objectNameCounters.clear();

    if (par.load(settings, scene, st, cam, objects))
        init();

    return true;
}

//--------------------------------------------------------------------------
// main init function

void init_(const char *settings, const char *scene) {

    // st.win_name = "void";
    // st.win_mode = window_mode::WINDOWED;
    // st.w = 1280;
    // st.h = 720;
    // st.VSync = 0;
    // st.fov = 95;
    //
    // par.save(settings, scene, st, cam, objects, lightObjects, lights);

    if (par.load(settings, scene, st, cam, objects)) {

        avformat_network_init();

        windowedSize = {st.w, st.h};

        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {

            std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';

            return;
        }

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        window = SDL_CreateWindow(st.win_name.c_str(), st.w, st.h, SDL_WINDOW_OPENGL);

        ApplyWindowMode(window);

        context = SDL_GL_CreateContext(window);

        if (!st.VSync)
            SDL_GL_SetSwapInterval(0);
        else
            SDL_GL_SetSwapInterval(1);

        glewExperimental = GL_TRUE;

        GLenum err = glewInit();

        if (err != GLEW_OK) {

            std::cout << glewGetErrorString(err) << '\n';
            return;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO &io = ImGui::GetIO();
        (void)io;

        io.ConfigWindowsMoveFromTitleBarOnly = true;

        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGuiStyle &style = ImGui::GetStyle();

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {

            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        ImGui::StyleColorsDark();

        ImGui_ImplSDL3_InitForOpenGL(window, context);
        ImGui_ImplOpenGL3_Init("#version 330");

        create_programs();

        init();
    } else {

        st.win_name = "void";
        st.win_mode = window_mode::WINDOWED;
        st.w = 1280;
        st.h = 720;
        st.VSync = 0;
        st.fov = 95;

        avformat_network_init();

        windowedSize = {st.w, st.h};

        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {

            std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';

            return;
        }

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        window = SDL_CreateWindow(st.win_name.c_str(), st.w, st.h, SDL_WINDOW_OPENGL);

        ApplyWindowMode(window);

        context = SDL_GL_CreateContext(window);

        if (!st.VSync)
            SDL_GL_SetSwapInterval(0);
        else
            SDL_GL_SetSwapInterval(1);

        glewExperimental = GL_TRUE;

        GLenum err = glewInit();

        if (err != GLEW_OK) {

            std::cout << glewGetErrorString(err) << '\n';
            return;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO &io = ImGui::GetIO();
        (void)io;

        io.ConfigWindowsMoveFromTitleBarOnly = true;

        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGuiStyle &style = ImGui::GetStyle();

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {

            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        ImGui::StyleColorsDark();

        ImGui_ImplSDL3_InitForOpenGL(window, context);
        ImGui_ImplOpenGL3_Init("#version 330");

        create_programs();

        init();
    }
}
