#include "parser.h"

void Parser::WriteString(std::ofstream& out, const std::string& str) {

    uint32_t len = static_cast<uint32_t>(str.size());

    out.write(reinterpret_cast<const char*>(&len), sizeof(len));
    out.write(str.data(), len);
}

void Parser::ReadString(std::ifstream& in, std::string& str) {

    uint32_t len;

    in.read(reinterpret_cast<char*>(&len), sizeof(len));

    str.resize(len);

    in.read(str.data(), len);
}

void Parser::save(const char* settings_name, const char* scene_name, Settings& st, camera& cam, const std::vector<Object>& scene, const std::vector<Object>& scene_lgh_obj, const std::vector<::Light>& scene_lgh) {

    std::ofstream out(settings_name, std::ios::binary);
    std::ofstream out_scene(scene_name, std::ios::binary);

    if (!out)
        return;

    save_settings(out, st, cam);

	save_scene(out_scene, scene, scene_lgh_obj, scene_lgh);

}

bool Parser::load(const char* settings_name, const char* scene_name, Settings& st, camera& cam, std::vector<Object>& scene, std::vector<Object>& scene_lgh_obj) {

    std::ifstream in(settings_name, std::ios::binary);
    std::ifstream in_scene(scene_name, std::ios::binary);

    if (!in)
        return false;

    load_settings(in, st, cam);

    load_scene(in_scene, scene_lgh_obj);

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

    Write(out, count);
    Write(out, lightObjCount);
    Write(out, lightCount);

    for (auto& obj : scene) {

        WriteString(out, obj.name);
        WriteString(out, obj.mesh);
        WriteString(out, obj.tex);

        WriteString(out, obj.norm_map);

        Write(out, obj.obj_texture);
        Write(out, obj.normalTexture);

        Write(out, obj.has_normal_map);

        Write(out, obj.position);
        Write(out, obj.scale);

        glm::vec3 euler = glm::degrees(glm::eulerAngles(obj.rotation));

        Write(out, euler);

        Write(out, obj.min);
        Write(out, obj.max);

        Write(out, obj.model);
        Write(out, obj.mvp);

        Write(out, obj.shininess);

        Write(out, obj.mapping);
        Write(out, obj.coordType);
        Write(out, obj.cyaxisType);

        Write(out, obj.physics);
        Write(out, obj.hasPhysics);

    }

    for (int i = 0; i < lightObjCount; ++i) {

        WriteString(out, scene_lgh_obj[i].name);
        WriteString(out, scene_lgh_obj[i].mesh);
        WriteString(out, scene_lgh_obj[i].tex);

        WriteString(out, scene_lgh_obj[i].norm_map);

        Write(out, scene_lgh_obj[i].obj_texture);
        Write(out, scene_lgh_obj[i].normalTexture);

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
    }

    for (int i = 0; i < lightCount; ++i) {

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
    }
}

bool Parser::load_scene(std::ifstream& in, std::vector<Object>& scene_lgh_obj) {
    
    uint32_t count;
    uint32_t lightObjCount;
    uint32_t lightCount;

    Read(in, count);
    Read(in, lightObjCount);
    Read(in, lightCount);

    objects.clear();
    scene_lgh_obj.clear();
    lights.clear();

    objects.resize(count);
    scene_lgh_obj.resize(lightObjCount);
    lights.resize(lightCount);

    for (auto& obj : objects) {

        ReadString(in, obj.obj_name);
        ReadString(in, obj.mesh);
        ReadString(in, obj.tex);

        ReadString(in, obj.normal_map);

        Read(in, obj.obj_texture);
        Read(in, obj.normalTexture);

        Read(in, obj.has_normal_map);

        Read(in, obj.position);
        Read(in, obj.scale);

        glm::vec3 euler;

        Read(in, euler);

        obj.rotation = glm::quat(glm::radians(euler));

        Read(in, obj.min);
        Read(in, obj.max);

        Read(in, obj.model);
        Read(in, obj.mvp);

        Read(in, obj.shininess);

        Read(in, obj.mapping);
        Read(in, obj.coordType);
        Read(in, obj.cyaxisType);

        Read(in, obj.physics);
        Read(in, obj.hasPhysics);
    }

    for (auto& obj : scene_lgh_obj) {

        ReadString(in, obj.name);
        ReadString(in, obj.mesh);
        ReadString(in, obj.tex);

        ReadString(in, obj.norm_map);

        Read(in, obj.obj_texture);
        Read(in, obj.normalTexture);

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
    }

    for (auto& lgh : lights) {

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
    }

    return true;
}