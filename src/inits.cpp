#include "headers/inits.h"
#include "headers/scene_hierarchy.h"
#include "cgltf.h"

#include <filesystem>
#include <functional>
#include <limits>
#include <unordered_map>
#include <glm/gtx/matrix_decompose.hpp>

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

    for (auto &shape : shapes) {

        auto &indices = shape.mesh.indices;

        int activeMaterialId = std::numeric_limits<int>::min();

        Submesh *activeSubmesh = nullptr;

        for (size_t face = 0; face * 3 + 2 < indices.size(); ++face) {

            int materialId = face < shape.mesh.material_ids.size() ? shape.mesh.material_ids[face] : -1;

            if (materialId != activeMaterialId) {

                Submesh submesh{};

                submesh.firstIndex = static_cast<GLuint>(obj.indices.size());

                if (materialId >= 0 && materialId < static_cast<int>(materials.size())) {

                    size_t index = static_cast<size_t>(materialId);

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

            for (size_t corner = 0; corner < 3; corner++) {

                GLuint vertexIndex = addVertex(indices[face * 3 + corner]);

                obj.indices.push_back(vertexIndex);
                activeSubmesh->indexCount++;

                glm::vec3 &position = obj.vertices[vertexIndex].position;

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

bool ImportGltf(const std::string &path, std::vector<Object> &importedObjects) {

    cgltf_options options{};

    cgltf_data *data = nullptr;

    if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success || !data)
        return false;

    auto cleanup = [&]() { 
        cgltf_free(data); 
    };

    if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success) {

        cleanup();

        return false;
    }

    std::filesystem::path sourcePath(path);

    auto readAttribute = [](cgltf_accessor *accessor, size_t index, float *out, size_t size) {

        if (!accessor || accessor->type == cgltf_type_invalid || !cgltf_accessor_read_float(accessor, index, out, size))
            return false;

        return true;
    };

    auto materialFrom = [&](cgltf_material *source) {

        auto loadTexture = [&](cgltf_texture *texture, bool sRGB, std::string &texturePath) -> GLuint {

            if (!texture || !texture->image)
                return 0;

            cgltf_image *image = texture->image;
            if (image->uri) {
                auto imagePath = (sourcePath.parent_path() / image->uri).lexically_normal();
                if (!std::filesystem::exists(imagePath))
                    return 0;
                texturePath = imagePath.generic_string();
                return TextureManager::Get(texturePath, sRGB);
            }

            cgltf_buffer_view *view = image->buffer_view;
            if (!view || !view->buffer || !view->buffer->data || view->offset > view->buffer->size ||
                view->size > view->buffer->size - view->offset)
                return 0;

            const auto *bytes = static_cast<const unsigned char *>(view->data ? view->data : view->buffer->data);
            if (!view->data)
                bytes += view->offset;

            const size_t imageIndex = static_cast<size_t>(image - data->images);
            const std::string key = sourcePath.lexically_normal().generic_string() + "#image-" + std::to_string(imageIndex);
            return TextureManager::GetEmbedded(key, bytes, view->size, sRGB);
        };

        Material material{};

        if (!source) 
            return material;

        material.name = source->name ? source->name : "Material";

        if (source->has_pbr_metallic_roughness) {

            auto &pbr = source->pbr_metallic_roughness;

            material.diffuse = {pbr.base_color_factor[0], pbr.base_color_factor[1], pbr.base_color_factor[2]};

            material.opacity = pbr.base_color_factor[3];

            material.roughness = pbr.roughness_factor;

            material.metallic = pbr.metallic_factor;

            material.albedoTexture = loadTexture(pbr.base_color_texture.texture, true, material.albedoPath);

            const bool hasBaseColorTexture = pbr.base_color_texture.texture != nullptr;
            const bool hasDefaultWhiteFactor = pbr.base_color_factor[0] == 1.0f && pbr.base_color_factor[1] == 1.0f &&
                                               pbr.base_color_factor[2] == 1.0f;
            const std::string materialName = source->name ? source->name : "";
            if (!hasBaseColorTexture && hasDefaultWhiteFactor) {
                if (materialName.find("bread") != std::string::npos)
                    material.diffuse = {0.78f, 0.49f, 0.22f};
                else if (materialName.find("meat") != std::string::npos)
                    material.diffuse = {0.30f, 0.12f, 0.055f};
            }
        }
        material.normalTexture = loadTexture(source->normal_texture.texture, false, material.normalPath);
        return material;
    };

    std::vector<cgltf_node *> nodes;

    if (data->scene) {

        for (cgltf_size i = 0; i < data->scene->nodes_count; ++i) 
            nodes.push_back(data->scene->nodes[i]);

    } else if (data->scenes_count) {

        for (cgltf_size i = 0; i < data->scenes[0].nodes_count; ++i) 
            nodes.push_back(data->scenes[0].nodes[i]);

    } else {
        for (cgltf_size i = 0; i < data->nodes_count; ++i)
            if (!data->nodes[i].parent) nodes.push_back(&data->nodes[i]);
    }

    size_t unnamedIndex = 0;

    std::function<void(cgltf_node *)> visit = [&](cgltf_node *node) {

        if (node->mesh) {

            Object obj;

            obj.name = node->name ? node->name : (node->mesh->name ? node->mesh->name : "ImportedObject" + std::to_string(unnamedIndex++));

            obj.mesh = path;

            float worldValues[16];

            cgltf_node_transform_world(node, worldValues);

            glm::mat4 world = glm::make_mat4(worldValues);
            glm::vec3 skew, scale;
            glm::vec4 perspective;

            glm::decompose(world, scale, obj.rotation, obj.position, skew, perspective);

            obj.scale = scale;

            glm::vec3 boundsMin(FLT_MAX), boundsMax(-FLT_MAX);

            for (cgltf_size p = 0; p < node->mesh->primitives_count; ++p) {

                cgltf_primitive &primitive = node->mesh->primitives[p];

                if (primitive.type != cgltf_primitive_type_triangles)
                    continue;

                cgltf_accessor *positions = nullptr, *normals = nullptr, *uvs = nullptr;

                for (cgltf_size a = 0; a < primitive.attributes_count; ++a) {

                    cgltf_attribute &attribute = primitive.attributes[a];

                    if (attribute.type == cgltf_attribute_type_position) 
                        positions = attribute.data;

                    else if (attribute.type == cgltf_attribute_type_normal) 
                        normals = attribute.data;

                    else if (attribute.type == cgltf_attribute_type_texcoord && attribute.index == 0) 
                        uvs = attribute.data;

                }
                if (!positions) 
                    continue;

                Submesh submesh{};

                submesh.firstIndex = static_cast<GLuint>(obj.indices.size());

                submesh.min = glm::vec3(FLT_MAX); submesh.max = glm::vec3(-FLT_MAX);

                submesh.material = materialFrom(primitive.material);

                GLuint baseVertex = static_cast<GLuint>(obj.vertices.size());

                for (cgltf_size v = 0; v < positions->count; ++v) {

                    Vertex vertex{};
                    float value[4]{};

                    if (!readAttribute(positions, v, value, 3)) 
                        continue;

                    vertex.position = {value[0], value[1], value[2]};

                    if (readAttribute(normals, v, value, 3)) 
                        vertex.normal = {value[0], value[1], value[2]};

                    if (readAttribute(uvs, v, value, 2)) 
                        vertex.uv = {value[0], 1.0f - value[1]};

                    vertex.originalUV = vertex.uv;

                    obj.vertices.push_back(vertex);

                    submesh.min = glm::min(submesh.min, vertex.position); submesh.max = glm::max(submesh.max, vertex.position);
                }
                if (primitive.indices) {

                    for (cgltf_size i = 0; i < primitive.indices->count; ++i)
                        obj.indices.push_back(baseVertex + static_cast<GLuint>(cgltf_accessor_read_index(primitive.indices, i)));

                } else {

                    for (cgltf_size i = 0; i < positions->count; ++i) 
                        obj.indices.push_back(baseVertex + static_cast<GLuint>(i));
                }

                submesh.indexCount = static_cast<GLuint>(obj.indices.size()) - submesh.firstIndex;
                if (submesh.indexCount) 
                    obj.submeshes.push_back(std::move(submesh));
            }
            if (!obj.vertices.empty() && !obj.indices.empty()) {

                BuildImportedTBN(obj);

                for (const Vertex &vertex : obj.vertices) { 

                    boundsMin = glm::min(boundsMin, vertex.position); 
                    boundsMax = glm::max(boundsMax, vertex.position); 
                }

                obj.min = boundsMin; 
                obj.max = boundsMax;

                obj.visible = std::none_of(obj.submeshes.begin(), obj.submeshes.end(), [](const Submesh &submesh) {
                    return submesh.material.name == "floor";
                });

                obj.collider.radius = glm::length(boundsMax - boundsMin) * 0.5f;

                obj.original_vertices = obj.vertices; obj.original_indices = obj.indices;

                importedObjects.push_back(std::move(obj));
            }
        }

        for (cgltf_size i = 0; i < node->children_count; ++i) 
            visit(node->children[i]);
    };

    for (cgltf_node *node : nodes) 
        visit(node);

    if (!importedObjects.empty()) {

        glm::vec3 sceneMin(FLT_MAX), sceneMax(-FLT_MAX);

        for (Object &obj : importedObjects) {

            if (!obj.visible)
                continue;

            glm::mat4 model = glm::translate(glm::mat4(1.0f), obj.position) * glm::toMat4(obj.rotation) * glm::scale(glm::mat4(1.0f), obj.scale);

            glm::vec3 objectMin(FLT_MAX), objectMax(-FLT_MAX);

            for (int corner = 0; corner < 8; ++corner) {

                glm::vec3 point( (corner & 1) ? obj.max.x : obj.min.x, (corner & 2) ? obj.max.y : obj.min.y, (corner & 4) ? obj.max.z : obj.min.z);

                glm::vec3 worldPoint = glm::vec3(model * glm::vec4(point, 1.0f));

                sceneMin = glm::min(sceneMin, worldPoint);
                sceneMax = glm::max(sceneMax, worldPoint);

                objectMin = glm::min(objectMin, worldPoint);
                objectMax = glm::max(objectMax, worldPoint);
            }

        }
         
        glm::vec3 center = (sceneMin + sceneMax) * 0.5f;
        glm::vec3 sceneSize = sceneMax - sceneMin;
        float largestSceneDimension = std::max(sceneSize.x, std::max(sceneSize.y, sceneSize.z));
        float importScale = largestSceneDimension > 0.0001f ? 100.0f / largestSceneDimension : 1.0f;

        for (Object &obj : importedObjects) {

            if (!obj.visible)
                continue;

            obj.position = (obj.position - center) * importScale;
            obj.scale *= importScale;
            obj.collider.radius *= importScale;
        }
    }

    cleanup();

    return !importedObjects.empty();
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

            std::cout << "Saved material count does not match OBJ submeshes for " << object.mesh << '\n';

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

    std::unordered_map<std::string, std::vector<Object>> importedGltfMeshes;

    for (auto &p : par.objects) {

        Object obj;

        const bool isEmpty = p.mesh.empty();
        if (!isEmpty) {
            const std::string extension = std::filesystem::path(p.mesh).extension().string();
            if (extension == ".gltf" || extension == ".glb") {
                auto imported = importedGltfMeshes.find(p.mesh);
                if (imported == importedGltfMeshes.end()) {
                    std::vector<Object> meshes;
                    if (ImportGltf(p.mesh, meshes))
                        imported = importedGltfMeshes.emplace(p.mesh, std::move(meshes)).first;
                    else
                        std::cerr << "Could not restore glTF model: " << p.mesh << '\n';
                }
                if (imported != importedGltfMeshes.end() && p.importedMeshIndex < imported->second.size())
                    obj = imported->second[p.importedMeshIndex];
                else
                    std::cerr << "Could not find imported glTF mesh " << p.importedMeshIndex << " in " << p.mesh << '\n';
            } else {
                init_obj(p.mesh.c_str(), obj);
            }
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
