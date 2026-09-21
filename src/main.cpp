#define TINYOBJLOADER_IMPLEMENTATION

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <cstdlib>

#include "Parser.h"
#include "buffers.h"
#include "classes.h"
#include "helpers.h"
#include "inits.h"
#include "local.h"
#include "programs.h"
#include "textures.h"
#include "updates.h"

float totalTime = 0.0f;

static float fps = 0.0f;

constexpr float fixedDt = 1.0f / 60.0f;
constexpr int maxSimulationStepsPerFrame = 6;

Object *player = nullptr;

SDL_DisplayID display;
SDL_Rect rect;

SDL_Event event;

constexpr float angle_speed = glm::radians(90.0f);
float radius_speed = 20.0f;

void update(SDL_Window *window, float dt) {

    if (!isCinematicActive) {

        eventManager.Dispatch();

        if (st.light_active && st.shadow_active)
            update_depthmap();

        if (currentViewMode == view_Mode::PLAY)
            player = search_obj_by_name("Player");

        if (currentViewMode == view_Mode::PLAY && player && !free_cam) {

            cameraPos = cam.cam_position;

        } else {

            cameraPos = get_cam_pos();
        }

        view = glm::lookAt(cameraPos, cam.cam_target, cam.cam_up);

        if (currentViewMode != prevViewMode && currentViewMode == view_Mode::PLAY) {

            save_scene_state();
            prevViewMode = currentViewMode;

        } else if (currentViewMode != prevViewMode && currentViewMode == view_Mode::EDITOR) {

            load_scene_state();
            prevViewMode = currentViewMode;
        }

        if (camera_update) {

            projection = glm::perspective(glm::radians(st.fov), (float)viewport_width / viewport_height, cam.nearPlane,
                                          cam.farPlane);

            camera_update = false;
        }

        if (currentViewMode == view_Mode::EDITOR) {

            if ((int)viewportSize.x != viewport_width || (int)viewportSize.y != viewport_height)
                ResizeViewport((int)viewportSize.x, (int)viewportSize.y);

            glBindFramebuffer(GL_FRAMEBUFFER, viewportFBO);

            glViewport(0, 0, viewport_width, viewport_height);

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            if (currentViewMode == view_Mode::EDITOR) {

                //----------------------------------------------------
                // grid

                update_grid(cameraPos);
            }

            //----------------------------------------------------
            // obj

            update_objects();

            //----------------------------------------------------
            // skybox

            update_skybox();

            //----------------------------------------------------

            glBindTexture(GL_TEXTURE_2D, 0);
            glBindVertexArray(0);
            glUseProgram(0);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }

    //----------------------------------------------------

    glViewport(0, 0, st.w, st.h);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!isCinematicActive) {

        static ImGuiIO &io = ImGui::GetIO();
        (void)io;

        if (currentViewMode == view_Mode::PLAY) {

            io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;

            //----------------------------------------------------
            // obj

            update_objects();

            //----------------------------------------------------
            // skybox

            update_skybox();

            //----------------------------------------------------
        }

        //----------------------------------------------------
        // imgui / imguizmo

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        if (currentViewMode == view_Mode::EDITOR) {

            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
            Docking();
        }

        fps = ImGui::GetIO().Framerate;

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

        ImGui::Begin("HUD", nullptr, flags);

        ImGui::Text("FPS: %.1f", fps);
        ImGui::Text("Frame: %.3f ms", 1000.0f / fps);

        ImGui::End();

        if (currentViewMode == view_Mode::EDITOR)
            update_imgui_guizmo();

        ImGui::Render();

        if (currentViewMode == view_Mode::EDITOR) {

            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {

                SDL_Window *backupWindow = SDL_GL_GetCurrentWindow();
                SDL_GLContext backupContext = SDL_GL_GetCurrentContext();

                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();

                SDL_GL_MakeCurrent(backupWindow, backupContext);
            }
        }

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    //----------------------------------------------------
    // windows

    if (st.last_win_mode != st.win_mode) {

        ApplyWindowMode(window);
        init_viewport();
    }

    if (isCinematicActive)
        videoPlayer.Update(dt);

    if (isCinematicActive && videoPlayer.IsFinished()) {

        isCinematicActive = false;
    }

    if (isCinematicActive) {

        DrawVideo(videoPlayer);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Audio");

        bool audio = videoPlayer.IsAudioEnabled();

        if (ImGui::Checkbox("Audio", &audio))
            videoPlayer.SetAudioEnabled(audio);

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);

    SDL_GL_SwapWindow(window);
}

void Rebuild() {

    for (int i = 0; i < objects.size(); i++) {

        Object &obj = objects[i];

        // Build_Normals(obj);
        Build_TBN(obj);

        UpdateMeshBuffers(obj);
        UpdateLineBuffers(obj);
    }
}

int main(int argc, char *args[]) {

    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    init_("../data/settings.bin", "../data/scene/scene.bin");

    Uint64 lastCounter = SDL_GetPerformanceCounter();
    float simulationAccumulator = 0.0f;

    while (!quit) {

        Uint64 currentCounter = SDL_GetPerformanceCounter();

        float dt = float(currentCounter - lastCounter) / float(SDL_GetPerformanceFrequency());

        lastCounter = currentCounter;

        dt = glm::min(dt, 0.1f);

        while (SDL_PollEvent(&event)) {

            ImGui_ImplSDL3_ProcessEvent(&event);

            ImGuiIO &io = ImGui::GetIO();

            switch (event.type) {

            case SDL_EVENT_QUIT:
                quit = true;
                break;

            case SDL_EVENT_KEY_DOWN:

                if (event.key.scancode == SDL_SCANCODE_ESCAPE)
                    quit = true;

                if (currentViewMode == view_Mode::EDITOR && !io.WantCaptureKeyboard) {

                    if (event.key.scancode == SDL_SCANCODE_Y) {

                        shadow_map_render = !shadow_map_render;
                    }

                    if (event.key.scancode == SDL_SCANCODE_N) {

                        normals_appear = !normals_appear;
                        // Rebuild();
                    }

                    if (event.key.scancode == SDL_SCANCODE_F) {
                        face_normal = !face_normal;
                        // Rebuild();
                    }

                    if (event.key.scancode == SDL_SCANCODE_BACKSPACE) {

                        if (selectedObject != -1) {

                            if (selectedObject < (int)objects.size()) {
                                DetachChildren(objects[selectedObject].id);

                                DestroyBuffers(objects[selectedObject]);

                                objects[selectedObject].vertices.clear();
                                objects[selectedObject].indices.clear();

                                objects[selectedObject].normalLines.clear();

                                objects[selectedObject].tangentLines.clear();
                                objects[selectedObject].bitangentLines.clear();
                                objects[selectedObject].normalLinesTBN.clear();

                                objects[selectedObject].original_vertices.clear();
                                objects[selectedObject].original_indices.clear();

                                objects.erase(objects.begin() + selectedObject);

                            } else {

                                int lightIndex = selectedObject - (int)objects.size();

                                if (lightIndex >= 0 && lightIndex < (int)lightObjects.size()) {

                                    DestroyBuffers(lightObjects[lightIndex]);

                                    lightObjects[lightIndex].vertices.clear();
                                    lightObjects[lightIndex].indices.clear();

                                    lightObjects[lightIndex].normalLines.clear();

                                    lightObjects[lightIndex].tangentLines.clear();
                                    lightObjects[lightIndex].bitangentLines.clear();
                                    lightObjects[lightIndex].normalLinesTBN.clear();

                                    lightObjects[lightIndex].original_vertices.clear();
                                    lightObjects[lightIndex].original_indices.clear();

                                    lights.erase(lights.begin() + lightIndex);
                                    lightObjects.erase(lightObjects.begin() + lightIndex);
                                }
                            }

                            selectedObject = -1;
                        }
                    }

                    if (event.key.scancode == SDL_SCANCODE_F5) {
                        read_textures_path();
                    }

                    if (event.key.mod & SDL_KMOD_CTRL && event.key.scancode == SDL_SCANCODE_Z) {

                        if (selectedObject != -1) {

                            if (selectedObject < objects.size()) {

                                Object &obj = objects[selectedObject];

                                if (!obj.undoStack.empty()) {

                                    obj.redoStack.push_back(CaptureState(obj));
                                    ApplyState(obj, obj.undoStack.back());
                                    obj.undoStack.pop_back();
                                    UpdateMapping(obj);
                                }
                            } else {

                                int lightIndex = selectedObject - objects.size();

                                Object &obj = lightObjects[lightIndex];
                                Light &light = lights[lightIndex];

                                if (!obj.undoStack.empty()) {

                                    obj.redoStack.push_back(CaptureState(obj));
                                    light.redoStack.push_back(CaptureLightState(light));

                                    ApplyState(obj, obj.undoStack.back());
                                    ApplyLightState(light, light.undoStack.back());

                                    obj.undoStack.pop_back();
                                    light.undoStack.pop_back();

                                    light.pos = obj.position;
                                    light.dir = glm::normalize(obj.rotation * glm::vec3(0.0f, -1.0f, 0.0f));

                                    obj.model = GetModelMatrix(obj);
                                    obj.mvp = projection * view * obj.model;
                                }
                            }
                        }
                    }

                    if (event.key.mod & SDL_KMOD_CTRL && event.key.scancode == SDL_SCANCODE_Y) {

                        if (selectedObject != -1) {

                            if (selectedObject < objects.size()) {

                                Object &obj = objects[selectedObject];

                                if (!obj.redoStack.empty()) {

                                    obj.undoStack.push_back(CaptureState(obj));
                                    ApplyState(obj, obj.redoStack.back());
                                    obj.redoStack.pop_back();
                                    UpdateMapping(obj);
                                }
                            } else {

                                int lightIndex = selectedObject - objects.size();

                                Object &obj = lightObjects[lightIndex];
                                Light &light = lights[lightIndex];

                                if (!obj.redoStack.empty()) {

                                    obj.undoStack.push_back(CaptureState(obj));
                                    light.undoStack.push_back(CaptureLightState(light));

                                    ApplyState(obj, obj.redoStack.back());
                                    ApplyLightState(light, light.redoStack.back());

                                    obj.redoStack.pop_back();
                                    light.redoStack.pop_back();

                                    light.pos = obj.position;
                                    light.dir = glm::normalize(obj.rotation * glm::vec3(0.0f, -1.0f, 0.0f));

                                    obj.model = GetModelMatrix(obj);
                                    obj.mvp = projection * view * obj.model;
                                }
                            }
                        }
                    }

                    if ((event.key.mod & SDL_KMOD_CTRL) && event.key.scancode == SDL_SCANCODE_C) {

                        if (selectedObject != -1) {

                            if (selectedObject < (int)objects.size()) {

                                objCopy = objects[selectedObject];
                                hasObjectCopy = true;
                                hasLightCopy = false;

                            } else {

                                int lightIndex = selectedObject - (int)objects.size();

                                objCopy = lightObjects[lightIndex];
                                lightCopy = lights[lightIndex];

                                hasObjectCopy = true;
                                hasLightCopy = true;
                            }
                        }
                    }

                    if ((event.key.mod & SDL_KMOD_CTRL) && event.key.scancode == SDL_SCANCODE_V) {

                        if (hasObjectCopy) {

                            if (hasLightCopy) {

                                Object copy = objCopy;

                                copy.vao = 0;
                                copy.vbo = 0;
                                copy.ebo = 0;

                                copy.normalVAO = 0;
                                copy.normalVBO = 0;

                                copy.normalTBVAO = 0;
                                copy.normalTBVBO = 0;

                                copy.tangentVAO = 0;
                                copy.tangentVBO = 0;

                                copy.bitangentVAO = 0;
                                copy.bitangentVBO = 0;

                                buffer(copy);

                                lightObjects.push_back(copy);
                                lights.push_back(lightCopy);

                                selectedObject = (int)(objects.size() + lightObjects.size() - 1);

                            } else {

                                Object copy = objCopy;

                                copy.vao = 0;
                                copy.vbo = 0;
                                copy.ebo = 0;

                                copy.normalVAO = 0;
                                copy.normalVBO = 0;

                                copy.normalTBVAO = 0;
                                copy.normalTBVBO = 0;

                                copy.tangentVAO = 0;
                                copy.tangentVBO = 0;

                                copy.bitangentVAO = 0;
                                copy.bitangentVBO = 0;

                                buffer(copy);

                                objects.push_back(copy);
                                selectedObject = (int)objects.size() - 1;
                            }
                        }
                    }

                    if (event.key.mod & SDL_KMOD_CTRL && event.key.scancode == SDL_SCANCODE_S) {

                        cam.cam_position = cameraPos;

                        if (save_outPath != "")
                            par.save("../data/settings.bin", save_outPath.c_str(), st, cam, objects, lightObjects,
                                     lights);
                        else {

                            nfdu8char_t *outPath = nullptr;

                            nfdfilteritem_t filterItem[1] = {{"Text file", "bin"}};

                            nfdresult_t result = NFD_SaveDialog(&outPath, filterItem, 1, nullptr, "scene.bin");

                            if (result == NFD_OKAY) {

                                par.save("../data/settings.bin", outPath, st, cam, objects, lightObjects, lights);
                                save_outPath = outPath;
                                NFD_FreePath(outPath);
                            }
                        }
                    }

                    if (event.key.mod & SDL_KMOD_CTRL && event.key.scancode == SDL_SCANCODE_O) {

                        nfdu8char_t *outPath = nullptr;

                        dont_reload_cam = true;

                        nfdfilteritem_t filterItem[1] = {{"Text file", "bin"}};

                        nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);

                        if (result == NFD_OKAY) {

                            std::string path = outPath;

                            save_outPath = path;
                            open_init("../data/settings.bin", path.c_str(), st);

                            NFD_FreePath(outPath);
                        }
                    }
                }

                if (event.key.scancode == SDL_SCANCODE_F11) {

                    switch (st.win_mode) {

                    case window_mode::WINDOWED:
                        st.win_mode = window_mode::WINDOWED_BORDERLESS;
                        break;

                    case window_mode::WINDOWED_BORDERLESS:
                        st.win_mode = window_mode::FULLSCREEN;
                        break;

                    case window_mode::FULLSCREEN:
                        st.win_mode = window_mode::WINDOWED;
                        break;
                    }
                }

                if (event.key.scancode == SDL_SCANCODE_SPACE && !io.WantCaptureKeyboard) {

                    if (currentViewMode == view_Mode::EDITOR) {

                        currentViewMode = view_Mode::PLAY;
                        SDL_SetWindowRelativeMouseMode(window, true);

                    } else {

                        currentViewMode = view_Mode::EDITOR;
                        SDL_SetWindowRelativeMouseMode(window, false);
                        free_cam = false;
                    }
                }

                break;

            case SDL_EVENT_MOUSE_MOTION:

                if (currentViewMode == view_Mode::EDITOR || free_cam) {
                    
                    if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LCTRL]) {
                    
                        SDL_SetWindowRelativeMouseMode(window, true);

                        cam.alpha += event.motion.xrel * mouseSensitivity;

                        cam.beta -= event.motion.yrel * mouseSensitivity;

                        cam.beta = glm::clamp(cam.beta, 0.05f, glm::pi<float>() - 0.05f);
                    } 
                } else {
                
                    if (SDL_GetWindowRelativeMouseMode(window)) {

                        cam.alpha += event.motion.xrel * mouseSensitivity;
                        cam.beta -= event.motion.yrel * mouseSensitivity;

                        if (free_cam || !player) {

                            cam.beta = glm::clamp(cam.beta, 0.05f, glm::pi<float>() - 0.05f);
                            
                        } else {
                        
                            constexpr float maxPitch = glm::radians(89.0f);

                            cam.beta = glm::clamp(cam.beta, -maxPitch, maxPitch);
                        }
                    }

                }

                break;

            case SDL_EVENT_KEY_UP:

                if (event.key.scancode == SDL_SCANCODE_LCTRL && currentViewMode == view_Mode::EDITOR) {

                    SDL_SetWindowRelativeMouseMode(window, false);
                    //SDL_WarpMouseInWindow(window, st.w / 2.0f, st.h / 2.0f);
                }

                if (currentViewMode == view_Mode::PLAY && event.key.scancode == SDL_SCANCODE_J) {
                
                    free_cam = !free_cam;
                }

                break;

            case SDL_EVENT_MOUSE_WHEEL:

                if (viewport_scroll_active || (currentViewMode == view_Mode::PLAY && free_cam)) {

                    cam.radius -= (event.wheel.y * 20);

                    cam.radius = std::max(cam.radius, 1.0f);
                }

                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:

                if (currentViewMode == view_Mode::EDITOR) {

                    if (event.button.button == SDL_BUTTON_LEFT) {

                        if (ImGuizmo::IsOver() || ImGuizmo::IsUsing())
                            break;

                        // if (ImGui::GetIO().WantCaptureMouse)
                        //	break;

                        ImVec2 mouse = ImGui::GetMousePos();

                        float mx = mouse.x - viewportPos.x;
                        float my = mouse.y - viewportPos.y;

                        if (mx < 0 || my < 0 || mx > viewport_width || my > viewport_height)
                            break;

                        BuildRay(mx, my, rayOrigin, rayDir);

                        select_object(rayOrigin, rayDir);

                        /*if (selectedObject != nullptr) {
                            std::cout << "selected" << std::endl;
                        }*/
                    }

                    if (event.button.button == SDL_BUTTON_RIGHT) {
                        selectedObject = -1;
                    }

                    if (event.button.button == SDL_BUTTON_X1) {

                        switch (currentTool) {

                        case Tools::TRANSLATE:
                            currentTool = Tools::SCALE;
                            break;

                        case Tools::ROTATE:
                            currentTool = Tools::TRANSLATE;
                            break;
                        case Tools::SCALE:
                            currentTool = Tools::ROTATE;
                            break;
                        }
                    }

                    if (event.button.button == SDL_BUTTON_X2) {

                        switch (currentTool) {

                        case Tools::TRANSLATE:
                            currentTool = Tools::ROTATE;
                            break;

                        case Tools::ROTATE:
                            currentTool = Tools::SCALE;
                            break;
                        case Tools::SCALE:
                            currentTool = Tools::TRANSLATE;
                            break;
                        }
                    }

                    break;
                }

                break;
            }
        }

        simulationAccumulator += dt;

        const bool *keyboard = SDL_GetKeyboardState(nullptr);

        ImGuiIO &io = ImGui::GetIO();

        int simulationSteps = 0;

        while (simulationAccumulator >= fixedDt && simulationSteps < maxSimulationStepsPerFrame) {

            if (!isCinematicActive) {

                if ((free_cam || currentViewMode == view_Mode::EDITOR) && !io.WantCaptureKeyboard)
                    controls(keyboard, angle_speed, radius_speed, fixedDt);

                if (currentViewMode == view_Mode::PLAY) {

                    player = search_obj_by_name("Player");

                    if (player && !free_cam && !io.WantCaptureKeyboard) {

                        PlayerControl(keyboard, fixedDt, *player);
                        UpdatePlayerCamera(*player);

                    } else if (!player) {

                        controls(keyboard, angle_speed, radius_speed, fixedDt);
                    }

                    if (st.physics_active)
                        PhysicsUpdate(fixedDt);
                    if (st.collision_active)
                        CollisionUpdate();
                }

                UpdateAnimation(fixedDt);
            }

            simulationAccumulator -= fixedDt;
            ++simulationSteps;
        }

        // Do not try to catch up indefinitely after a breakpoint, minimize,
        // or prolonged hitch. The cap preserves application responsiveness.
        if (simulationSteps == maxSimulationStepsPerFrame)
            simulationAccumulator = 0.0f;

        totalTime += dt;

        update(window, dt);
    }

    clean();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    videoPlayer.Close();

    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);

    SDL_Quit();

    _CrtDumpMemoryLeaks();

    return 0;
}
