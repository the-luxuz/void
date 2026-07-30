#pragma once

#include "local.h"

struct camera {

    camera() : fov(90), nearPlane(.1f), farPlane(5000.0f), cam_position(100, 100, 100), cam_target(0, 0, 0), cam_up(0, 1, 0) {}

    float fov;

    float nearPlane;
    float farPlane;

    glm::vec3 cam_position;
    glm::vec3 cam_target;
    glm::vec3 cam_up;

};

struct Vertex {

    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct Object {

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;

    std::string name;

    GLuint obj_texture;

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    std::string mesh;

    glm::vec3 min, max;

};