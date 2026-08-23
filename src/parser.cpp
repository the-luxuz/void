#include "parser.h"


void Parser::WriteAnimationValue(std::ofstream& out, const AnimationValue& value) {

    uint8_t type = static_cast<uint8_t>(value.index());

    Write(out, type);

    switch (value.index()) {

    case 0: // float
        Write(out, std::get<float>(value));
        break;

    case 1: // int
        Write(out, std::get<int>(value));
        break;

    case 2: // bool
        Write(out, std::get<bool>(value));
        break;

    case 3: // vec3
        Write(out, std::get<glm::vec3>(value));
        break;

    case 4: // quat
        Write(out, std::get<glm::quat>(value));
        break;

    case 5: // string
        WriteString(out, std::get<std::string>(value));
        break;
    }
}

void Parser::ReadAnimationValue(std::ifstream& in, AnimationValue& value) {

    uint8_t type;

    Read(in, type);

    switch (type) {

    case 0: {
        float v;
        Read(in, v);
        value = v;
        break;
    }

    case 1: {
        int v;
        Read(in, v);
        value = v;
        break;
    }

    case 2: {
        bool v;
        Read(in, v);
        value = v;
        break;
    }

    case 3: {
        glm::vec3 v;
        Read(in, v);
        value = v;
        break;
    }

    case 4: {
        glm::quat v;
        Read(in, v);
        value = v;
        break;
    }

    case 5: {
        std::string v;
        ReadString(in, v);
        value = v;
        break;
    }

    default:
        in.setstate(std::ios::failbit);
        break;
    }
}

void Parser::WriteAnimationKey(std::ofstream& out, const AnimationKey& key) {

    Write(out, key.time);

    WriteAnimationValue(out, key.value);
}

void Parser::ReadAnimationKey(std::ifstream& in, AnimationKey& key) {

    Read(in, key.time);

    ReadAnimationValue(in, key.value);
}

void Parser::WriteAnimationTrack(std::ofstream& out, const AnimationTrack& track) {

    Write(out, track.targetType);
    Write(out, track.targetID);
    Write(out, track.property);

    uint32_t keyCount =
        static_cast<uint32_t>(track.keys.size());

    Write(out, keyCount);

    for (const auto& key : track.keys)
        WriteAnimationKey(out, key);
}

void Parser::ReadAnimationTrack(std::ifstream& in, AnimationTrack& track) {

    Read(in, track.targetType);
    Read(in, track.targetID);
    Read(in, track.property);

    uint32_t keyCount;

    Read(in, keyCount);

    track.keys.resize(keyCount);

    for (auto& key : track.keys)
        ReadAnimationKey(in, key);
}

void Parser::WriteAnimationClip(std::ofstream& out, const AnimationClip& clip) {

    WriteString(out, clip.name);

    Write(out, clip.duration);
    Write(out, clip.loop);

    uint32_t trackCount =
        static_cast<uint32_t>(clip.tracks.size());

    Write(out, trackCount);

    for (const auto& track : clip.tracks)
        WriteAnimationTrack(out, track);
}

void Parser::ReadAnimationClip(std::ifstream& in, AnimationClip& clip) {

    ReadString(in, clip.name);

    Read(in, clip.duration);
    Read(in, clip.loop);

    uint32_t trackCount;

    Read(in, trackCount);

    clip.tracks.resize(trackCount);

    for (auto& track : clip.tracks)
        ReadAnimationTrack(in, track);
}

void Parser::WriteString(std::ofstream& out, const std::string& str) {

    uint32_t len = static_cast<uint32_t>(str.size());

    out.write(reinterpret_cast<const char*>(&len), sizeof(len));
    out.write(str.data(), len);
}

constexpr uint32_t MAX_STRING_SIZE = 1024 * 1024;

void Parser::ReadString(std::ifstream& in, std::string& str) {

    uint32_t len;

    Read(in, len);

    if (!in || len > MAX_STRING_SIZE) {

        in.setstate(std::ios::failbit);
        return;
    }

    str.resize(len);

    if (len > 0)
        in.read(str.data(), len);
}

void Parser::save(const char* settings_name, const char* scene_name, Settings& st, camera& cam, const std::vector<Object>& scene, const std::vector<Object>& scene_lgh_obj, const std::vector<::Light>& scene_lgh) {

    std::ofstream out(settings_name, std::ios::binary);
    std::ofstream out_scene(scene_name, std::ios::binary);

    if (!out || !out_scene)
        return;

    save_settings(out, st, cam);

	save_scene(out_scene, scene, scene_lgh_obj, scene_lgh);

}

bool Parser::load(const char* settings_name, const char* scene_name, Settings& st, camera& cam, std::vector<Object>& scene) {

    std::ifstream in(settings_name, std::ios::binary);
    std::ifstream in_scene(scene_name, std::ios::binary);

    if (!in || !in_scene)
        return false;

    load_settings(in, st, cam);

    if (!load_scene(in_scene))
        return false;

    return true;
}

void Parser::save_settings(std::ofstream& out, Settings& st, camera& cam) {

    Write(out, st.VSync);
    Write(out, st.fov);
    Write(out, st.win_mode);

    Write(out, st.light_active);
    Write(out, st.normal_map_active);
    Write(out, st.shadow_active);
    Write(out, st.wireframe_active);
    Write(out, st.physics_active);
    Write(out, st.collision_active);

    WriteString(out, st.win_name);
    WriteString(out, st.skybox);

    Write(out, cam.cam_position);
    Write(out, cam.cam_target);
    Write(out, cam.cam_up);

    Write(out, cam.alpha);
    Write(out, cam.beta);
    Write(out, cam.radius);

}

void Parser::load_settings(std::ifstream& in, Settings& st, camera& cam) {

    Read(in, st.VSync);
    Read(in, st.fov);
    Read(in, st.win_mode);

    Read(in, st.light_active);
    Read(in, st.normal_map_active);
    Read(in, st.shadow_active);
    Read(in, st.wireframe_active);
    Read(in, st.physics_active);
    Read(in, st.collision_active);

    ReadString(in, st.win_name);
    ReadString(in, st.skybox);

    st.last_win_mode = st.win_mode;

    Read(in, cam.cam_position);
    Read(in, cam.cam_target);
    Read(in, cam.cam_up);

    Read(in, cam.alpha);
    Read(in, cam.beta);
    Read(in, cam.radius);
}

void Parser::save_scene(std::ofstream& out, const std::vector<Object>& scene, const std::vector<Object>& scene_lgh_obj, const std::vector<::Light>& scene_lgh) {
    
    uint32_t count = scene.size();
    uint32_t lightObjCount = scene_lgh_obj.size();
    uint32_t lightCount = scene_lgh.size();
    uint32_t animCount = animations.size();

    Write(out, count);
    Write(out, lightObjCount);
    Write(out, lightCount);
    Write(out, animCount);

    for (auto& obj : scene) {

        Write(out, obj.id);

        WriteString(out, obj.name);
        WriteString(out, obj.mesh);
        WriteString(out, obj.tex);

        WriteString(out, obj.norm_map);

        Write(out, obj.hasNormalMap);

        Write(out, obj.position);
        Write(out, obj.scale);

        glm::vec3 euler = glm::degrees(glm::eulerAngles(obj.rotation));

        Write(out, euler);

        Write(out, obj.min);
        Write(out, obj.max);

        Write(out, obj.shininess);

        Write(out, obj.mapping);
        Write(out, obj.coordType);
        Write(out, obj.cyaxisType);

        Write(out, obj.physics);
        Write(out, obj.hasPhysics);

        Write(out, obj.collider);
        Write(out, obj.hasCollider);

        Write(out, obj.hasMaterial);
        Write(out, obj.hasMapping);

        Write(out, obj.hasAnimation);

        Write(out, obj.tiling);

        Write(out, obj.visible);

    }

    for (int i = 0; i < lightObjCount; ++i) {

        Write(out, scene_lgh_obj[i].id);

        WriteString(out, scene_lgh_obj[i].name);
        WriteString(out, scene_lgh_obj[i].mesh);
        WriteString(out, scene_lgh_obj[i].tex);

        WriteString(out, scene_lgh_obj[i].norm_map);

        Write(out, scene_lgh_obj[i].position);
        Write(out, scene_lgh_obj[i].scale);

        glm::vec3 euler = glm::degrees(glm::eulerAngles(scene_lgh_obj[i].rotation));

        Write(out, euler);

        Write(out, scene_lgh_obj[i].min);
        Write(out, scene_lgh_obj[i].max);

        Write(out, scene_lgh_obj[i].shininess);

        Write(out, scene_lgh_obj[i].mapping);
        Write(out, scene_lgh_obj[i].coordType);
        Write(out, scene_lgh_obj[i].cyaxisType);

        Write(out, scene_lgh_obj[i].physics);
        Write(out, scene_lgh_obj[i].hasPhysics);

        Write(out, scene_lgh_obj[i].collider);
        Write(out, scene_lgh_obj[i].hasCollider);

        Write(out, scene_lgh_obj[i].hasMaterial);
        Write(out, scene_lgh_obj[i].hasMapping);

        Write(out, scene_lgh_obj[i].hasAnimation);

        Write(out, scene_lgh_obj[i].tiling);

        Write(out, scene_lgh_obj[i].visible);
    }

    for (int i = 0; i < lightCount; ++i) {

        Write(out, scene_lgh[i].id);

        WriteString(out, scene_lgh[i].name);

        Write(out, scene_lgh[i].pos);
        Write(out, scene_lgh[i].dir);
        Write(out, scene_lgh[i].col);
        Write(out, scene_lgh[i].att);

        glm::vec3 euler = glm::degrees(glm::eulerAngles(scene_lgh_obj[i].rotation));
        Write(out, euler);

        Write(out, scene_lgh[i].ambient);

        Write(out, scene_lgh[i].inner);
        Write(out, scene_lgh[i].outer);
        Write(out, scene_lgh[i].falloff);

        Write(out, scene_lgh[i].bias);
        Write(out, scene_lgh[i].pcf);

        WriteString(out, scene_lgh[i].type);

        Write(out, scene_lgh[i].hasAnimation);
    }

    for (auto& a : animations)
        WriteAnimationClip(out, a);

    // animPlayer

    for (auto& pl : animationPlayers) {
    
        Write(out, pl.current_animation);

        Write(out, pl.time);

        Write(out, pl.playing);

        Write(out, pl.speed);
    }

    // animEditor

    Write(out, animationEditor.timelineViewMax);

    Write(out, animationEditor.selectedTrack);

    Write(out, animationEditor.selectedKey);

    Write(out, animationEditor.draggingKey);

    Write(out, animationEditor.dragOffsetTime);

}

bool Parser::load_scene(std::ifstream& in) {
    
    uint32_t count;
    uint32_t lightObjCount;
    uint32_t lightCount;
    uint32_t animCount;

    Read(in, count);
    Read(in, lightObjCount);
    Read(in, lightCount);
    Read(in, animCount);

    objects.clear();
    lights_objects.clear();
    lights.clear();
    anim.clear();

    objects.resize(count);
    lights_objects.resize(lightObjCount);
    lights.resize(lightCount);
    anim.resize(animCount);

    for (auto& obj : objects) {

        Read(in, obj.id);

        if (obj.id == 0)
            obj.id = GenerateEntityID();

        ReadString(in, obj.obj_name);
        ReadString(in, obj.mesh);
        ReadString(in, obj.tex);

        ReadString(in, obj.normal_map);

        Read(in, obj.hasNormalMap);

        Read(in, obj.position);
        Read(in, obj.scale);

        glm::vec3 euler;

        Read(in, euler);

        obj.rotation = glm::quat(glm::radians(euler));

        Read(in, obj.min);
        Read(in, obj.max);

        Read(in, obj.shininess);

        Read(in, obj.mapping);
        Read(in, obj.coordType);
        Read(in, obj.cyaxisType);

        Read(in, obj.physics);
        Read(in, obj.hasPhysics);

        Read(in, obj.collider);
        Read(in, obj.hasCollider);

        Read(in, obj.hasMaterial);
        Read(in, obj.hasMapping);

        Read(in, obj.hasAnimation);

        Read(in, obj.tiling);

        Read(in, obj.visible);
    }

    for (auto& obj : lights_objects) {

        Read(in, obj.id);

        if (obj.id == 0)
            obj.id = GenerateEntityID();

        ReadString(in, obj.obj_name);
        ReadString(in, obj.mesh);
        ReadString(in, obj.tex);

        ReadString(in, obj.normal_map);

        Read(in, obj.position);
        Read(in, obj.scale);

        glm::vec3 euler;

        Read(in, euler);

        obj.rotation = glm::quat(glm::radians(euler));

        Read(in, obj.min);
        Read(in, obj.max);

        Read(in, obj.shininess);

        Read(in, obj.mapping);
        Read(in, obj.coordType);
        Read(in, obj.cyaxisType);

        Read(in, obj.physics);
        Read(in, obj.hasPhysics);

        Read(in, obj.collider);
        Read(in, obj.hasCollider);

        Read(in, obj.hasMaterial);
        Read(in, obj.hasMapping);

        Read(in, obj.hasAnimation);

        Read(in, obj.tiling);

        Read(in, obj.visible);
    }

    for (auto& lgh : lights) {

        Read(in, lgh.id);

        if (lgh.id == 0)
            lgh.id = GenerateEntityID();

        ReadString(in, lgh.name);

        Read(in, lgh.pos);
        Read(in, lgh.dir);
        Read(in, lgh.col);
        Read(in, lgh.att);

        glm::vec3 euler;

        Read(in, euler);

        lgh.rotation = glm::quat(glm::radians(euler));

        Read(in, lgh.amb);

        Read(in, lgh.inner);
        Read(in, lgh.outer);
        Read(in, lgh.falloff);

        Read(in, lgh.bias);
        Read(in, lgh.pcf);

        ReadString(in, lgh.type);

        Read(in, lgh.hasAnimation);
    }

    for (auto& a : anim)
        ReadAnimationClip(in, a);

    // animPlayer

    for (int i = 0; i < animationPlayers.size(); i++) {

        Read(in, animPlayer[i].current_animation);

        Read(in, animPlayer[i].time);

        Read(in, animPlayer[i].playing);

        Read(in, animPlayer[i].speed);
    }

    // animEditor

    Read(in, animEditor.timelineViewMax);

    Read(in, animEditor.selectedTrack);

    Read(in, animEditor.selectedKey);

    Read(in, animEditor.draggingKey);

    Read(in, animEditor.dragOffsetTime);

    return static_cast<bool>(in);
}