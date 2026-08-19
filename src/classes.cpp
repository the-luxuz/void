#include "headers/classes.h"

//--------------------------------------------------------------
// animations

std::vector<AnimationClip> animations;
AnimationPlayer animationPlayer;
AnimationEditorState animationEditor;

VideoPlayer videoPlayer;
bool isCinematicActive = false;

EventManager eventManager;

EntityID GenerateEntityID() {

    return nextEntityID++;
}

float InterpolateFloat(const AnimationKey& a, const AnimationKey& b, float time) {

    float t = (time - a.time) / (b.time - a.time);

    t = glm::clamp(t, 0.0f, 1.f);

    float va = std::get<float>(a.value);
    float vb = std::get<float>(b.value);

    return glm::mix(va, vb, t);
}

glm::vec3 InterpolateVec3(const AnimationKey& a, const AnimationKey& b, float time) {

    float t = (time - a.time) / (b.time - a.time);

    t = glm::clamp(t, 0.0f, 1.f);

    glm::vec3 va = std::get<glm::vec3>(a.value);
    glm::vec3 vb = std::get<glm::vec3>(b.value);

    return glm::mix(va, vb, t);
}

glm::quat InterpolateQuat(const AnimationKey& a, const AnimationKey& b, float time) {
    
    float t = (time - a.time) / (b.time - a.time);

    t = glm::clamp(t, 0.0f, 1.f);

    glm::quat va = std::get<glm::quat>(a.value);
    glm::quat vb = std::get<glm::quat>(b.value);

    return glm::slerp(va, vb, t);
}

const AnimationKey* GetPrevKey(const AnimationTrack& track, float time) {

    if (track.keys.empty())
        return nullptr;

    const AnimationKey* result = nullptr;

    for (const auto& track : track.keys) {
    
        if (track.time <= time)
            result = &track;
        else
            break;
    }

    return result;
}

const AnimationKey* GetNextKey(const AnimationTrack& track, float time) {

    for (const auto& track : track.keys) {

        if (track.time >= time)
            return &track;
    }

    return nullptr;
}

AnimationValue EvaluateTrack(const AnimationTrack& track, float time) {

    if (track.keys.empty())
        return {};

    if (time <= track.keys.front().time)
        return track.keys.front().value;

    if (time >= track.keys.back().time)
        return track.keys.back().value;

    const AnimationKey* prev = nullptr;
    const AnimationKey* next = nullptr;

    for (size_t i = 0; i + 1 < track.keys.size(); ++i) {

        if (time >= track.keys[i].time && time <= track.keys[i + 1].time) {

            prev = &track.keys[i];
            next = &track.keys[i + 1];

            break;
        }
    }

    if (!prev || !next)
        return track.keys.back().value;

    if (std::holds_alternative<float>(prev->value))
        return InterpolateFloat(*prev, *next, time);

    if (std::holds_alternative<glm::vec3>(prev->value))
        return InterpolateVec3(*prev, *next, time);

    if (std::holds_alternative<glm::quat>(prev->value))
        return InterpolateQuat(*prev, *next, time);

    return prev->value;
}

AnimationTrack* FindTrack(AnimationClip& clip, AnimationTargetType targetType, EntityID targetID, AnimationProperty property) {

    for (auto& track : clip.tracks) {

        if (track.targetType == targetType && track.targetID == targetID && track.property == property) {

            return &track;
        }
    }

    return nullptr;
}

AnimationTrack& GetOrCreateTrack(AnimationClip& clip, AnimationTargetType targetType, EntityID targetID, AnimationProperty property) {

    if (AnimationTrack* track = FindTrack(clip, targetType, targetID, property)) {

        return *track;
    }

    clip.tracks.push_back({ targetType, targetID, property, {} });

    return clip.tracks.back();
}

void RecalculateAnimationDuration(AnimationClip& clip) {

    float maxKeyTime = 0.0f;

    for (const auto& track : clip.tracks) {

        if (!track.keys.empty())
            maxKeyTime = std::max(maxKeyTime, track.keys.back().time);
    }

        clip.duration = maxKeyTime;
}

const char* AnimationPropertyName(AnimationProperty property) {

    switch (property) {

        case AnimationProperty::Position:

            return "Position";

            break;

        case AnimationProperty::Rotation:

            return "Rotation";

            break;

        case AnimationProperty::Scale:

            return "Scale";

            break;

        case AnimationProperty::Color:

            return "Color";

            break;

        case AnimationProperty::Attenuation:

            return "Attenuation";

            break;

        case AnimationProperty::Ambient:

            return "Ambient";

            break;

        case AnimationProperty::Inner:

            return "Inner";

            break;

        case AnimationProperty::Outer:

            return "Outer";

            break;

        case AnimationProperty::Falloff:

            return "Falloff";

            break;

        case AnimationProperty::Visible:

            return "Visible";

            break;

        case AnimationProperty::Texture:

            return "Texture";

            break;
    }

    return "null";

}

float TimelineTimeToX(float time, float startX, float pixelsPerSecond, float scrollTime) {

    return startX +
        (time - scrollTime) *
        pixelsPerSecond;
}

float TimelineXToTime(float x, float startX, float pixelsPerSecond, float scrollTime) {

    return scrollTime +
        (x - startX) /
        pixelsPerSecond;
}

//--------------------------------------------------------------
// events

int FindAnimations(std::string name, EntityID objId) {

    for (int i = 0; i < static_cast<int>(animations.size()); i++) {

        AnimationClip& clip = animations[i];

        if (clip.name != name)
            continue;

        for (const AnimationTrack& track : clip.tracks) {

            if (track.targetID == objId) {

                return i;
            }
        }
    }

    return -1;
}