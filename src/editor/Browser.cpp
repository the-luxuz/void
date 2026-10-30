#include "Browser.h"

#include <unordered_map>

namespace {

constexpr int ModelThumbnailSize = 128;

struct PreviewMesh {

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
};

static std::unordered_map<std::string, GLuint> modelThumbnailCache;
static GLuint modelThumbnailFbo = 0;
static GLuint modelThumbnailDepth = 0;

void ClearModelThumbnailCache() {

    for (auto& [path, texture] : modelThumbnailCache) {

        (void)path;

        if (texture)
            glDeleteTextures(1, &texture);
    }

    modelThumbnailCache.clear();
}

bool LoadPreviewModel(const std::string &path, std::vector<Object> &previewObjects) {

    std::string extension = std::filesystem::path(path).extension().string();

    if (extension == ".obj") {

        Object object;

        init_obj(path.c_str(), object);
        PrepareImportedObject(object);

        if (!object.vertices.empty() && !object.indices.empty())
            previewObjects.push_back(std::move(object));

        return !previewObjects.empty();
    }

    if (extension == ".gltf" || extension == ".glb")
        return ImportGltf(path, previewObjects);

    return false;
}

GLuint CreatePreviewMesh(Object &object, PreviewMesh &mesh) {

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, object.vertices.size() * sizeof(Vertex), object.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, object.indices.size() * sizeof(GLuint), object.indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, normal)));

    return mesh.vao;
}

GLuint RenderModelThumbnail(const std::string &path) {

    std::vector<Object> previewObjects;

    if (!LoadPreviewModel(path, previewObjects))
        return 0;

    glm::vec3 boundsMin(FLT_MAX);
    glm::vec3 boundsMax(-FLT_MAX);

    bool hasBounds = false;

    std::vector<glm::mat4> objectModels;
    objectModels.reserve(previewObjects.size());

    for (Object &object : previewObjects) {

        const glm::mat4 objectModel = glm::translate(glm::mat4(1.0f), object.position) * glm::toMat4(object.rotation) * glm::scale(glm::mat4(1.0f), object.scale);
        objectModels.push_back(objectModel);

        if (!object.visible)
            continue;

        for (const Vertex &vertex : object.vertices) {

            glm::vec3 point = glm::vec3(objectModel * glm::vec4(vertex.position, 1.0f));

            boundsMin = glm::min(boundsMin, point);
            boundsMax = glm::max(boundsMax, point);

            hasBounds = true;
        }
    }

    if (!hasBounds)
        return 0;

    glm::vec3 center = (boundsMin + boundsMax) * 0.5f;
    glm::vec3 size = boundsMax - boundsMin;

    float largestDimension = std::max({size.x, size.y, size.z});

    if (largestDimension <= 0.0001f)
        return 0;

    float fitScale = 1.55f / largestDimension;
    glm::mat4 previewRotation = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(1, 0, 0)) * glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0, 1, 0));

    glm::mat4 frameModel = glm::scale(glm::mat4(1.0f), glm::vec3(fitScale)) * previewRotation * glm::translate(glm::mat4(1.0f), -center);

    glm::mat4 previewView = glm::lookAt(glm::vec3(0, 0, 3.6f), glm::vec3(0), glm::vec3(0, 1, 0));

    glm::mat4 previewProjection = glm::perspective(glm::radians(60.0f), 1.0f, 0.05f, 20.0f);

    GLint oldRenderbuffer = 0;
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &oldRenderbuffer);

    if (!modelThumbnailFbo) {

        glGenFramebuffers(1, &modelThumbnailFbo);
        glGenRenderbuffers(1, &modelThumbnailDepth);
        glBindRenderbuffer(GL_RENDERBUFFER, modelThumbnailDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, ModelThumbnailSize, ModelThumbnailSize);
    }

    glBindRenderbuffer(GL_RENDERBUFFER, static_cast<GLuint>(oldRenderbuffer));

    GLint oldActiveTexture = GL_TEXTURE0;
    GLint oldTexture2D = 0;

    glGetIntegerv(GL_ACTIVE_TEXTURE, &oldActiveTexture);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture2D);

    GLuint thumbnail = 0;

    glGenTextures(1, &thumbnail);
    glBindTexture(GL_TEXTURE_2D, thumbnail);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, ModelThumbnailSize, ModelThumbnailSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLint oldFramebuffer = 0;
    GLint oldViewport[4]{};
    GLint oldProgram = 0;
    GLint oldVao = 0;
    GLint oldArrayBuffer = 0;
    GLint oldPolygonMode[2]{};
    GLint oldDepthFunction = GL_LESS;
    GLfloat oldClearColor[4]{};

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFramebuffer);
    glGetIntegerv(GL_VIEWPORT, oldViewport);
    glGetIntegerv(GL_CURRENT_PROGRAM, &oldProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &oldVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &oldArrayBuffer);
    glGetIntegerv(GL_POLYGON_MODE, oldPolygonMode);
    glGetIntegerv(GL_DEPTH_FUNC, &oldDepthFunction);
    glGetFloatv(GL_COLOR_CLEAR_VALUE, oldClearColor);

    GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
    GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    GLboolean scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, modelThumbnailFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, thumbnail, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, modelThumbnailDepth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {

        glBindFramebuffer(GL_FRAMEBUFFER, oldFramebuffer);
        glActiveTexture(static_cast<GLenum>(oldActiveTexture));

        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(oldTexture2D));
        glDeleteTextures(1, &thumbnail);

        return 0;
    }

    glViewport(0, 0, ModelThumbnailSize, ModelThumbnailSize);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glClearColor(0.105f, 0.12f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program_thumbnail);
    GLint mvpLocation = glGetUniformLocation(program_thumbnail, "uMVP");
    GLint modelLocation = glGetUniformLocation(program_thumbnail, "uModel");
    GLint colorLocation = glGetUniformLocation(program_thumbnail, "uColor");

    std::vector<PreviewMesh> previewMeshes(previewObjects.size());

    for (size_t i = 0; i < previewObjects.size(); ++i) {

        Object &object = previewObjects[i];

        if (!object.visible || object.vertices.empty() || object.indices.empty())
            continue;

        glm::mat4 model = frameModel * objectModels[i];
        glm::mat4 mvp = previewProjection * previewView * model;

        glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(CreatePreviewMesh(object, previewMeshes[i]));

        auto drawRange = [&](GLuint firstIndex, GLuint indexCount, const glm::vec3 &color) {

            glUniform3fv(colorLocation, 1, glm::value_ptr(color));

            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT, reinterpret_cast<void *>(static_cast<uintptr_t>(firstIndex) * sizeof(GLuint)));
        };

        if (object.submeshes.empty())
            drawRange(0, static_cast<GLuint>(object.indices.size()), glm::vec3(0.72f, 0.76f, 0.84f));
        else
            for (Submesh &submesh : object.submeshes)
                drawRange(submesh.firstIndex, submesh.indexCount, submesh.material.diffuse);
    }

    for (PreviewMesh &mesh : previewMeshes) {

        if (mesh.vao)
            glDeleteVertexArrays(1, &mesh.vao);

        if (mesh.vbo)
            glDeleteBuffers(1, &mesh.vbo);

        if (mesh.ebo)
            glDeleteBuffers(1, &mesh.ebo);
    }

    glBindVertexArray(static_cast<GLuint>(oldVao));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(oldArrayBuffer));
    glUseProgram(static_cast<GLuint>(oldProgram));
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(oldFramebuffer));

    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
    glPolygonMode(GL_FRONT, static_cast<GLenum>(oldPolygonMode[0]));
    glPolygonMode(GL_BACK, static_cast<GLenum>(oldPolygonMode[1]));
    glDepthFunc(static_cast<GLenum>(oldDepthFunction));

    glClearColor(oldClearColor[0], oldClearColor[1], oldClearColor[2], oldClearColor[3]);

    depthWasEnabled ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
    cullWasEnabled ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
    blendWasEnabled ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
    scissorWasEnabled ? glEnable(GL_SCISSOR_TEST) : glDisable(GL_SCISSOR_TEST);

    glActiveTexture(static_cast<GLenum>(oldActiveTexture));
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(oldTexture2D));

    return thumbnail;
}

GLuint GetModelThumbnail(std::string &path) {

    auto found = modelThumbnailCache.find(path);

    if (found != modelThumbnailCache.end())
        return found->second;

    GLuint thumbnail = RenderModelThumbnail(path);

    modelThumbnailCache.emplace(path, thumbnail);

    return thumbnail;
}

GLuint FindModelThumbnail(std::string &path) {

    auto found = modelThumbnailCache.find(path);
    return found == modelThumbnailCache.end() ? 0 : found->second;
}

} // namespace

void CleanBrowserThumbnails() {

    ClearModelThumbnailCache();

    if (modelThumbnailFbo)
        glDeleteFramebuffers(1, &modelThumbnailFbo);

    if (modelThumbnailDepth)
        glDeleteRenderbuffers(1, &modelThumbnailDepth);

    modelThumbnailFbo = 0;
    modelThumbnailDepth = 0;
}

void Browser() {

    //----------------------------------------------------
    // browser

    ImGuiWindowFlags a_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

    //----------------------------------------------------
    // icons

    static GLuint folderIcon = 0;

    if (!folderIcon)
        folderIcon = TextureManager::Get("data/Icons/floder_icon.png", false);

    static GLuint homeIcon = 0;

    if (!homeIcon)
        homeIcon = TextureManager::Get("data/Icons/home_button.png", false);

    static GLuint returnIcon = 0;

    if (!returnIcon)
        returnIcon = TextureManager::Get("data/Icons/return_button.png", false);

    static GLuint fileIcon = 0;

    if (!fileIcon)
        fileIcon = TextureManager::Get("data/Icons/file_icon.png", false);

    static GLuint soundIcon = 0;

    if (!soundIcon)
        soundIcon = TextureManager::Get("data/Icons/sound_icon.png", false);

    static GLuint jsonIcon = 0;

    if (!jsonIcon)
        jsonIcon = TextureManager::Get("data/Icons/json_icon.png", false);

    static GLuint refresIcon = 0;

    if (!refresIcon)
        refresIcon = TextureManager::Get("data/Icons/refres_button.png", false);

    //----------------------------------------------------
    // browser window

    ImGui::Begin("Assets", nullptr, a_flags);

    //----------------------------------------------------
    // back button

    if (currentFolder != rootFolder) {

        if (returnIcon) {

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

            if (ImGui::ImageButton("##return", (ImTextureID)(intptr_t)returnIcon, ImVec2(24, 24), ImVec2(0, 1), ImVec2(1, 0))) {

                currentFolder = currentFolder.parent_path();

                ReloadFolder();
            }

            ImGui::PopStyleColor(1);

        } else {

            if (ImGui::Button("<-")) {

                currentFolder = currentFolder.parent_path();
                ReloadFolder();
            }
        }
    } else {

        ImGui::BeginDisabled();

        if (returnIcon) {

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

            ImGui::ImageButton("##return", (ImTextureID)(intptr_t)returnIcon, ImVec2(24, 24), ImVec2(0, 1),
                               ImVec2(1, 0));

            ImGui::PopStyleColor(1);

        } else {

            ImGui::Button("<-");
        }

        ImGui::EndDisabled();
    }

    ImGui::SameLine();

    //----------------------------------------------------
    // home button

    if (homeIcon) {

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

        if (ImGui::ImageButton("##home", (ImTextureID)(intptr_t)homeIcon, ImVec2(24, 24), ImVec2(0, 1), ImVec2(1, 0))) {

            currentFolder = rootFolder;
            ReloadFolder();
        }

        ImGui::PopStyleColor(1);

    } else {

        if (ImGui::Button("Home")) {

            currentFolder = rootFolder;
            ReloadFolder();
        }
    }

    ImGui::SameLine();

    //----------------------------------------------------
    // refresh button

    if (refresIcon) {

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

        if (ImGui::ImageButton("##refresh", (ImTextureID)(intptr_t)refresIcon, ImVec2(24, 24), ImVec2(0, 1), ImVec2(1, 0))) {
            ClearModelThumbnailCache();
            ReloadFolder();
        }

        ImGui::PopStyleColor(1);

    } else {

        if (ImGui::Button("Refresh")) {
            ClearModelThumbnailCache();
            ReloadFolder();
        }
    }

    ImGui::SameLine();

    ImGui::TextUnformatted(currentFolder.string().c_str());

    ImGui::Separator();

    //----------------------------------------------------
    // assets table

    int columns = std::max(1, (int)(ImGui::GetContentRegionAvail().x / 72));

    if (ImGui::BeginTable("AssetsTable", columns, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_NoSavedSettings)) {

        for (int column = 0; column < columns; column++)
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 72);

        for (auto &entry : folderEntries) {

            std::string ext = entry.path().extension().string();
            bool supportedAsset = entry.is_directory() || ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".obj" || ext == ".gltf" || ext == ".glb" || ext == ".exr" || ext == ".hdr" || ext == ".bin" || ext == ".mp4";

            if (!supportedAsset)
                continue;

            ImGui::TableNextColumn();

            ImGui::PushID(entry.path().string().c_str());

            //------------------------------------------------
            // directories

            if (entry.is_directory()) {

                if (folderIcon) {

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

                    if (ImGui::ImageButton("##folder", (ImTextureID)(intptr_t)folderIcon, ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0))) {

                        newFolder = entry.path();
                        reloadFolder = true;
                    }

                    ImGui::PopStyleColor(1);

                } else {

                    if (ImGui::Button("Folder", ImVec2(64, 64))) {

                        newFolder = entry.path();
                        reloadFolder = true;
                    }
                }

                ImGui::TextWrapped("%s", entry.path().filename().string().c_str());

            } else {

                //------------------------------------------------
                // image files

                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {

                    std::string path = entry.path().string();

                    if (path.rfind("../", 0) == 0)
                        path.erase(0, 3);

                    GLuint tex = TextureManager::Get(path, false);

                    if (ImGui::ImageButton("##image", (ImTextureID)(intptr_t)tex, ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0))) {

                        if (selectedObject >= 0 && selectedObject < (int)objects.size() &&
                            objects[selectedObject].hasMaterial) {

                            objects[selectedObject].obj_texture = tex;
                            objects[selectedObject].tex = path;
                        }
                    }

                    if (ImGui::BeginDragDropSource()) {

                        ImGui::SetDragDropPayload("IMAGE_FILE", path.c_str(), path.size() + 1);

                        ImGui::Image((ImTextureID)(intptr_t)tex, ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));
                        ImGui::Text("%s", entry.path().filename().string().c_str());

                        ImGui::EndDragDropSource();
                    }

                    ImGui::TextWrapped("%s", entry.path().stem().string().c_str());
                }

                //------------------------------------------------
                // OBJ models

                else if (ext == ".obj" || ext == ".gltf" || ext == ".glb") {

                    std::string path = entry.path().string();

                    GLuint thumbnail = ImGui::IsRectVisible(ImVec2(64, 88)) ? GetModelThumbnail(path) : FindModelThumbnail(path);
                    GLuint modelIcon = thumbnail ? thumbnail : fileIcon;

                    bool clicked = false;

                    if (modelIcon) {

                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

                        clicked = ImGui::ImageButton("##obj", (ImTextureID)(intptr_t)modelIcon, ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));

                        ImGui::PopStyleColor(1);

                    } else {

                        clicked = ImGui::Button("OBJ", ImVec2(64, 64));
                    }

                    if (clicked) {

                        std::vector<Object> imported;

                        if (ext == ".obj") {

                            Object obj;

                            init_obj(path.c_str(), obj);

                            imported.push_back(std::move(obj));

                        } else if (!ImportGltf(path, imported))
                            std::cerr << "Could not import glTF model: " << path << '\n';

                        size_t firstImported = objects.size();

                        EntityID importGroupID = 0;

                        if (!imported.empty() && (ext == ".gltf" || ext == ".glb")) {

                            Object group = CreateEmptyObject();

                            group.name = GenerateObjectName(entry.path().stem().string());
                            group.position = glm::vec3(0.0f, 50.0f, 0.0f);
                            group.model = GetModelMatrix(group);

                            importGroupID = group.id;

                            objects.push_back(std::move(group));
                        }

                        for (size_t importedIndex = 0; importedIndex < imported.size(); ++importedIndex) {

                            Object &obj = imported[importedIndex];

                            if (ext == ".obj")
                                PrepareImportedObject(obj);

                            obj.id = GenerateEntityID();
                            obj.parentID = importGroupID;
                            obj.importedMeshIndex = static_cast<uint32_t>(importedIndex);

                            obj.name = GenerateObjectName(obj.name.empty() ? entry.path().stem().string() : obj.name);

                            obj.mesh = path;

                            buffer(obj);

                            if (importGroupID == 0)
                                obj.position += glm::vec3(0.0f, 50.0f, 0.0f);
                            obj.scale *= glm::vec3(obj.importScale);

                            obj.shininess = 20.0f;
                            obj.hasNormalMap = false;

                            obj.model = GetModelMatrix(obj);

                            obj.obj_texture = 0;

                            obj.tex.clear();

                            objects.push_back(std::move(obj));
                        }

                        if (objects.size() > firstImported)
                            selectedObject = static_cast<int>(firstImported);
                    }

                    if (ImGui::BeginDragDropSource()) {

                        ImGui::SetDragDropPayload("OBJ_FILE", path.c_str(), path.size() + 1);

                        if (modelIcon) {

                            ImGui::Image((ImTextureID)(intptr_t)modelIcon, ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));

                        } else {

                            ImGui::TextUnformatted("OBJ");
                        }

                        ImGui::Text("%s", entry.path().filename().string().c_str());

                        ImGui::EndDragDropSource();
                    }

                    ImGui::TextWrapped("%s", entry.path().stem().string().c_str());
                }

                //------------------------------------------------
                // HDR / EXR files

                else if (ext == ".exr" || ext == ".hdr") {

                    std::string path = entry.path().string();

                    if (path.rfind("../", 0) == 0)
                        path.erase(0, 3);

                    bool clicked = false;

                    if (fileIcon) {

                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

                        clicked = ImGui::ImageButton("##hdr", (ImTextureID)(intptr_t)fileIcon, ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));

                        ImGui::PopStyleColor(1);

                    } else {

                        clicked = ImGui::Button("HDR", ImVec2(64, 64));
                    }

                    if (clicked) {

                        init_skybox(path.c_str(), false);
                        st.skybox = path;
                    }

                    ImGui::TextWrapped("%s", entry.path().stem().string().c_str());
                }

                //------------------------------------------------
                // scene files

                else if (ext == ".bin") {

                    std::string path = entry.path().string();

                    bool clicked = false;

                    if (jsonIcon) {

                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

                        clicked = ImGui::ImageButton("##scene", (ImTextureID)(intptr_t)jsonIcon, ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));

                        ImGui::PopStyleColor(1);

                    } else {

                        clicked = ImGui::Button("Scene", ImVec2(64, 64));
                    }

                    if (clicked)
                        load_scene_ok(path.c_str());

                    ImGui::TextWrapped("%s", entry.path().stem().string().c_str());
                }

                //------------------------------------------------
                // video files

                else if (ext == ".mp4") {

                    std::string path = entry.path().string();

                    bool clicked = false;

                    if (soundIcon) {

                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

                        clicked = ImGui::ImageButton("##video", (ImTextureID)(intptr_t)soundIcon, ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));

                        ImGui::PopStyleColor(1);

                    } else {

                        clicked = ImGui::Button("Video", ImVec2(64, 64));
                    }

                    if (clicked) {

                        videoPlayer.Open(path.c_str());
                        videoPlayer.Play();

                        isCinematicActive = true;
                    }

                    ImGui::TextWrapped("%s", entry.path().stem().string().c_str());
                }
            }

            ImGui::PopID();
        }

        //----------------------------------------------------
        // reload folder

        if (reloadFolder) {

            currentFolder = newFolder;

            ReloadFolder();

            reloadFolder = false;
        }

        ImGui::EndTable();
    }

    ImGui::End();
}
