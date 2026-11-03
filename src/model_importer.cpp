#include "headers/inits.h"
#include "headers/helpers.h"

#include <assimp/cimport.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/texture.h>
#include <tinyexr_zstd.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <unordered_map>

#include <glm/gtx/matrix_decompose.hpp>

namespace {

glm::mat4 ToGlm(const aiMatrix4x4 &matrix) {
    glm::mat4 result(1.0f);
    result[0][0] = matrix.a1; result[1][0] = matrix.a2; result[2][0] = matrix.a3; result[3][0] = matrix.a4;
    result[0][1] = matrix.b1; result[1][1] = matrix.b2; result[2][1] = matrix.b3; result[3][1] = matrix.b4;
    result[0][2] = matrix.c1; result[1][2] = matrix.c2; result[2][2] = matrix.c3; result[3][2] = matrix.c4;
    result[0][3] = matrix.d1; result[1][3] = matrix.d2; result[2][3] = matrix.d3; result[3][3] = matrix.d4;
    return result;
}

glm::mat4 ComposeTransform(const glm::vec3 &position, const glm::quat &rotation, const glm::vec3 &scale) {
    return glm::translate(glm::mat4(1.0f), position) * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), scale);
}

bool DecomposeTransform(const glm::mat4 &matrix, Object &object) {
    glm::vec3 skew;
    glm::vec4 perspective;
    if (!glm::decompose(matrix, object.scale, object.rotation, object.position, skew, perspective))
        return false;
    object.rotation = glm::normalize(object.rotation);
    return true;
}

glm::vec3 ToGlm(const aiVector3D &value) {
    return {value.x, value.y, value.z};
}

glm::quat ToGlm(const aiQuaternion &value) {
    return glm::normalize(glm::quat(value.w, value.x, value.y, value.z));
}

bool ReadFloat(const aiMaterial *material, const char *key, unsigned int type, unsigned int index, float &value) {
    ai_real result = value;
    unsigned int count = 1;
    if (aiGetMaterialFloatArray(material, key, type, index, &result, &count) != AI_SUCCESS || count == 0)
        return false;
    value = static_cast<float>(result);
    return true;
}

bool ReadColor(const aiMaterial *material, const char *key, unsigned int type, unsigned int index, glm::vec3 &value) {
    aiColor4D color;
    if (aiGetMaterialColor(material, key, type, index, &color) != AI_SUCCESS)
        return false;
    value = {color.r, color.g, color.b};
    return true;
}

std::string NormalizedExtension(const std::string &extension);

const aiScene *ImportAssimpFile(const std::string &path, unsigned int flags,
                                std::vector<char> &decompressedBlend) {
    constexpr unsigned char ZstdMagic[] = {0x28, 0xB5, 0x2F, 0xFD};
    constexpr unsigned long long MaxDecompressedBlendSize = 1024ull * 1024ull * 1024ull;

    std::ifstream input(path, std::ios::binary);
    if (!input)
        return aiImportFile(path.c_str(), flags);

    unsigned char magic[sizeof(ZstdMagic)]{};
    input.read(reinterpret_cast<char *>(magic), sizeof(magic));
    if (input.gcount() != sizeof(magic) || std::memcmp(magic, ZstdMagic, sizeof(magic)) != 0)
        return aiImportFile(path.c_str(), flags);

    input.seekg(0, std::ios::end);
    const std::streamoff fileSize = input.tellg();
    if (fileSize <= 0 || static_cast<unsigned long long>(fileSize) > std::numeric_limits<unsigned int>::max()) {
        std::cerr << "Compressed Blender file is too large to import: " << path << '\n';
        return nullptr;
    }
    input.seekg(0, std::ios::beg);
    std::vector<char> compressed(static_cast<size_t>(fileSize));
    if (!input.read(compressed.data(), fileSize)) {
        std::cerr << "Could not read compressed Blender file: " << path << '\n';
        return nullptr;
    }

    const unsigned long long decompressedSize = tinyexr_zstd_decompress_bound(compressed.data(), compressed.size());
    if (decompressedSize == 0 || decompressedSize > MaxDecompressedBlendSize ||
        decompressedSize > std::numeric_limits<unsigned int>::max()) {
        std::cerr << "Could not determine a safe decompressed size for Blender file: " << path << '\n';
        return nullptr;
    }

    decompressedBlend.resize(static_cast<size_t>(decompressedSize));
    const size_t actualSize = tinyexr_zstd_decompress(decompressedBlend.data(), decompressedBlend.size(),
                                                       compressed.data(), compressed.size());
    if (tinyexr_zstd_is_error(actualSize) || actualSize > decompressedBlend.size() || actualSize < 7 ||
        std::memcmp(decompressedBlend.data(), "BLENDER", 7) != 0) {
        decompressedBlend.clear();
        std::cerr << "Could not decompress Blender Zstandard file: " << path << '\n';
        return nullptr;
    }
    decompressedBlend.resize(actualSize);

    std::string extension = NormalizedExtension(std::filesystem::path(path).extension().string());
    if (!extension.empty() && extension.front() == '.')
        extension.erase(extension.begin());
    const aiScene *scene = aiImportFileFromMemory(decompressedBlend.data(), static_cast<unsigned int>(actualSize),
                                                   flags, extension.c_str());
    if (!scene)
        std::cerr << "Assimp could not import decompressed Blender file '" << path << "': " << aiGetErrorString() << '\n';
    return scene;
}

std::string NormalizedExtension(const std::string &extension) {
    std::string result = extension;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (!result.empty() && result.front() != '.')
        result.insert(result.begin(), '.');
    return result;
}

void UpdateObjectBounds(Object &object) {
    if (object.vertices.empty()) {
        object.min = glm::vec3(0.0f);
        object.max = glm::vec3(0.0f);
        object.collider.radius = 0.0f;
        return;
    }

    object.min = glm::vec3(std::numeric_limits<float>::max());
    object.max = glm::vec3(std::numeric_limits<float>::lowest());
    for (const Vertex &vertex : object.vertices) {
        object.min = glm::min(object.min, vertex.position);
        object.max = glm::max(object.max, vertex.position);
    }

    const glm::vec3 size = object.max - object.min;
    object.collider.radius = glm::length(size) * 0.5f;
    for (Submesh &submesh : object.submeshes) {
        submesh.min = glm::vec3(std::numeric_limits<float>::max());
        submesh.max = glm::vec3(std::numeric_limits<float>::lowest());
        const size_t end = std::min(object.indices.size(), static_cast<size_t>(submesh.firstIndex) + submesh.indexCount);
        for (size_t i = submesh.firstIndex; i < end; ++i) {
            const GLuint vertexIndex = object.indices[i];
            if (vertexIndex < object.vertices.size()) {
                submesh.min = glm::min(submesh.min, object.vertices[vertexIndex].position);
                submesh.max = glm::max(submesh.max, object.vertices[vertexIndex].position);
            }
        }
    }
}

struct ImportContext {
    const aiScene *scene = nullptr;
    std::filesystem::path sourcePath;
    std::string sourcePathKey;
    std::vector<Object> &objects;
    std::unordered_map<std::string, std::vector<size_t>> objectsByNodeName;
    glm::vec3 boundsMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 boundsMax = glm::vec3(std::numeric_limits<float>::lowest());
    bool hasBounds = false;

    ImportContext(const aiScene *importedScene, const std::string &path, std::vector<Object> &output)
        : scene(importedScene), sourcePath(path), sourcePathKey(std::filesystem::path(path).lexically_normal().generic_string()), objects(output) {}

    std::filesystem::path ResolveTexturePath(const aiString &reference) const {
        std::string text = reference.C_Str();
        std::replace(text.begin(), text.end(), '\\', '/');
        if (text.empty() || text.front() == '*')
            return {};
        std::filesystem::path texturePath(text);
        if (texturePath.is_absolute())
            return texturePath.lexically_normal();
        return (sourcePath.parent_path() / texturePath).lexically_normal();
    }

    GLuint LoadMaterialTexture(const aiMaterial *material, aiTextureType preferredType, aiTextureType fallbackType,
                               bool sRGB, std::string &storedPath) const {
        if (!material)
            return 0;
        aiString reference;
        aiReturn status = AI_FAILURE;
        if (aiGetMaterialTextureCount(material, preferredType) > 0)
            status = aiGetMaterialTexture(material, preferredType, 0, &reference);
        if (status != AI_SUCCESS && aiGetMaterialTextureCount(material, fallbackType) > 0)
            status = aiGetMaterialTexture(material, fallbackType, 0, &reference);
        if (status != AI_SUCCESS)
            return 0;

        const std::string referenceText = reference.C_Str();
        if (const aiTexture *embedded = aiGetEmbeddedTexture(scene, referenceText.c_str())) {
            const std::string embeddedKey = sourcePathKey + "#embedded-" + referenceText + (sRGB ? "|srgb" : "|linear");
            if (embedded->mHeight == 0)
                return TextureManager::GetEmbedded(embeddedKey, reinterpret_cast<const unsigned char *>(embedded->pcData), embedded->mWidth, sRGB);
            return TextureManager::GetEmbeddedRawBGRA(embeddedKey, static_cast<int>(embedded->mWidth), static_cast<int>(embedded->mHeight),
                                                       reinterpret_cast<const unsigned char *>(embedded->pcData), sRGB);
        }

        const std::filesystem::path resolved = ResolveTexturePath(reference);
        if (resolved.empty())
            return 0;
        storedPath = resolved.generic_string();
        if (!std::filesystem::exists(resolved)) {
            std::cerr << "Assimp material texture not found: " << storedPath << '\n';
            return 0;
        }
        return TextureManager::Get(storedPath, sRGB);
    }

    Material ConvertMaterial(unsigned int materialIndex) const {
        Material result{};
        if (materialIndex >= scene->mNumMaterials)
            return result;
        const aiMaterial *source = scene->mMaterials[materialIndex];
        aiString name;
        if (aiGetMaterialString(source, AI_MATKEY_NAME, &name) == AI_SUCCESS)
            result.name = name.C_Str();

        if (!ReadColor(source, AI_MATKEY_BASE_COLOR, result.diffuse))
            ReadColor(source, AI_MATKEY_COLOR_DIFFUSE, result.diffuse);
        ReadColor(source, AI_MATKEY_COLOR_SPECULAR, result.specular);
        ReadColor(source, AI_MATKEY_COLOR_EMISSIVE, result.emission);

        ReadFloat(source, AI_MATKEY_OPACITY, result.opacity);
        ReadFloat(source, AI_MATKEY_SHININESS, result.shininess);
        if (!ReadFloat(source, AI_MATKEY_ROUGHNESS_FACTOR, result.roughness))
            result.roughness = glm::clamp(std::sqrt(2.0f / (result.shininess + 2.0f)), 0.04f, 1.0f);
        ReadFloat(source, AI_MATKEY_METALLIC_FACTOR, result.metallic);
        result.roughness = glm::clamp(result.roughness, 0.04f, 1.0f);
        result.metallic = glm::clamp(result.metallic, 0.0f, 1.0f);

        result.albedoTexture = LoadMaterialTexture(source, aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE, true, result.albedoPath);
        result.normalTexture = LoadMaterialTexture(source, aiTextureType_NORMALS, aiTextureType_HEIGHT, false, result.normalPath);
        // glTF packs roughness into G and metalness into B of one texture.
        // Assimp also exposes aliases as separate roughness/metalness slots,
        // but those must not be sampled as grayscale (R) maps.
        std::string packedMetallicRoughnessPath;
        const GLuint packedMetallicRoughness = LoadMaterialTexture(
            source, aiTextureType_GLTF_METALLIC_ROUGHNESS, aiTextureType_UNKNOWN,
            false, packedMetallicRoughnessPath);
        if (packedMetallicRoughness != 0) {
            result.roughnessTexture = packedMetallicRoughness;
            result.metallicTexture = packedMetallicRoughness;
            result.roughnessPath = packedMetallicRoughnessPath;
            result.metallicPath = packedMetallicRoughnessPath;
        } else {
            result.roughnessTexture = LoadMaterialTexture(source, aiTextureType_DIFFUSE_ROUGHNESS, aiTextureType_UNKNOWN, false, result.roughnessPath);
            result.metallicTexture = LoadMaterialTexture(source, aiTextureType_METALNESS, aiTextureType_UNKNOWN, false, result.metallicPath);
        }
        result.emissionTexture = LoadMaterialTexture(source, aiTextureType_EMISSION_COLOR, aiTextureType_EMISSIVE, true, result.emissionPath);
        result.opacityTexture = LoadMaterialTexture(source, aiTextureType_OPACITY, aiTextureType_UNKNOWN, false, result.opacityPath);
        return result;
    }

    void AppendMesh(Object &object, const aiMesh *mesh) {
        if (!mesh || !mesh->HasPositions() || mesh->mNumVertices == 0)
            return;

        const GLuint baseVertex = static_cast<GLuint>(object.vertices.size());
        for (unsigned int vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
            Vertex vertex{};
            vertex.position = ToGlm(mesh->mVertices[vertexIndex]);
            if (mesh->HasNormals())
                vertex.normal = glm::normalize(ToGlm(mesh->mNormals[vertexIndex]));
            if (mesh->HasTextureCoords(0)) {
                vertex.uv = {mesh->mTextureCoords[0][vertexIndex].x, mesh->mTextureCoords[0][vertexIndex].y};
                vertex.originalUV = vertex.uv;
            }
            object.vertices.push_back(vertex);
        }

        Submesh submesh{};
        submesh.firstIndex = static_cast<GLuint>(object.indices.size());
        submesh.material = ConvertMaterial(mesh->mMaterialIndex);
        for (unsigned int faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
            const aiFace &face = mesh->mFaces[faceIndex];
            if (face.mNumIndices != 3)
                continue;
            for (unsigned int corner = 0; corner < 3; ++corner)
                object.indices.push_back(baseVertex + face.mIndices[corner]);
        }
        submesh.indexCount = static_cast<GLuint>(object.indices.size()) - submesh.firstIndex;
        if (submesh.indexCount > 0)
            object.submeshes.push_back(std::move(submesh));
    }

    void VisitNode(const aiNode *node, const glm::mat4 &parentWorld, EntityID parentObjectID) {
        if (!node)
            return;
        const glm::mat4 localMatrix = ToGlm(node->mTransformation);
        const glm::mat4 worldMatrix = parentWorld * localMatrix;
        Object object;
        object.id = GenerateEntityID();
        object.name = node->mName.length > 0 ? node->mName.C_Str() : "ImportedNode";
        object.mesh = sourcePath.string();
        object.parentID = parentObjectID;
        object.importedMeshIndex = static_cast<uint32_t>(objects.size());
        DecomposeTransform(localMatrix, object);
        for (unsigned int meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex) {
            const unsigned int sceneMeshIndex = node->mMeshes[meshIndex];
            if (sceneMeshIndex < scene->mNumMeshes)
                AppendMesh(object, scene->mMeshes[sceneMeshIndex]);
        }
        if (!object.vertices.empty() && !object.indices.empty()) {
            BuildImportedTBN(object);
            UpdateObjectBounds(object);
            object.original_vertices = object.vertices;
            object.original_indices = object.indices;
            for (const Vertex &vertex : object.vertices) {
                const glm::vec3 point = glm::vec3(worldMatrix * glm::vec4(vertex.position, 1.0f));
                boundsMin = glm::min(boundsMin, point);
                boundsMax = glm::max(boundsMax, point);
                hasBounds = true;
            }
        }

        const size_t objectIndex = objects.size();
        objectsByNodeName[node->mName.C_Str()].push_back(objectIndex);
        objects.push_back(std::move(object));
        const EntityID thisObjectID = objects.back().id;
        for (unsigned int childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
            VisitNode(node->mChildren[childIndex], worldMatrix, thisObjectID);
    }
};

AnimationTrack MakeTrack(EntityID targetID, AnimationProperty property) {
    AnimationTrack track{};
    track.targetType = AnimationTargetType::Object;
    track.targetID = targetID;
    track.property = property;
    return track;
}

void ImportAnimations(const aiScene *scene, ImportContext &context, std::vector<AnimationClip> &clips,
                      const glm::mat4 &normalization, bool normalizeForEditor) {
    for (unsigned int animationIndex = 0; animationIndex < scene->mNumAnimations; ++animationIndex) {
        const aiAnimation *sourceAnimation = scene->mAnimations[animationIndex];
        if (!sourceAnimation)
            continue;
        const double ticksPerSecond = sourceAnimation->mTicksPerSecond > 0.0 ? sourceAnimation->mTicksPerSecond : 25.0;
        AnimationClip clip;
        clip.name = sourceAnimation->mName.length > 0 ? sourceAnimation->mName.C_Str() : "Animation" + std::to_string(animationIndex + 1);
        clip.duration = static_cast<float>(sourceAnimation->mDuration / ticksPerSecond);
        clip.loop = true;

        float latestKeyTime = 0.0f;
        for (unsigned int channelIndex = 0; channelIndex < sourceAnimation->mNumChannels; ++channelIndex) {
            const aiNodeAnim *channel = sourceAnimation->mChannels[channelIndex];
            if (!channel)
                continue;
            const std::string targetName = channel->mNodeName.C_Str();
            const auto namedObject = context.objectsByNodeName.find(targetName);
            if (namedObject == context.objectsByNodeName.end() || namedObject->second.empty()) {
                std::cerr << "Assimp animation channel has no imported node: " << targetName << '\n';
                continue;
            }
            Object &target = context.objects[namedObject->second.front()];
            const bool normalizeTarget = normalizeForEditor && target.parentID == 0;

            AnimationTrack positionTrack = MakeTrack(target.id, AnimationProperty::Position);
            for (unsigned int keyIndex = 0; keyIndex < channel->mNumPositionKeys; ++keyIndex) {
                const aiVectorKey &key = channel->mPositionKeys[keyIndex];
                const float time = static_cast<float>(key.mTime / ticksPerSecond);
                glm::vec3 value = ToGlm(key.mValue);
                if (normalizeTarget) {
                    glm::mat4 transformed = normalization * ComposeTransform(value, target.rotation, target.scale);
                    Object decomposed;
                    if (DecomposeTransform(transformed, decomposed))
                        value = decomposed.position;
                }
                AddKey(positionTrack, time, value);
                latestKeyTime = std::max(latestKeyTime, time);
            }
            if (!positionTrack.keys.empty())
                clip.tracks.push_back(std::move(positionTrack));

            AnimationTrack rotationTrack = MakeTrack(target.id, AnimationProperty::Rotation);
            for (unsigned int keyIndex = 0; keyIndex < channel->mNumRotationKeys; ++keyIndex) {
                const aiQuatKey &key = channel->mRotationKeys[keyIndex];
                const float time = static_cast<float>(key.mTime / ticksPerSecond);
                glm::quat value = ToGlm(key.mValue);
                if (normalizeTarget) {
                    glm::mat4 transformed = normalization * ComposeTransform(target.position, value, target.scale);
                    Object decomposed;
                    if (DecomposeTransform(transformed, decomposed))
                        value = decomposed.rotation;
                }
                AddKey(rotationTrack, time, value);
                latestKeyTime = std::max(latestKeyTime, time);
            }
            if (!rotationTrack.keys.empty())
                clip.tracks.push_back(std::move(rotationTrack));

            AnimationTrack scaleTrack = MakeTrack(target.id, AnimationProperty::Scale);
            for (unsigned int keyIndex = 0; keyIndex < channel->mNumScalingKeys; ++keyIndex) {
                const aiVectorKey &key = channel->mScalingKeys[keyIndex];
                const float time = static_cast<float>(key.mTime / ticksPerSecond);
                glm::vec3 value = ToGlm(key.mValue);
                if (normalizeTarget) {
                    glm::mat4 transformed = normalization * ComposeTransform(target.position, target.rotation, value);
                    Object decomposed;
                    if (DecomposeTransform(transformed, decomposed))
                        value = decomposed.scale;
                }
                AddKey(scaleTrack, time, value);
                latestKeyTime = std::max(latestKeyTime, time);
            }
            if (!scaleTrack.keys.empty())
                clip.tracks.push_back(std::move(scaleTrack));
        }
        clip.duration = std::max(clip.duration, latestKeyTime);
        if (!clip.tracks.empty())
            clips.push_back(std::move(clip));
    }
}

} // namespace

bool IsModelFormatSupported(const std::string &extension) {
    const std::string normalized = NormalizedExtension(extension);
    return normalized.size() > 1 && aiIsExtensionSupported(normalized.c_str()) == AI_TRUE;
}

bool ImportModel(const std::string &path, std::vector<Object> &importedObjects,
                 std::vector<AnimationClip> *importedAnimations, bool normalizeForEditor) {
    importedObjects.clear();
    if (importedAnimations)
        importedAnimations->clear();

    // The glTF Assimp importer already flips UVs to its internal convention.
    // Applying aiProcess_FlipUVs here would flip them a second time and sample
    // a different region of embedded texture atlases. Texture uploads already
    // account for image row orientation in TextureManager.
    const unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                               aiProcess_SortByPType | aiProcess_ValidateDataStructure | aiProcess_JoinIdenticalVertices;
    std::vector<char> decompressedBlend;
    const aiScene *scene = ImportAssimpFile(path, flags, decompressedBlend);
    if (!scene || !scene->mRootNode) {
        std::cerr << "Assimp could not import '" << path << "': " << aiGetErrorString() << '\n';
        if (scene)
            aiReleaseImport(scene);
        return false;
    }

    ImportContext context(scene, path, importedObjects);
    context.VisitNode(scene->mRootNode, glm::mat4(1.0f), 0);

    // Keep mesh-bearing node indices dense and first. Older saved scenes assigned
    // importedMeshIndex only to geometry objects, with the first mesh at index 0.
    uint32_t nextImportedIndex = 0;
    for (Object &object : importedObjects)
        if (!object.vertices.empty() && !object.indices.empty())
            object.importedMeshIndex = nextImportedIndex++;
    for (Object &object : importedObjects)
        if (object.vertices.empty() || object.indices.empty())
            object.importedMeshIndex = nextImportedIndex++;

    glm::vec3 center(0.0f);
    float importScale = 1.0f;
    if (normalizeForEditor && context.hasBounds) {
        center = (context.boundsMin + context.boundsMax) * 0.5f;
        const glm::vec3 dimensions = context.boundsMax - context.boundsMin;
        const float largestDimension = std::max({dimensions.x, dimensions.y, dimensions.z});
        if (largestDimension > 0.0001f)
            importScale = 100.0f / largestDimension;
    }
    const glm::mat4 normalization = glm::scale(glm::mat4(1.0f), glm::vec3(importScale)) *
                                    glm::translate(glm::mat4(1.0f), -center);

    // Build animation keys while node transforms still hold their source values.
    if (importedAnimations)
        ImportAnimations(scene, context, *importedAnimations, normalization, normalizeForEditor);

    if (normalizeForEditor) {
        for (Object &object : importedObjects) {
            if (object.parentID != 0)
                continue;
            const glm::mat4 local = ComposeTransform(object.position, object.rotation, object.scale);
            DecomposeTransform(normalization * local, object);
            object.collider.radius *= importScale;
        }
    }

    aiReleaseImport(scene);
    return !importedObjects.empty();
}

void init_model(const char *modelFile, Object &object, bool normalizeImportedSize) {
    if (!modelFile)
        return;
    std::vector<Object> imported;
    if (!ImportModel(modelFile, imported, nullptr, normalizeImportedSize))
        return;

    object.vertices.clear();
    object.indices.clear();
    object.submeshes.clear();
    object.mesh = modelFile;
    object.name = std::filesystem::path(modelFile).stem().string();
    object.position = glm::vec3(0.0f);
    object.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    object.scale = glm::vec3(1.0f);

    std::unordered_map<EntityID, size_t> objectIndices;
    for (size_t i = 0; i < imported.size(); ++i)
        objectIndices[imported[i].id] = i;
    std::vector<glm::mat4> worldMatrices(imported.size(), glm::mat4(1.0f));
    for (size_t i = 0; i < imported.size(); ++i) {
        const Object &part = imported[i];
        glm::mat4 parentWorld(1.0f);
        const auto parent = objectIndices.find(part.parentID);
        if (parent != objectIndices.end() && parent->second < i)
            parentWorld = worldMatrices[parent->second];
        worldMatrices[i] = parentWorld * ComposeTransform(part.position, part.rotation, part.scale);
        if (part.vertices.empty() || part.indices.empty())
            continue;

        const GLuint baseVertex = static_cast<GLuint>(object.vertices.size());
        const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(worldMatrices[i])));
        for (Vertex vertex : part.vertices) {
            vertex.position = glm::vec3(worldMatrices[i] * glm::vec4(vertex.position, 1.0f));
            vertex.normal = glm::normalize(normalMatrix * vertex.normal);
            object.vertices.push_back(vertex);
        }
        for (GLuint index : part.indices)
            object.indices.push_back(baseVertex + index);
        for (Submesh submesh : part.submeshes) {
            submesh.firstIndex += static_cast<GLuint>(object.indices.size() - part.indices.size());
            object.submeshes.push_back(std::move(submesh));
        }
    }
    if (object.vertices.empty() || object.indices.empty())
        return;
    BuildImportedTBN(object);
    UpdateObjectBounds(object);
    object.original_vertices = object.vertices;
    object.original_indices = object.indices;
}
