#include "headers/imgui_helpers.h"

bool canScroll = false;

void UpdateAnimationImGui() {

    ImGui::Begin("Animation");

    if (ImGui::BeginCombo("Clip", animationPreviewPlayer.current_animation >= 0 ? animations[animationPreviewPlayer.current_animation].name.c_str() : "None")) {

        for (int i = 0; i < static_cast<int>(animations.size()); ++i) {

            bool selected = animationPreviewPlayer.current_animation == i;

            if (ImGui::Selectable( animations[i].name.c_str(), selected)) {

                animationPreviewPlayer.current_animation = i;
                animationPreviewPlayer.time = 0.0f;
                animationPreviewPlayer.playing = false;

                EvaluateCurrentAnimation();
            }

            if (selected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    static std::string anim_name = "";
    static char name[256];

    strncpy_s(name, anim_name.c_str(), sizeof(name));

    if (ImGui::InputText("Anim Name", name, sizeof(name))) {

        anim_name = name;
    }

    if (ImGui::Button("New")) {

        AnimationClip clip;

        clip.name = anim_name;

        clip.duration = 1.0f;
        clip.loop = true;

        animations.push_back(std::move(clip));

        animationPreviewPlayer.current_animation = static_cast<int>(animations.size()) - 1;

        animationPreviewPlayer.time = 0.0f;
        animationPreviewPlayer.playing = false;
    }

    if (ImGui::Button(animationPreviewPlayer.playing ? "Pause" : "Play")) {

        animationPreviewPlayer.playing = !animationPreviewPlayer.playing;
    }

    ImGui::SameLine();

    if (ImGui::Button("Stop")) {

        animationPreviewPlayer.playing = false;
        animationPreviewPlayer.time = 0.0f;

        EvaluateCurrentAnimation();
    }

    if (ImGui::Button("Play All")) {
        PlayAllAnims();
    }

    ImGui::SameLine();

    if (ImGui::Button("Stop All")) {
        StopAllAnims();
    }

    AnimationClip* clip = nullptr;

    if (animationPreviewPlayer.current_animation >= 0 && animationPreviewPlayer.current_animation < static_cast<int>(animations.size())) {

        clip = &animations[animationPreviewPlayer.current_animation];
    }

    if (clip) {

        int clipIndex = animationPreviewPlayer.current_animation;

        std::string animationRef = "AnimationRefs::" + AnimationReferenceIdentifier(clipIndex);

        ImGui::Separator();

        ImGui::TextDisabled("C++ reference (updated when you save the scene):");

        ImGui::TextWrapped("eventManager.Throw(std::make_unique<PlayAnimationEvent>(%s));", animationRef.c_str());

        if (ImGui::SmallButton("Copy C++ reference")) {

            std::string code = "\neventManager.Throw(std::make_unique<PlayAnimationEvent>(" + animationRef + "));";

            SDL_SetClipboardText(code.c_str());
        }

        ImGui::Checkbox("Loop", &clip->loop);

        if (ImGui::IsWindowHovered())
            canScroll = true;
        else
            canScroll = false;

        DrawAnimationTimeline(*clip);
    }

    ImGui::End();
}

void DrawObjectAnimationButtons(Object& obj) {

    if (!ImGui::CollapsingHeader("Animation##object"))
        return;

    if (animationPreviewPlayer.current_animation < 0) {

        ImGui::TextDisabled("Create/select an animation first.");

        return;
    }

    AnimationClip& clip = animations[animationPreviewPlayer.current_animation];

    ImGui::Text("Time: %.2f", animationPreviewPlayer.time);

    if (ImGui::Button("Key Position")) {

        AnimationTrack& track = GetOrCreateTrack(clip, AnimationTargetType::Object, obj.id, AnimationProperty::Position);

        AddKey(track, animationPreviewPlayer.time, obj.position);

        RecalculateAnimationDuration(clip);
    }

    ImGui::SameLine();

    if (ImGui::Button("Key Rotation")) {

        AnimationTrack& track = GetOrCreateTrack(clip, AnimationTargetType::Object, obj.id, AnimationProperty::Rotation);

        AddKey(track, animationPreviewPlayer.time, obj.rotation);

        RecalculateAnimationDuration(clip);
    }

    ImGui::SameLine();

    if (ImGui::Button("Key Scale")) {

        AnimationTrack& track = GetOrCreateTrack(clip, AnimationTargetType::Object, obj.id, AnimationProperty::Scale);

        AddKey(track, animationPreviewPlayer.time, obj.scale);

        RecalculateAnimationDuration(clip);
    }

    if (ImGui::Button("Key Visible")) {

        AnimationTrack& track = GetOrCreateTrack(clip, AnimationTargetType::Object, obj.id, AnimationProperty::Visible);

        AddKey(track, animationPreviewPlayer.time, obj.visible);

        RecalculateAnimationDuration(clip);
    }

    if (ImGui::Button("Key Texture")) {

        AnimationTrack& track = GetOrCreateTrack(clip, AnimationTargetType::Object, obj.id, AnimationProperty::Texture);

        AddKey(track, animationPreviewPlayer.time, obj.tex );

        RecalculateAnimationDuration(clip);
    }
}

void DrawLightAnimationButtons(Light& light) {

    if (!ImGui::CollapsingHeader("Animation##light"))
        return;

    if (animationPreviewPlayer.current_animation < 0) {

        ImGui::TextDisabled("Create/select an animation first.");

        return;
    }

    AnimationClip& clip = animations[animationPreviewPlayer.current_animation];

    if (ImGui::Button("Key Color")) {

        auto& track = GetOrCreateTrack(clip, AnimationTargetType::Light, light.id, AnimationProperty::Color);

        AddKey(track, animationPreviewPlayer.time, light.col);
    }

    if (ImGui::Button("Key Ambient")) {

        auto& track = GetOrCreateTrack(clip, AnimationTargetType::Light, light.id, AnimationProperty::Ambient);

        AddKey(track, animationPreviewPlayer.time, light.ambient);
    }

    if (ImGui::Button("Key Attenuation")) {

        auto& track = GetOrCreateTrack(clip, AnimationTargetType::Light, light.id, AnimationProperty::Attenuation);

        AddKey(track, animationPreviewPlayer.time, light.att);
    }

    if (ImGui::Button("Key Inner")) {

        auto& track = GetOrCreateTrack(clip, AnimationTargetType::Light, light.id, AnimationProperty::Inner);

        AddKey(track, animationPreviewPlayer.time, light.inner);
    }

    if (ImGui::Button("Key Outer")) {

        auto& track = GetOrCreateTrack(clip, AnimationTargetType::Light, light.id, AnimationProperty::Outer);

        AddKey(track, animationPreviewPlayer.time, light.outer);
    }

    if (ImGui::Button("Key Falloff")) {

        auto& track = GetOrCreateTrack(clip, AnimationTargetType::Light, light.id, AnimationProperty::Falloff);

        AddKey(track, animationPreviewPlayer.time, light.falloff);
    }

    RecalculateAnimationDuration(clip);
}

void DrawAnimationTimeline(AnimationClip& clip) {

    const float leftPanelWidth = 220.0f;
    const float rowHeight = 32.0f;
    const float rulerHeight = 28.0f;

    bool animationWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);

    ImVec2 available = ImGui::GetContentRegionAvail();

    if (available.x < leftPanelWidth + 100.0f)
        return;

    float timelineWidth = available.x - leftPanelWidth;
    float tracksHeight = std::max(100.0f, available.y - rulerHeight);

    ImVec2 startPos = ImGui::GetCursorScreenPos();
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mouse = ImGui::GetMousePos();
    ImDrawList* parentDrawList = ImGui::GetWindowDrawList();

    float pixelsPerSecond = animationEditor.timelinePixelsPerSecond;
    float scrollTime = animationEditor.timelineScrollTime;

    bool mouseOverTimeline = mouse.x >= startPos.x + leftPanelWidth && mouse.x <= startPos.x + available.x && mouse.y >= startPos.y && mouse.y <= startPos.y + rulerHeight + tracksHeight;

    if (animationWindowHovered && canScroll && mouseOverTimeline && io.MouseWheel != 0.0f) {

        float localMouseX = mouse.x - (startPos.x + leftPanelWidth);
        float mouseTime = scrollTime + localMouseX / pixelsPerSecond;
        float zoomFactor = io.MouseWheel > 0.0f ? 1.15f : 1.0f / 1.15f;

        float newPixelsPerSecond = glm::clamp(pixelsPerSecond * zoomFactor, 10.0f, 2000.0f);
        float newScrollTime = std::max(0.0f, mouseTime - localMouseX / newPixelsPerSecond);

        animationEditor.timelinePixelsPerSecond = newPixelsPerSecond;
        animationEditor.timelineScrollTime = newScrollTime;

        pixelsPerSecond = newPixelsPerSecond;
        scrollTime = newScrollTime;
    }

    ImGui::BeginChild(
        "AnimationTrackNames",
        ImVec2(leftPanelWidth, tracksHeight),
        true
    );

    float scrollY = ImGui::GetScrollY();
    animationEditor.timelineTrackScrollY = scrollY;

    ImDrawList* leftDrawList = ImGui::GetWindowDrawList();
    ImVec2 leftPos = ImGui::GetWindowPos();

    float leftPadding = ImGui::GetStyle().WindowPadding.x;
    float leftTop = leftPos.y + ImGui::GetStyle().WindowPadding.y;
    float leftX = leftPos.x + leftPadding;

    float totalTrackHeight = rulerHeight + clip.tracks.size() * rowHeight;

    ImGui::Dummy(ImVec2(leftPanelWidth, totalTrackHeight));

    leftDrawList->AddRectFilled(
        ImVec2(leftX, leftTop),
        ImVec2(leftX + leftPanelWidth, leftTop + rulerHeight),
        IM_COL32(40, 40, 40, 255)
    );

    for (int i = 0; i < static_cast<int>(clip.tracks.size()); ++i) {

        AnimationTrack& track = clip.tracks[i];

        float y = leftTop + rulerHeight + i * rowHeight - scrollY;

        if (y + rowHeight < leftTop + rulerHeight)
            continue;

        if (y > leftTop + tracksHeight)
            continue;

        leftDrawList->AddRectFilled(
            ImVec2(leftX, y),
            ImVec2(leftX + leftPanelWidth, y + rowHeight),
            i % 2 == 0 ? IM_COL32(32, 32, 32, 255) : IM_COL32(38, 38, 38, 255)
        );

        leftDrawList->AddLine(
            ImVec2(leftX, y + rowHeight),
            ImVec2(leftX + leftPanelWidth, y + rowHeight),
            IM_COL32(70, 70, 70, 255)
        );

        std::string name = AnimationTrackTargetName(track) + " : " + AnimationPropertyName(track.property);

        leftDrawList->AddText(
            ImVec2(leftX + 6.0f, y + 8.0f),
            IM_COL32(255, 255, 255, 255),
            name.c_str()
        );
    }

    ImGui::EndChild();

    ImGui::SameLine(0.0f, 0.0f);

    ImGui::BeginChild(
        "AnimationTimelineChild",
        ImVec2(timelineWidth, tracksHeight),
        true,
        ImGuiWindowFlags_HorizontalScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse
    );

    ImVec2 timelineWindowPos = ImGui::GetWindowPos();

    float windowPaddingX = ImGui::GetStyle().WindowPadding.x;
    float windowPaddingY = ImGui::GetStyle().WindowPadding.y;

    float timelineX = timelineWindowPos.x + windowPaddingX;
    float timelineY = timelineWindowPos.y + windowPaddingY;

    float visibleTime = timelineWidth / pixelsPerSecond;

    float maxTrackTime = clip.duration;

    for (const AnimationTrack& track : clip.tracks) {
        for (const AnimationKey& key : track.keys) {
            maxTrackTime = std::max(maxTrackTime, key.time);
        }
    }

    float minimumTimelineTime = 120.0f;
    float requiredTime = std::max(minimumTimelineTime, maxTrackTime + 60.0f);
    float visibleRequiredTime = scrollTime + visibleTime + 60.0f;

    if (visibleRequiredTime > requiredTime)
        requiredTime = visibleRequiredTime;

    float virtualWidth = std::max(timelineWidth, requiredTime * pixelsPerSecond);
    float virtualHeight = std::max(tracksHeight, rulerHeight + clip.tracks.size() * rowHeight);

    ImGui::Dummy(
        ImVec2(
            virtualWidth,
            virtualHeight
        )
    );

    ImGui::SetScrollY(scrollY);

    float nativeScrollX = ImGui::GetScrollX();
    float newScrollTime = nativeScrollX / pixelsPerSecond;

    if (std::abs(newScrollTime - scrollTime) > 0.0001f) {

        scrollTime = newScrollTime;
        animationEditor.timelineScrollTime = scrollTime;
    }

    float visibleStartTime = scrollTime;
    float visibleEndTime = scrollTime + visibleTime;

    int firstTick = static_cast<int>(std::floor(visibleStartTime));
    int lastTick = static_cast<int>(std::ceil(visibleEndTime));

    float rulerX = startPos.x + leftPanelWidth;
    float rulerY = startPos.y;

    parentDrawList->AddRectFilled(
        ImVec2(rulerX, rulerY),
        ImVec2(rulerX + timelineWidth, rulerY + rulerHeight),
        IM_COL32(40, 40, 40, 255)
    );

    for (int i = firstTick; i <= lastTick; ++i) {

        float time = static_cast<float>(i);
        float x = TimelineTimeToX(time, rulerX, pixelsPerSecond, scrollTime);

        if (x < rulerX || x > rulerX + timelineWidth)
            continue;

        parentDrawList->AddLine(
            ImVec2(x, rulerY),
            ImVec2(x, rulerY + rulerHeight),
            IM_COL32(100, 100, 100, 255)
        );

        char text[32];
        snprintf(text, sizeof(text), "%.1f", time);

        parentDrawList->AddText(
            ImVec2(x + 3.0f, rulerY + 4.0f),
            IM_COL32(200, 200, 200, 255),
            text
        );
    }

    float playheadX = TimelineTimeToX(
        animationPreviewPlayer.time,
        rulerX,
        pixelsPerSecond,
        scrollTime
    );

    if (playheadX >= rulerX && playheadX <= rulerX + timelineWidth) {

        parentDrawList->AddLine(
            ImVec2(playheadX, rulerY),
            ImVec2(playheadX, rulerY + rulerHeight),
            IM_COL32(255, 80, 80, 255),
            2.5f
        );
    }

    ImDrawList* timelineDrawList = ImGui::GetWindowDrawList();

    for (int i = 0; i < static_cast<int>(clip.tracks.size()); ++i) {

        AnimationTrack& track = clip.tracks[i];

        float y = timelineY + rulerHeight + i * rowHeight - scrollY;

        if (y + rowHeight < timelineY + rulerHeight)
            continue;

        if (y > timelineY + tracksHeight)
            continue;

        timelineDrawList->AddRectFilled(
            ImVec2(timelineX, y),
            ImVec2(timelineX + virtualWidth, y + rowHeight),
            i % 2 == 0 ? IM_COL32(32, 32, 32, 255) : IM_COL32(38, 38, 38, 255)
        );

        timelineDrawList->AddLine(
            ImVec2(timelineX, y + rowHeight),
            ImVec2(timelineX + virtualWidth, y + rowHeight),
            IM_COL32(70, 70, 70, 255)
        );

        timelineDrawList->AddLine(
            ImVec2(timelineX, y + rowHeight * 0.5f),
            ImVec2(timelineX + virtualWidth, y + rowHeight * 0.5f),
            IM_COL32(70, 70, 70, 255)
        );

        for (int j = 0; j < static_cast<int>(track.keys.size()); ++j) {

            AnimationKey& key = track.keys[j];

            float x = TimelineTimeToX(
                key.time,
                timelineX,
                pixelsPerSecond,
                scrollTime
            );

            float cy = y + rowHeight * 0.5f;
            const float radius = 6.0f;

            ImVec2 mousePos = ImGui::GetMousePos();

            bool selected = animationEditor.selectedTrack == i && animationEditor.selectedKey == j;

            bool hovered =
                mousePos.x >= x - radius - 3.0f &&
                mousePos.x <= x + radius + 3.0f &&
                mousePos.y >= cy - radius - 3.0f &&
                mousePos.y <= cy + radius + 3.0f;

            ImU32 keyColor = selected ? IM_COL32(255, 220, 70, 255) : IM_COL32(220, 180, 50, 255);

            if (hovered)
                keyColor = IM_COL32(255, 240, 100, 255);

            timelineDrawList->AddQuadFilled(
                ImVec2(x, cy - radius),
                ImVec2(x + radius, cy),
                ImVec2(x, cy + radius),
                ImVec2(x - radius, cy),
                keyColor
            );

            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {

                animationEditor.selectedTrack = i;
                animationEditor.selectedKey = j;
                animationEditor.draggingKey = true;
                animationPreviewPlayer.playing = false;

                float mouseTime = TimelineXToTime(
                    mousePos.x,
                    timelineX,
                    pixelsPerSecond,
                    scrollTime
                );

                animationEditor.dragOffsetTime = key.time - mouseTime;
            }

            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {

                track.keys.erase(track.keys.begin() + j);

                animationEditor.selectedTrack = -1;
                animationEditor.selectedKey = -1;
                animationEditor.draggingKey = false;

                if (track.keys.empty()) {

                    clip.tracks.erase(clip.tracks.begin() + i);

                    RecalculateAnimationDuration(clip);

                    ImGui::EndChild();

                    return;
                }

                RecalculateAnimationDuration(clip);

                break;
            }
        }
    }

    if (animationWindowHovered && animationEditor.draggingKey) {

        int trackIndex = animationEditor.selectedTrack;
        int keyIndex = animationEditor.selectedKey;

        if (trackIndex >= 0 && trackIndex < static_cast<int>(clip.tracks.size())) {

            AnimationTrack& track = clip.tracks[trackIndex];

            if (keyIndex >= 0 && keyIndex < static_cast<int>(track.keys.size())) {

                AnimationKey& key = track.keys[keyIndex];

                ImVec2 mousePos = ImGui::GetMousePos();

                float mouseTime = TimelineXToTime(
                    mousePos.x,
                    timelineX,
                    pixelsPerSecond,
                    scrollTime
                );

                float newTime = mouseTime + animationEditor.dragOffsetTime;

                if (animationEditor.animationSnapEnable)
                    newTime = Snap(newTime, animationEditor.animationSnap);

                key.time = glm::clamp(newTime, 0.0f, requiredTime);
            }
        }
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {

        animationEditor.draggingKey = false;

        int trackIndex = animationEditor.selectedTrack;

        if (trackIndex >= 0 && trackIndex < static_cast<int>(clip.tracks.size())) {

            auto& keys = clip.tracks[trackIndex].keys;

            std::sort(
                keys.begin(),
                keys.end(),
                [](const AnimationKey& a, const AnimationKey& b) {
                    return a.time < b.time;
                }
            );

            RecalculateAnimationDuration(clip);
        }
    }

    mouse = ImGui::GetMousePos();

    bool mouseInsideRuler =
        mouse.x >= rulerX &&
        mouse.x <= rulerX + timelineWidth &&
        mouse.y >= rulerY &&
        mouse.y <= rulerY + rulerHeight;

    bool mouseInsideTracks =
        mouse.x >= timelineX &&
        mouse.x <= timelineX + timelineWidth &&
        mouse.y >= timelineY + rulerHeight &&
        mouse.y <= timelineY + tracksHeight;

    if (animationWindowHovered && (mouseInsideRuler || mouseInsideTracks) &&
        !animationEditor.draggingKey &&
        ImGui::IsMouseDown(ImGuiMouseButton_Left)) {

        float mouseTime = TimelineXToTime(
            mouse.x,
            rulerX,
            pixelsPerSecond,
            scrollTime
        );

        if (animationEditor.animationSnapEnable)
            mouseTime = Snap(mouseTime, animationEditor.animationSnap);

        animationPreviewPlayer.time = glm::clamp(mouseTime, 0.0f, requiredTime);

        animationPreviewPlayer.playing = false;

        EvaluateCurrentAnimation();
    }

    ImGui::EndChild();
}
