#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <cstdlib>

#ifdef _WIN32
#include <Windows.h>

extern "C" {
// NVIDIA Optimus
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;

// AMD PowerXpress
__declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}
#endif

#include "Parser.h"
#include "buffers.h"
#include "classes.h"
#include "editor/Browser.h"
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

SDL_DisplayID display;
SDL_Rect rect;

SDL_Event event;

static const char *GetImGuiLayoutPath(window_mode mode) {
    return mode == window_mode::WINDOWED ? "imgui_windowed.ini" : "imgui_fullscreen_windowed.ini";
}

constexpr float angle_speed = glm::radians(90.0f);
float radius_speed = 20.0f;

static void RefreshPlayerEntityID() {

    Object *playerObject = search_obj_by_name("Player");
    playerEntityID = playerObject ? playerObject->id : 0;
}

static PlayerInput BuildPlayerInput(const bool *keyboard) {

    PlayerInput input{};
    input.moveForward = keyboard[SDL_SCANCODE_W];
    input.moveBackward = keyboard[SDL_SCANCODE_S];
    input.moveLeft = keyboard[SDL_SCANCODE_A];
    input.moveRight = keyboard[SDL_SCANCODE_D];
    input.jump = keyboard[SDL_SCANCODE_SPACE];
    input.slide = keyboard[SDL_SCANCODE_LSHIFT];

    return input;
}

void update(SDL_Window *window, float dt) {

    const GameAPI *gameAPI = GetGameAPI();

    if (!isCinematicActive) {

        eventManager.Dispatch();

        if (st.light_active && st.shadow_active)
            update_depthmap();

        if (currentViewMode == view_Mode::PLAY)
            RefreshPlayerEntityID();

        if (currentViewMode != prevViewMode) {

            if (currentViewMode == view_Mode::PLAY) {

                save_scene_state();
                gameAPI->OnPlayStart();

                if (playerEntityID != 0)
                    gameAPI->UpdatePlayerCamera(playerEntityID, 0.0f);

                playerRayFired = false;

            } else {

                load_scene_state();
                gameAPI->OnPlayStop();
            }

            prevViewMode = currentViewMode;
        }

        const bool usePlayerCamera = currentViewMode == view_Mode::PLAY && playerEntityID != 0 && !free_cam;
        GameCameraState playerCamera{};

        if (usePlayerCamera) {

            gameAPI->GetCameraState(&playerCamera);
            cameraPos = glm::vec3(playerCamera.position[0], playerCamera.position[1], playerCamera.position[2]);
            glm::vec3 cameraTarget(playerCamera.target[0], playerCamera.target[1], playerCamera.target[2]);
            glm::vec3 cameraUp(playerCamera.up[0], playerCamera.up[1], playerCamera.up[2]);
            view = glm::lookAt(cameraPos, cameraTarget, cameraUp);

            // ray cast of player camera to shoot
            BuildRay(viewport_width * 0.5f, viewport_height * 0.5f, rayOrigin, rayDir);
            rayOrigin = cameraPos;

        } else {

            cameraPos = get_cam_pos();
            view = glm::lookAt(cameraPos, cam.cam_target, cam.cam_up);
        }

        float activeFov = st.fov + (usePlayerCamera ? playerCamera.speedFovOffset : 0.0f);
        static float lastProjectionFov = -1.0f;

        if (camera_update || std::abs(activeFov - lastProjectionFov) > 0.001f) {

            projection = glm::perspective(glm::radians(activeFov), (float)viewport_width / viewport_height, cam.nearPlane, cam.farPlane);

            camera_update = false;
            lastProjectionFov = activeFov;
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

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

        static constexpr int FPS_HISTORY_SIZE = 180;

        static float fpsHistory[FPS_HISTORY_SIZE] = {};
        static int fpsHistoryCount = 0;

        const float currentFPS = io.DeltaTime > 0.0f ? 1.0f / io.DeltaTime : 0.0f;

        if (currentFPS > 0.0f) {
            if (fpsHistoryCount < FPS_HISTORY_SIZE) {
                fpsHistory[fpsHistoryCount++] = currentFPS;
            } else {
                for (int i = 0; i < FPS_HISTORY_SIZE - 1; ++i)
                    fpsHistory[i] = fpsHistory[i + 1];

                fpsHistory[FPS_HISTORY_SIZE - 1] = currentFPS;
            }
        }

        float fpsLow = FLT_MAX;
        float fpsHigh = 0.0f;

        for (int i = 0; i < fpsHistoryCount; ++i) {

            fpsLow = std::min(fpsLow, fpsHistory[i]);
            fpsHigh = std::max(fpsHigh, fpsHistory[i]);
        }

        ImGui::SetNextWindowSize(ImVec2(275.0f, 30.0f));

        if (ImGui::Begin("FPS Debug", nullptr, flags)) {

            ImGui::Text("FPS: %.1f", currentFPS);
            ImGui::SameLine();

            ImGui::Text("Low: %.1f", fpsLow);
            ImGui::SameLine();

            ImGui::Text("High: %.1f", fpsHigh);
        }

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

        const char *previousLayout = GetImGuiLayoutPath(st.last_win_mode);
        const char *nextLayout = GetImGuiLayoutPath(st.win_mode);

        // Keep separate dock layouts for desktop windowed and fullscreen
        // windowed sizes, so changing display mode doesn't distort the panels.
        ImGui::SaveIniSettingsToDisk(previousLayout);
        ApplyWindowMode(window);
        init_viewport();

        std::ifstream savedLayout(nextLayout, std::ios::binary);
        if (savedLayout.good())
            ImGui::LoadIniSettingsFromDisk(nextLayout);

        ImGui::GetIO().IniFilename = nextLayout;
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

int main() {

    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    init_("../data/settings.bin", "../data/scene/scene.bin");

    const GameAPI *gameAPI = GetGameAPI();

    if (!gameAPI || gameAPI->version != GAME_API_VERSION || !gameAPI->Load()) {

        clean();
        return 1;
    }

    std::cout << "\n========== OPENGL INFO ==========\n";

    std::cout << "Vendor: " << glGetString(GL_VENDOR) << '\n';

    std::cout << "Renderer: " << glGetString(GL_RENDERER) << '\n';

    std::cout << "Version: " << glGetString(GL_VERSION) << '\n';

    std::cout << "=================================\n\n";

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

                if (event.key.mod & SDL_KMOD_CTRL && event.key.scancode == SDL_SCANCODE_Q)
                    quit = true;

                if (currentViewMode == view_Mode::PLAY) {
                    if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                        currentViewMode = view_Mode::EDITOR;
                        free_cam = false;
                        SDL_SetWindowRelativeMouseMode(window, false);
                        SDL_ShowCursor();
                    }
                }

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
                                DeleteObjectHierarchy(objects[selectedObject].id);
                                selectedObject = -1;

                            } else {

                                size_t lightIndex = selectedObject - objects.size();

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

                                size_t lightIndex = selectedObject - objects.size();

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

                                size_t lightIndex = selectedObject - objects.size();

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

                                size_t lightIndex = selectedObject - objects.size();

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
                                Light copiedLight = lightCopy;
                                EntityID copiedID = GenerateEntityID();

                                copy.id = copiedID;
                                copiedLight.id = copiedID;

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
                                lights.push_back(copiedLight);

                                selectedObject = (int)(objects.size() + lightObjects.size() - 1);

                            } else {

                                Object copy = objCopy;
                                copy.id = GenerateEntityID();

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

                    if (event.key.mod & SDL_KMOD_CTRL && event.key.scancode == SDL_SCANCODE_G) {

                        cam.cam_position = cameraPos;

                        if (save_outPath != "")
                            par.save("../data/settings.bin", save_outPath.c_str(), st, cam, objects, lightObjects, lights);
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

                    if (st.win_mode == window_mode::WINDOWED) {
                        SDL_GetWindowSize(window, &windowedSize.x, &windowedSize.y);
                        st.windowed_w = windowedSize.x;
                        st.windowed_h = windowedSize.y;
                    }

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

                if (event.key.scancode == SDL_SCANCODE_P && !io.WantCaptureKeyboard) {

                    if (currentViewMode == view_Mode::EDITOR) {

                        currentViewMode = view_Mode::PLAY;
                        RefreshPlayerEntityID();
                        SDL_SetWindowRelativeMouseMode(window, playerEntityID != 0);

                    } else {

                        currentViewMode = view_Mode::EDITOR;
                        SDL_SetWindowRelativeMouseMode(window, false);
                        free_cam = false;
                    }
                }

                break;

            case SDL_EVENT_MOUSE_MOTION:

                if (currentViewMode == view_Mode::EDITOR || free_cam || (currentViewMode == view_Mode::PLAY && playerEntityID == 0)) {
                    
                    if (SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LCTRL]) {
                    
                        SDL_SetWindowRelativeMouseMode(window, true);

                        SDL_WarpMouseInWindow(window, viewport_width / 2.0f, viewport_height / 2.0f);

                        cam.alpha += event.motion.xrel * mouseSensitivity;

                        cam.beta -= event.motion.yrel * mouseSensitivity;

                        cam.beta = glm::clamp(cam.beta, 0.05f, glm::pi<float>() - 0.05f);
                    } 
                } else {
                
                    if (SDL_GetWindowRelativeMouseMode(window)) {

                        GetGameAPI()->OnMouseLook(event.motion.xrel, event.motion.yrel);
                    }

                }

                break;

            case SDL_EVENT_KEY_UP:

                if (event.key.scancode == SDL_SCANCODE_LCTRL && (currentViewMode == view_Mode::EDITOR || free_cam || (currentViewMode == view_Mode::PLAY && playerEntityID == 0))) {

                    SDL_SetWindowRelativeMouseMode(window, false);
                    //SDL_WarpMouseInWindow(window, st.w / 2.0f, st.h / 2.0f);
                }

                if (currentViewMode == view_Mode::PLAY && event.key.scancode == SDL_SCANCODE_J) {
                
                    free_cam = !free_cam;
                    SDL_SetWindowRelativeMouseMode(window, !free_cam);
                }

                break;

            case SDL_EVENT_MOUSE_WHEEL:

                if (currentViewMode == view_Mode::PLAY && playerEntityID != 0 && !free_cam) {

                    if (event.wheel.y != 0.0f)
                        GetGameAPI()->OnJumpPressed();

                } else if (viewport_scroll_active || (currentViewMode == view_Mode::PLAY && free_cam) || (currentViewMode == view_Mode::PLAY && playerEntityID == 0)) {

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

                } else if (currentViewMode == view_Mode::PLAY && playerEntityID != 0) {
                
                    if (event.button.button == SDL_BUTTON_LEFT) {

                        playerRayFired = true;

                    }
                }

                break;
            }
        }

        simulationAccumulator += dt;

        const bool *keyboard = SDL_GetKeyboardState(nullptr);
        const PlayerInput playerInput = BuildPlayerInput(keyboard);

        ImGuiIO &io = ImGui::GetIO();

        int simulationSteps = 0;

        while (simulationAccumulator >= fixedDt && simulationSteps < maxSimulationStepsPerFrame) {

            if (!isCinematicActive) {

                if ((free_cam || currentViewMode == view_Mode::EDITOR) && !io.WantCaptureKeyboard)
                    controls(keyboard, angle_speed, radius_speed, fixedDt);

                if (currentViewMode == view_Mode::PLAY) {

                    RefreshPlayerEntityID();

                    if (playerEntityID != 0 && !free_cam && !io.WantCaptureKeyboard) {

                        GetGameAPI()->Update(playerEntityID, &playerInput, fixedDt);

                    } else if (playerEntityID == 0 && !free_cam && !io.WantCaptureKeyboard) {

                        controls(keyboard, angle_speed, radius_speed, fixedDt);
                    }

                    if (st.physics_active)
                        PhysicsUpdate(fixedDt);
                    if (st.collision_active)
                        CollisionUpdate();
                    else
                        GetGameAPI()->BeginSimulationStep();

                    if (playerEntityID != 0 && !free_cam)
                        GetGameAPI()->UpdatePlayerCamera(playerEntityID, fixedDt);
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

    gameAPI->Shutdown();

    CleanBrowserThumbnails();
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
