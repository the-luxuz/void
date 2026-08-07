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

void Parser::save(const char* fname, Settings& st, camera& cam, const std::vector<Object>& scene, const std::vector<Object>& scene_lgh_obj, const std::vector<::Light>& scene_lgh) {

    std::ofstream out(fname, std::ios::binary);

    if (!out)
        return;

    /*Write(out, st.w);
    Write(out, st.h);*/
    Write(out, st.VSync);
    Write(out, st.fov);
    Write(out, st.win_mode);

    WriteString(out, st.win_name);
    WriteString(out, st.skybox);

    Write(out, cam.cam_position);
    Write(out, cam.cam_target);
    Write(out, cam.cam_up);

    Write(out, cam.alpha);
    Write(out, cam.beta);
    Write(out, cam.radius);

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
    }

    for (auto& obj : scene_lgh_obj) {

        WriteString(out, obj.name);
        WriteString(out, obj.mesh);
        WriteString(out, obj.tex);

        WriteString(out, obj.norm_map);

        Write(out, obj.obj_texture);
        Write(out, obj.normalTexture);

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
    }

    for (auto& lgh : scene_lgh) {

        WriteString(out, lgh.name);

        Write(out, lgh.pos);
        Write(out, lgh.dir);
        Write(out, lgh.col);
        Write(out, lgh.att);

        Write(out, lgh.ambient);

        Write(out, lgh.inner);
        Write(out, lgh.outer);
        Write(out, lgh.falloff);

        Write(out, lgh.bias);
        Write(out, lgh.pcf);

        WriteString(out, lgh.type);
    }

}

bool Parser::load(const char* fname, Settings& st, camera& cam, std::vector<Object>& scene_lgh_obj) {

    std::ifstream in(fname, std::ios::binary);

    if (!in)
        return false;

    /*Read(in, st.w);
    Read(in, st.h);*/
    Read(in, st.VSync);
    Read(in, st.fov);
    Read(in, st.win_mode);

    ReadString(in, st.win_name);

    ReadString(in, st.skybox);

    st.last_win_mode = st.win_mode;

    Read(in, cam.cam_position);
    Read(in, cam.cam_target);
    Read(in, cam.cam_up);

    Read(in, cam.alpha);
    Read(in, cam.beta);
    Read(in, cam.radius);

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