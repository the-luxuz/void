#include "Browser.h"

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

        if (ImGui::ImageButton("##refresh", (ImTextureID)(intptr_t)refresIcon, ImVec2(24, 24), ImVec2(0, 1), ImVec2(1, 0)))
            ReloadFolder();

        ImGui::PopStyleColor(1);

    } else {

        if (ImGui::Button("Refresh"))
            ReloadFolder();
    }

    ImGui::SameLine();

    ImGui::TextUnformatted(currentFolder.string().c_str());

    ImGui::Separator();

    //----------------------------------------------------
    // assets table

    int columns = std::max(1, (int)(ImGui::GetContentRegionAvail().x / cellSize));

    if (ImGui::BeginTable("AssetsTable", columns)) {

        for (auto &entry : folderEntries) {

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

                auto ext = entry.path().extension().string();

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

                    bool clicked = false;

                    if (fileIcon) {

                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

                        clicked = ImGui::ImageButton("##obj", (ImTextureID)(intptr_t)fileIcon, ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));

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

                        for (Object &obj : imported) {

                            if (ext == ".obj")
                                PrepareImportedObject(obj);

                            obj.id = GenerateEntityID();

                            obj.name = GenerateObjectName(obj.name.empty() ? entry.path().stem().string() : obj.name);

                            obj.mesh = path;

                            buffer(obj);

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
                            selectedObject = static_cast<int>(objects.size()) - 1;
                    }

                    if (ImGui::BeginDragDropSource()) {

                        ImGui::SetDragDropPayload("OBJ_FILE", path.c_str(), path.size() + 1);

                        if (fileIcon) {

                            ImGui::Image((ImTextureID)(intptr_t)fileIcon, ImVec2(64, 64), ImVec2(0, 1), ImVec2(1, 0));

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
