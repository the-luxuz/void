#include "parser.h"

void Parser::load(const char* fname) {

    std::ifstream f(fname);

    if (!f.is_open()) {

        std::cout << "Error al abrir el archivo.\n";
        return;
    }

    std::string id;
    std::string value;

    while (f >> id) {

        if (!id.empty() && id[0] == '#') {

            std::getline(f, value);
            continue;
        }

        if (!(f >> value))
            break;

        if (id == "wname")
            win_name = value;

        else if (id == "width")
            w = std::stoi(value);

        else if (id == "height")
            h = std::stoi(value);

        else if (id == "fov")
            fov = std::stoi(value);

        else if (id == "obj")
            objects.emplace_back(value);

        else if (id == "mesh")
            objects.back().mesh = value;

        else if (id == "pos") {

            float x = std::stof(value);

            float y, z;
            f >> y >> z;

            objects.back().position = glm::vec3(x, y, z);
        }

        else if (id == "scal") {

            float x = std::stof(value);

            float y, z;
            f >> y >> z;

            objects.back().scale = glm::vec3(x, y, z);
        }

        else if (id == "rot") {

            float x = std::stof(value);

            float y, z;
            f >> y >> z;

            objects.back().rotation = glm::vec3(x, y, z);
        }
    }

    f.close();
}