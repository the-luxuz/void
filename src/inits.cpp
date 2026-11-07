#include "headers/inits.h"
#include "headers/scene_hierarchy.h"

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

    shader_uniform.renderBothSides = glGetUniformLocation(program, "renderBothSides");

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
    shader_uniform.packedMetallicRoughnessMap = glGetUniformLocation(program, "packedMetallicRoughnessMap");
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

        Create_depth_tex(ShadowMapResolution, ShadowMapResolution, i);

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

bool init_lights(Object &obj) {

    init_model("../data/objects/sphere.obj", obj);

    return true;
}

namespace {

    void RestoreMaterialTextures(Material& material) {

        if (!material.albedoPath.empty()) material.albedoTexture = TextureManager::Get(material.albedoPath, true);
        if (!material.normalPath.empty()) material.normalTexture = TextureManager::Get(material.normalPath, false);
        if (!material.roughnessPath.empty()) material.roughnessTexture = TextureManager::Get(material.roughnessPath, false);
        if (!material.metallicPath.empty()) material.metallicTexture = TextureManager::Get(material.metallicPath, false);
        if (!material.emissionPath.empty()) material.emissionTexture = TextureManager::Get(material.emissionPath, false);
        if (!material.opacityPath.empty()) material.opacityTexture = TextureManager::Get(material.opacityPath, false);
    }

    void RestoreSavedMaterials(Object& object, const std::vector<Material>& materials) {

        if (materials.empty())
            return;

        if (materials.size() != object.submeshes.size()) {

            std::cout << "Saved material count does not match model submeshes for " << object.mesh << '\n';

            return;
        }

        for (size_t i = 0; i < materials.size(); ++i) {

            Material material = materials[i];
            const Material &importedMaterial = object.submeshes[i].material;
            if (material.albedoPath.empty()) material.albedoTexture = importedMaterial.albedoTexture;
            if (material.normalPath.empty()) material.normalTexture = importedMaterial.normalTexture;
            if (material.roughnessPath.empty()) material.roughnessTexture = importedMaterial.roughnessTexture;
            if (material.metallicPath.empty()) material.metallicTexture = importedMaterial.metallicTexture;
            if (material.emissionPath.empty()) material.emissionTexture = importedMaterial.emissionTexture;
            if (material.opacityPath.empty()) material.opacityTexture = importedMaterial.opacityTexture;

            object.submeshes[i].material = std::move(material);

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

    std::unordered_map<std::string, std::vector<Object>> importedModelMeshes;

    for (auto &p : par.objects) {

        Object obj;

        const bool isEmpty = p.mesh.empty();
        if (!isEmpty) {
            auto imported = importedModelMeshes.find(p.mesh);
            if (imported == importedModelMeshes.end()) {
                std::vector<Object> meshes;
                if (ImportModel(p.mesh, meshes, nullptr, false))
                    imported = importedModelMeshes.emplace(p.mesh, std::move(meshes)).first;
                else
                    std::cerr << "Could not restore model: " << p.mesh << '\n';
            }
            if (imported != importedModelMeshes.end()) {
                const auto node = std::find_if(imported->second.begin(), imported->second.end(), [&](const Object &candidate) {
                    return candidate.importedMeshIndex == p.importedMeshIndex;
                });
                if (node != imported->second.end())
                    obj = *node;
                else
                    std::cerr << "Could not find imported model node " << p.importedMeshIndex << " in " << p.mesh << '\n';
            } else {
                std::cerr << "Could not find imported model node " << p.importedMeshIndex << " in " << p.mesh << '\n';
            }
            if (!obj.vertices.empty() && !obj.indices.empty())
                buffer(obj);
        }

        obj.id = p.id;
        obj.parentID = p.parentID;
        maxID = std::max(maxID, obj.id);
        obj.name = p.obj_name;
        obj.mesh = p.mesh;
        obj.importedMeshIndex = p.importedMeshIndex;
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
        obj.renderBothSides = p.renderBothSides;
        obj.model = GetModelMatrix(obj);
        if (!obj.vertices.empty())
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
            obj.renderBothSides = savedObject.renderBothSides;

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

    if (par.load(settings, scene, st, cam, objects)) {
        if (st.windowed_w <= 0 || st.windowed_h <= 0) {
            st.windowed_w = st.win_mode == window_mode::WINDOWED ? st.w : 1280;
            st.windowed_h = st.win_mode == window_mode::WINDOWED ? st.h : 720;
        }

        windowedSize = {st.windowed_w, st.windowed_h};
        ApplyWindowMode(window);
        init();
    }

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

        if (st.windowed_w <= 0 || st.windowed_h <= 0) {
            st.windowed_w = st.win_mode == window_mode::WINDOWED ? st.w : 1280;
            st.windowed_h = st.win_mode == window_mode::WINDOWED ? st.h : 720;
        }
        windowedSize = {st.windowed_w, st.windowed_h};

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

        if (st.windowed_w <= 0 || st.windowed_h <= 0) {
            st.windowed_w = st.w;
            st.windowed_h = st.h;
        }
        windowedSize = {st.windowed_w, st.windowed_h};

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
