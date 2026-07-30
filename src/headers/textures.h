#pragma once

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "tinyexr.h"

#include "local.h"

#include <filesystem>
#include <iostream>

GLuint skybox_texture;

GLuint LoadEXRTexture(const std::string& path) {

    //std::cout << std::filesystem::current_path() << std::endl;

    std::string fullPath = "../" + path;

    if (!std::filesystem::exists(fullPath)) {

        std::cout << "Doesnn't exist: " << fullPath << std::endl;
        return 0;
    }

    float* data = nullptr;

    int width;
    int height;

    const char* err = nullptr;

    int ret = LoadEXR(&data, &width, &height, fullPath.c_str(), &err);

    if (ret != TINYEXR_SUCCESS) {

        std::cout << "Couldn't load EXR: " << fullPath << std::endl;

        if (err) {

            std::cout << err << std::endl;
            FreeEXRErrorMessage(err);
        }

        return 0;
    }

    GLuint texture;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    free(data);

    return texture;
}

GLuint LoadHDRTexture(const std::string& path) {

    stbi_set_flip_vertically_on_load(true);

    int width, height, channels;

    std::string fullPath = "../" + path;

    float* data = stbi_loadf(fullPath.c_str(), &width, &height, &channels, 0);

    if (!data) {

        std::cout << "No se pudo cargar HDR\n";
        return 0;
    }

    GLuint texture;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB16F,
        width,
        height,
        0,
        GL_RGB,
        GL_FLOAT,
        data
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
}

void Load_Skybox(const std::string& path) {

    std::string fullPath = "../" + path;

    std::filesystem::path p(fullPath);

    std::string ext = p.extension().string();

    if (ext == ".exr")
        skybox_texture = LoadEXRTexture(path);

    else if (ext == ".hdr")
        skybox_texture = LoadHDRTexture(path);
    else
        std::cout << "Formato no soportado: " << ext << std::endl;
   
}