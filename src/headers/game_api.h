#pragma once

#include "entity_id.h"

#include <cstdint>

inline constexpr std::uint32_t GAME_API_VERSION = 1;

struct PlayerInput {

    bool moveForward = false;
    bool moveBackward = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool jump = false;
    bool slide = false;
};

struct GameContact {

    bool hit = false;
    float normal[3];
    float penetration = 0.0f;
    float point[3];
};

struct GameCameraState {

    float position[3];
    float target[3];
    float up[3];
    float speedFovOffset = 0.0f;
};

struct GameAPI {

    std::uint32_t version = GAME_API_VERSION;

    // game module lifecycle and player fixed-step update
    bool (*Load)() = nullptr;
    void (*Update)(EntityID playerID, const PlayerInput *input, float dt) = nullptr;
    void (*UpdatePlayerCamera)(EntityID playerID, float dt) = nullptr;
    void (*Shutdown)() = nullptr;

    void (*OnPlayStart)() = nullptr;
    void (*OnPlayStop)() = nullptr;

    // player input and camera callbacks
    void (*OnMouseLook)(float deltaX, float deltaY) = nullptr;
    void (*OnJumpPressed)() = nullptr;

    // simulation callbacks. The game module routes these to the actors/systems that need them
    void (*BeginSimulationStep)() = nullptr;
    void (*OnCollision)(EntityID aID, EntityID bID, const GameContact *contact) = nullptr;

    // data/UI requested by the Runtime
    void (*GetCameraState)(GameCameraState *cameraState) = nullptr;
    void (*DrawSettingsUI)() = nullptr;
};

extern "C" const GameAPI *GetGameAPI();
