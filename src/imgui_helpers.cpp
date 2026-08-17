#include "headers/imgui_helpers.h"

void UpdateAnimationImGui() {

    ImGui::Begin("Animation");

    if (ImGui::BeginCombo("Clip", animationPlayer.current_animation >= 0 ? animations[animationPlayer.current_animation].name.c_str() : "None")) {

        for (int i = 0; i < static_cast<int>(animations.size()); ++i) {

            bool selected = animationPlayer.current_animation == i;

            if (ImGui::Selectable( animations[i].name.c_str(), selected)) {

                animationPlayer.current_animation = i;
                animationPlayer.time = 0.0f;
                animationPlayer.playing = false;

                EvaluateCurrentAnimation();
            }

            if (selected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    if (ImGui::Button("New")) {

        AnimationClip clip;

        clip.name = "Animation_" + std::to_string(animations.size());

        clip.duration = 1.0f;
        clip.loop = true;

        animations.push_back(std::move(clip));

        animationPlayer.current_animation = static_cast<int>(animations.size()) - 1;

        animationPlayer.time = 0.0f;
        animationPlayer.playing = false;
    }

    if (ImGui::Button(animationPlayer.playing ? "Pause" : "Play")) {

        animationPlayer.playing = !animationPlayer.playing;
    }

    ImGui::SameLine();

    if (ImGui::Button("Stop")) {

        animationPlayer.playing = false;
        animationPlayer.time = 0.0f;

        EvaluateCurrentAnimation();
    }

    AnimationClip* clip = nullptr;

    if (animationPlayer.current_animation >= 0 && animationPlayer.current_animation < static_cast<int>(animations.size())) {

        clip = &animations[animationPlayer.current_animation];
    }

    if (clip) {

        float duration = clip->duration;

        if (ImGui::DragFloat("Duration", &duration, 0.1f, 0.1f, 60.0f)) {

            clip->duration = duration;

            if (animationPlayer.time > clip->duration)
                animationPlayer.time = clip->duration;

            EvaluateCurrentAnimation();
        }

        ImGui::Text("Time: %.2f / %.2f", animationPlayer.time, clip->duration);

        float time = animationPlayer.time;

        if (ImGui::SliderFloat("##AnimationTime", &time, 0.0f, std::max(clip->duration, 0.01f), "%.2f s")) {

            animationPlayer.time = time;

            EvaluateCurrentAnimation();
        }
    }

    ImGui::End();
}

void DrawObjectAnimationButtons(Object& obj) {

    if (!ImGui::CollapsingHeader("Animation##object"))
        return;

    if (animationPlayer.current_animation < 0) {

        ImGui::TextDisabled("Create/select an animation first.");

        return;
    }

    AnimationClip& clip = animations[animationPlayer.current_animation];

    ImGui::Text("Time: %.2f", animationPlayer.time);

    if (ImGui::Button("Key Position")) {

        AnimationTrack& track = GetOrCreateTrack(clip, AnimationTargetType::Object, obj.id, AnimationProperty::Position);

        AddKey(track, animationPlayer.time, obj.position);

        RecalculateAnimationDuration(clip);
    }

    ImGui::SameLine();

    if (ImGui::Button("Key Rotation")) {

        AnimationTrack& track = GetOrCreateTrack(clip, AnimationTargetType::Object, obj.id, AnimationProperty::Rotation);

        AddKey(track, animationPlayer.time, obj.rotation);

        RecalculateAnimationDuration(clip);
    }

    ImGui::SameLine();

    if (ImGui::Button("Key Scale")) {

        AnimationTrack& track = GetOrCreateTrack(clip, AnimationTargetType::Object, obj.id, AnimationProperty::Scale);

        AddKey(track, animationPlayer.time, obj.scale);

        RecalculateAnimationDuration(clip);
    }

    if (ImGui::Button("Key Visible")) {

        AnimationTrack& track = GetOrCreateTrack(clip, AnimationTargetType::Object, obj.id, AnimationProperty::Visible);

        AddKey(track, animationPlayer.time, obj.visible);

        RecalculateAnimationDuration(clip);
    }

    if (ImGui::Button("Key Texture")) {

        AnimationTrack& track = GetOrCreateTrack(clip, AnimationTargetType::Object, obj.id, AnimationProperty::Texture);

        AddKey(track, animationPlayer.time, obj.tex );

        RecalculateAnimationDuration(clip);
    }
}

void DrawLightAnimationButtons(Light& light) {

    if (!ImGui::CollapsingHeader("Animation##light"))
        return;

    if (animationPlayer.current_animation < 0) {

        ImGui::TextDisabled("Create/select an animation first.");

        return;
    }

    AnimationClip& clip = animations[animationPlayer.current_animation];

    if (ImGui::Button("Key Color")) {

        auto& track = GetOrCreateTrack(clip, AnimationTargetType::Light, light.id, AnimationProperty::Color);

        AddKey(track, animationPlayer.time, light.col);
    }

    if (ImGui::Button("Key Ambient")) {

        auto& track = GetOrCreateTrack(clip, AnimationTargetType::Light, light.id, AnimationProperty::Ambient);

        AddKey(track, animationPlayer.time, light.ambient);
    }

    if (ImGui::Button("Key Attenuation")) {

        auto& track = GetOrCreateTrack(clip, AnimationTargetType::Light, light.id, AnimationProperty::Attenuation);

        AddKey(track, animationPlayer.time, light.att);
    }

    if (ImGui::Button("Key Inner")) {

        auto& track = GetOrCreateTrack(clip, AnimationTargetType::Light, light.id, AnimationProperty::Inner);

        AddKey(track, animationPlayer.time, light.inner);
    }

    if (ImGui::Button("Key Outer")) {

        auto& track = GetOrCreateTrack(clip, AnimationTargetType::Light, light.id, AnimationProperty::Outer);

        AddKey(track, animationPlayer.time, light.outer);
    }

    if (ImGui::Button("Key Falloff")) {

        auto& track = GetOrCreateTrack(clip, AnimationTargetType::Light, light.id, AnimationProperty::Falloff);

        AddKey(track, animationPlayer.time, light.falloff);
    }

    RecalculateAnimationDuration(clip);
}