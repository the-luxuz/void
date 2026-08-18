#include "headers/imgui_helpers.h"

bool canScroll = false;

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

        if (ImGui::IsWindowHovered())
            canScroll = true;
        else
            canScroll = false;

        DrawAnimationTimeline(*clip);

        /*float duration = clip->duration;

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
        }*/
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

void DrawAnimationTimeline(AnimationClip& clip) {

    float leftPanelWidth = 220.f;
    float rowHeight = 32.f;
    float rulerHeight = 28.f;

    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();

    if (canvasSize.x < leftPanelWidth + 100.0f)
        return;

    float timeLineStartx = canvasPos.x + leftPanelWidth;

    float timeLineWidth = canvasSize.x - leftPanelWidth;

    float rulerY = canvasPos.y;

    if (canScroll) {

        ImGuiIO& io = ImGui::GetIO();
    
        if (io.MouseWheel != 0.0f) {

            if (io.MouseWheel > 0.0f) 
                animationEditor.timelineViewMax = std::max(0.1f, animationEditor.timelineViewMax - 1.0f);
            else 
                animationEditor.timelineViewMax += 1.0f;
            
        }
    }

    //animationEditor.timelineViewMax = clip.duration;

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(ImVec2(canvasPos.x, rulerY), ImVec2(canvasPos.x + canvasSize.x, rulerY + rulerHeight), IM_COL32(40, 40, 40, 255));

    float maxtime = std::max(animationEditor.timelineViewMax, 0.01f);

    int tickCount = static_cast<int>(std::ceil(maxtime));

    // lines of the timeline

    for (int i = 0; i < tickCount; i++) {
    
        float time = static_cast<float>(i);

        if (time > maxtime)
            break;

        float x = TimelineTimeToX(time, timeLineStartx, timeLineWidth, maxtime);

        drawList->AddLine(ImVec2(x, rulerY), ImVec2(x, rulerY + rulerHeight), IM_COL32(100, 100, 100, 255));

        char text[32];

        snprintf(text, sizeof(text), "%.1f", time);

        drawList->AddText(ImVec2(x + 3.0f, rulerY + 4.0f), IM_COL32(200, 200, 200, 255), text);

    }

    float current_time = TimelineTimeToX(animationPlayer.time, timeLineStartx, timeLineWidth, maxtime);

    drawList->AddLine(ImVec2(current_time, rulerY), ImVec2(current_time, rulerY + rulerHeight), IM_COL32(255, 80, 80, 255), 2.5f);

    ImGui::Separator();

    // tracks (position, scale...)

    for (int i = 0; i < static_cast<int>(clip.tracks.size()); i++) {
    
        AnimationTrack& track = clip.tracks[i];

        float y = canvasPos.y + rulerHeight + (i * rowHeight);

        drawList->AddRectFilled(ImVec2(canvasPos.x, y), ImVec2(canvasPos.x + canvasSize.x, y + rowHeight), i % 2 == 0 ? IM_COL32(32, 32, 32, 255) : IM_COL32(38, 38, 38, 255));

        drawList->AddLine(ImVec2(canvasPos.x, y + rowHeight), ImVec2(canvasPos.x + canvasSize.x, y + rowHeight), IM_COL32(70, 70, 70, 255));

        std::string name = AnimationTrackTargetName(track) + " : " + AnimationPropertyName(track.property);

        drawList->AddText(ImVec2(canvasPos.x + 6.f, y + 8.f), IM_COL32(255, 255, 255, 255), name.c_str());

        drawList->AddLine(ImVec2(timeLineStartx, y + rowHeight * 0.5f), ImVec2(timeLineStartx + timeLineWidth, y + rowHeight * 0.5f), IM_COL32(70, 70, 70, 255));

        // keys

        for (int j = 0; j < static_cast<int>(track.keys.size()); j++) {
            
            AnimationKey& key = track.keys[j];

            float x = TimelineTimeToX(key.time, timeLineStartx, timeLineWidth, maxtime);

            float cy = y + rowHeight * 0.5f;

            const float radius = 6.0f;

            bool selected = animationEditor.selectedTrack == i && animationEditor.selectedKey == j;

            ImVec2 mouse = ImGui::GetMousePos();

            bool hovered = mouse.x >= x - radius - 3.0f && mouse.x <= x + radius + 3.0f && mouse.y >= cy - radius - 3.0f && mouse.y <= cy + radius + 3.0f;

            ImU32 keyColor = selected ? IM_COL32(255, 220, 70, 255) : IM_COL32(220, 180, 50, 255);

            if (hovered) {
            
                keyColor = IM_COL32(255, 240, 100, 255);
            }

            // diamond (chatgpteada, jej)

            ImVec2 p1(x, cy - radius);
            ImVec2 p2(x + radius, cy);
            ImVec2 p3(x, cy + radius);
            ImVec2 p4(x - radius, cy);

            drawList->AddQuadFilled(p1, p2, p3, p4, keyColor);

            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                
                animationEditor.selectedTrack = i;

                animationEditor.selectedKey = j;

                animationEditor.draggingKey = true;

                animationPlayer.playing = false;

                float mouse_time = TimelineXToTime(mouse.x, timeLineStartx, timeLineWidth, maxtime);

                animationEditor.dragOffsetTime = key.time - mouse_time;

            }

            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            
                track.keys.erase(track.keys.begin() + j);

                animationEditor.selectedTrack = -1;

                animationEditor.selectedKey = -1;

                break;
            }

        }

        std::string loopName = AnimationTrackTargetName(track) + ", Loop: " + AnimationPropertyName(track.property);

        ImGui::Checkbox(loopName.c_str(), &clip.loop);

    }

    // drag

    if (animationEditor.draggingKey) {
    
        int trackIndex = animationEditor.selectedTrack;

        int keyIndex = animationEditor.selectedKey;

        if (trackIndex >= 0 && trackIndex < static_cast<int>(clip.tracks.size())) {
        
            AnimationTrack& track = clip.tracks[trackIndex];

            if (keyIndex >= 0 && keyIndex < static_cast<int>(track.keys.size())) {

                AnimationKey& key = track.keys[keyIndex];

                ImVec2 mouse = ImGui::GetMousePos();
                    
                float mouse_time = TimelineXToTime(mouse.x, timeLineStartx, timeLineWidth, maxtime);

                float new_time = mouse_time + animationEditor.dragOffsetTime;

                if (animationEditor.animationSnapEnable)
                    new_time = Snap(new_time, animationEditor.animationSnap);

                key.time = glm::clamp(new_time, 0.f, maxtime);

            }

        }
    }

    // release

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    
        animationEditor.draggingKey = false;

        int trackIndex = animationEditor.selectedTrack;

        if (trackIndex >= 0 && trackIndex < static_cast<int>(clip.tracks.size())) {
        
            auto& keys = clip.tracks[trackIndex].keys;

            std::sort(keys.begin(), keys.end(), [](const AnimationKey& a, const AnimationKey& b) {

                return a.time < b.time; 

                }
            );

            RecalculateAnimationDuration(clip);
        }

    }

    // click empty

    ImVec2 mouse = ImGui::GetMousePos();

    bool insideTimeline = mouse.x >= timeLineStartx && mouse.x <= timeLineStartx + timeLineWidth && mouse.y >= canvasPos.y && mouse.y <= canvasPos.y + rulerHeight + clip.tracks.size() * rowHeight;

    if (insideTimeline && !animationEditor.draggingKey && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    
        animationPlayer.time = TimelineXToTime(mouse.x, timeLineStartx, timeLineWidth, maxtime);

        animationPlayer.time = glm::clamp(animationPlayer.time, 0.f, maxtime);

        EvaluateCurrentAnimation();

    }

}