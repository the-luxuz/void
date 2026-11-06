#include "actors/player.h"

static bool LoadGame() {

    PlayerActor::ResetJumpState();
    PlayerActor::cameraInitialized = false;

    return true;
}

static void ShutdownGame() {

    PlayerActor::ResetJumpState();
    PlayerActor::cameraInitialized = false;
}

static void StartPlay() {

    PlayerActor::ResetJumpState();
    PlayerActor::cameraInitialized = false;
}

static void StopPlay() {

    PlayerActor::ResetJumpState();
    PlayerActor::cameraInitialized = false;
}

static void UpdatePlayer(EntityID playerID, const PlayerInput *input, float dt) {

    if (!input)
        return;

    PlayerActor::Control(playerID, *input, dt);
}

static void UpdatePlayerCamera(EntityID playerID, float dt) {

    PlayerActor::UpdateCamera(playerID, dt);
}

static void OnMouseLook(float deltaX, float deltaY) {

    constexpr float maxPitch = glm::radians(89.0f);

    PlayerActor::cameraState.alpha += deltaX * PlayerActor::lookSensitivity;
    PlayerActor::cameraState.beta -= deltaY * PlayerActor::lookSensitivity;
    PlayerActor::cameraState.beta = glm::clamp(PlayerActor::cameraState.beta, -maxPitch, maxPitch);
}

static void OnCollision(EntityID aID, EntityID bID, const GameContact *contact) {

    if (!contact)
        return;

    Contact engineContact{};

    engineContact.hit = contact->hit;
    engineContact.normal = glm::vec3(contact->normal[0], contact->normal[1], contact->normal[2]);
    engineContact.penetration = contact->penetration;
    engineContact.point = glm::vec3(contact->point[0], contact->point[1], contact->point[2]);

   PlayerActor::RegisterCollision(aID, bID, engineContact);
}

static void GetCameraState(GameCameraState *state) {

    if (!state)
        return;

    state->position[0] = PlayerActor::cameraState.cam_position.x;
    state->position[1] = PlayerActor::cameraState.cam_position.y;
    state->position[2] = PlayerActor::cameraState.cam_position.z;

    state->target[0] = PlayerActor::cameraState.cam_target.x;
    state->target[1] = PlayerActor::cameraState.cam_target.y;
    state->target[2] = PlayerActor::cameraState.cam_target.z;

    state->up[0] = PlayerActor::cameraState.cam_up.x;
    state->up[1] = PlayerActor::cameraState.cam_up.y;
    state->up[2] = PlayerActor::cameraState.cam_up.z;

    state->speedFovOffset = PlayerActor::speedFovOffset;
}

static void DrawSettingsUI() {

    ImGui::Separator();
    ImGui::TextUnformatted("Player Camera");
    ImGui::SliderFloat("Eye height", &PlayerActor::cameraEyeHeight, 0.5f, 3.0f, "%.2f");
    ImGui::SliderFloat("Follow response", &PlayerActor::cameraFollowSpeed, 1.0f, 30.0f, "%.1f");
    ImGui::SliderFloat("Look sensitivity", &PlayerActor::lookSensitivity, 0.0001f, 0.003f, "%.4f");

    ImGui::Separator();
    ImGui::TextUnformatted("Player Movement");
    ImGui::SliderFloat("Default max speed", &PlayerActor::defaultMaximumSpeed, 100.0f, 10000.0f, "%.0f");
    ImGui::SliderFloat("Air input control", &PlayerActor::airControlStrength, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Slide crouch scale", &PlayerActor::slideCrouchScale, 0.4f, 1.0f, "%.2f");
    ImGui::SliderFloat("Crouch transition", &PlayerActor::crouchTransitionDuration, 0.05f, 0.5f, "%.2f s");
    ImGui::SliderFloat("Crouched speed multiplier", &PlayerActor::crouchedMoveSpeedMultiplier, 0.2f, 1.0f, "%.2f x");
    ImGui::SliderFloat("Air slide steering", &PlayerActor::slideAirTurnDegreesPerSecond, 0.0f, 540.0f, "%.0f deg/s");
    ImGui::SliderFloat("Landing inertia decay", &PlayerActor::slideCarryLandingDecayDuration, 0.1f, 0.8f, "%.2f s");
    ImGui::SliderFloat("Slide release jump window", &PlayerActor::slideJumpReleaseWindowDuration, 0.0f, 0.3f, "%.2f s");
    ImGui::SliderFloat("Speed FOV increase", &PlayerActor::speedFovIncrease, 0.0f, 12.0f, "%.1f°");
    ImGui::SliderFloat("FOV reference speed", &PlayerActor::speedFovReferenceMultiplier, 0.5f, 4.0f, "%.2f x normal");
    ImGui::SliderFloat("Jump height", &PlayerActor::jumpHeight, 20.0f, 600.0f, "%.0f");
    ImGui::SliderFloat("Air gravity multiplier", &PlayerActor::jumpGravityMultiplier, 1.0f, 5.0f, "%.1f x");
}

extern "C" const GameAPI* GetGameAPI() {

    static const GameAPI gameAPI{

        GAME_API_VERSION,
        &LoadGame,
        &UpdatePlayer,
        &UpdatePlayerCamera,
        &ShutdownGame,
        &StartPlay,
        &StopPlay,
        &OnMouseLook,
        &PlayerActor::QueueJumpInput,
        &PlayerActor::BeginCollisionStep,
        &OnCollision,
        &GetCameraState,
        &DrawSettingsUI,
    };

    return &gameAPI;
}
