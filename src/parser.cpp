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

void Parser::save(const char* fname, Settings& st, const std::vector<Object>& scene) {

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

    uint32_t count = scene.size();

    Write(out, count);

    for (auto& obj : scene) {
    
        WriteString(out, obj.name);
        WriteString(out, obj.mesh);
        WriteString(out, obj.tex);

        Write(out, obj.position);
        Write(out, obj.scale);
        Write(out, obj.rotation);

        Write(out, obj.min);
        Write(out, obj.max);

        Write(out, obj.mapping);
        Write(out, obj.coordType);
        Write(out, obj.cyaxisType);
    }

}

bool Parser::load(const char* fname, Settings& st) {

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

    uint32_t count;

    Read(in, count);

    objects.clear();
    objects.resize(count);

    for (auto& obj : objects) {
    
        ReadString(in, obj.obj_name);
        ReadString(in, obj.mesh);
        ReadString(in, obj.tex);

        Read(in, obj.position);
        Read(in, obj.scale);
        Read(in, obj.rotation);

        Read(in, obj.min);
        Read(in, obj.max);

        Read(in, obj.mapping);
        Read(in, obj.coordType);
        Read(in, obj.cyaxisType);
    }

    return true;
}