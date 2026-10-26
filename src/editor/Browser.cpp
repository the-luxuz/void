#include "Browser.h"

void Browser(){

    //----------------------------------------------------
    // browser

    ImGuiWindowFlags a_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

    ImGui::Begin("Assets", nullptr, a_flags);

    if (currentFolder != rootFolder) {

        if (ImGui::Button("<-")) {

            currentFolder = currentFolder.parent_path();
            ReloadFolder();
        }
    } else {

        ImGui::BeginDisabled();
        ImGui::Button("<-");
        ImGui::EndDisabled();
    }

    ImGui::SameLine();

    if (ImGui::Button("Home")) {

        currentFolder = rootFolder;
        ReloadFolder();
    }

    ImGui::SameLine();

    ImGui::TextUnformatted(currentFolder.string().c_str());

    ImGui::Separator();

    int columns = std::max(1, (int)(ImGui::GetContentRegionAvail().x / cellSize));

    if (ImGui::BeginTable("AssetsTable", columns)) {

        for (auto &entry : folderEntries) {

            ImGui::TableNextColumn();

            ImGui::PushID(entry.path().string().c_str());

            if (entry.is_directory()) {

                if (ImGui::Button("", ImVec2(64, 64))) {

                    newFolder = entry.path();
                    reloadFolder = true;
                }

                ImGui::TextWrapped("%s", entry.path().filename().string().c_str());
            } else {

                auto ext = entry.path().extension().string();

                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {

                    ImGui::PushID(entry.path().string().c_str());

                    std::string path = entry.path().string();

                    if (path.rfind("../", 0) == 0)
                        path.erase(0, 3);

                    GLuint tex = TextureManager::Get(path, false);

                    if (ImGui::ImageButton("", (ImTextureID)(intptr_t)tex, ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0))) {

                        if (selectedObject != -1 && objects[selectedObject].hasMaterial) {

                            objects[selectedObject].obj_texture = tex;
                            objects[selectedObject].tex = path;
                        }
                    }

                    if (ImGui::BeginDragDropSource()) {

                        ImGui::SetDragDropPayload("IMAGE_FILE", path.c_str(), path.size() + 1);

                        ImGui::Image((ImTextureID)(intptr_t)tex, ImVec2(64, 64));
                        ImGui::Text("%s", entry.path().filename().string().c_str());

                        ImGui::EndDragDropSource();
                    }

                    ImGui::TextWrapped("%s", entry.path().stem().string().c_str());

                    ImGui::PopID();
                }

                if (ext == ".obj") {

                    std::string path = entry.path().string();

                    if (ImGui::Button("", ImVec2(64, 64))) {

                        Object obj;

                        init_obj(path.c_str(), obj);
                        PrepareImportedObject(obj);

                        obj.id = GenerateEntityID();
                        obj.name = GenerateObjectName(entry.path().stem().string());
                        obj.mesh = path;

                        buffer(obj);

                        obj.position = glm::vec3(0.0f, 50.0f, 0.0f);
                        obj.rotation = glm::quat(1.f, 0.f, 0.f, 0.f);
                        obj.scale = glm::vec3(obj.importScale);

                        obj.original_vertices = obj.vertices;
                        obj.original_indices = obj.indices;

                        obj.shininess = 20.0f;
                        obj.hasNormalMap = false;
                        obj.model = GetModelMatrix(obj);

                        obj.obj_texture = 0;
                        obj.tex.clear();

                        objects.push_back(std::move(obj));
                        selectedObject = static_cast<int>(objects.size()) - 1;
                    }

                    if (ImGui::BeginDragDropSource()) {

                        ImGui::SetDragDropPayload("OBJ_FILE", path.c_str(), path.size() + 1);

                        ImGui::Image((ImTextureID)(intptr_t)nullptr, ImVec2(64, 64));
                        ImGui::Text("%s", entry.path().filename().string().c_str());

                        ImGui::EndDragDropSource();
                    }

                    ImGui::TextWrapped("%s", entry.path().stem().string().c_str());
                }

                if (ext == ".exr" || ext == ".hdr") {

                    std::string path = entry.path().string();

                    if (path.rfind("../", 0) == 0)
                        path.erase(0, 3);

                    if (ImGui::Button("", ImVec2(64, 64))) {

                        init_skybox(path.c_str(), false);
                        st.skybox = path;
                    }

                    ImGui::TextWrapped("%s", entry.path().stem().string().c_str());
                }

                if (ext == ".bin") {

                    std::string path = entry.path().string();

                    if (ImGui::Button("", ImVec2(64, 64))) {

                        load_scene_ok(path.c_str());
                    }

                    ImGui::TextWrapped("%s", entry.path().stem().string().c_str());
                }

                if (ext == ".mp4") {

                    std::string path = entry.path().string();

                    if (ImGui::Button("", ImVec2(64, 64))) {

                        videoPlayer.Open(path.c_str());
                        videoPlayer.Play();

                        isCinematicActive = true;
                    }

                    ImGui::TextWrapped("%s", entry.path().stem().string().c_str());
                }
            }

            ImGui::PopID();
        }

        if (reloadFolder) {

            currentFolder = newFolder;

            ReloadFolder();

            reloadFolder = false;
        }

        ImGui::EndTable();
    }

    ImGui::End();
}