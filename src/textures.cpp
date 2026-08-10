#define STB_IMAGE_IMPLEMENTATION

#include "headers/textures.h"

//--------------------------------------------------------------------------
// global variables

// hdr check flag
bool is_hdr = false;

// viewport
GLuint viewport_texture;
GLuint viewportDepthTexture;

int viewport_width = 512;
int viewport_height = 512;

// depth map
GLuint depth_texture[8];

// skybox
GLuint skybox_texture;

//--------------------------------------------------------------------------
// EXR and HDR texture

GLuint LoadEXRTexture(const std::string& path) {

    is_hdr = false;
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

    is_hdr = true;
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

    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

//--------------------------------------------------------------------------
// Skybox

void Load_Skybox(const std::string& path) {

    std::string fullPath = "../" + path;

    std::filesystem::path p(fullPath);

    std::string ext = p.extension().string();
    //std::cout << fullPath << std::endl;

    if (ext == ".exr")
        skybox_texture = LoadEXRTexture(path);

    else if (ext == ".hdr")
        skybox_texture = LoadHDRTexture(path);
    else
        std::cout << "Formato no soportado: " << ext << std::endl;
   
}

//--------------------------------------------------------------------------
// OBJ texture

GLuint LoadObjTexture(const std::string& path) {

    stbi_set_flip_vertically_on_load(true);

    int width, height, channels;

    std::string fullPath = "../" + path;

    unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &channels, 0);

    if (!data) {

        std::cout << "No se pudo cargar la textura\n";

		std::cout << "Path: " << fullPath << std::endl;
        return 0;
    }

    GLuint texture;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    GLenum format = channels == 4 ? GL_RGBA : channels == 3 ? GL_RGB : GL_RED;

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        format,
        width,
        height,
        0,
        format,
        GL_UNSIGNED_BYTE,
        data
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

//--------------------------------------------------------------------------
// Viewport and viewport depth texture

void Create_viewport_tex(int width, int height) {

    glGenTextures(1, &viewport_texture);

    glBindTexture(GL_TEXTURE_2D, viewport_texture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB16F,
        width,
        height,
        0,
        GL_RGB,
        GL_FLOAT,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        viewport_texture,
        0);

    glBindTexture(GL_TEXTURE_2D, 0);
}
void CreateViewportDepthTexture(int width, int height) {
    
    glGenTextures(1, &viewportDepthTexture);
    glBindTexture(GL_TEXTURE_2D, viewportDepthTexture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_DEPTH_COMPONENT24,
        width,
        height,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D,
        viewportDepthTexture,
        0
    );

    GLenum drawBuffers[] = { GL_NONE };
    glDrawBuffers(1, drawBuffers);

    GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if (result != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Depth FBO ERROR: " << result << std::endl;

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Create_depth_tex(int width, int height, int i) {

    glGenTextures(1, &depth_texture[i]);
    glBindTexture(GL_TEXTURE_2D, depth_texture[i]);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_DEPTH_COMPONENT24,
        width,
        height,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D,
        depth_texture[i],
        0
    );

    GLenum drawBuffers[] = { GL_NONE };
    glDrawBuffers(1, drawBuffers);

    GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if (result != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Depth FBO ERROR: " << result << std::endl;

    glBindTexture(GL_TEXTURE_2D, 0);
}

//--------------------------------------------------------------------------
// Normal map

void LoadNormalMap(Object& obj) {

    int w, h, channels;

    std::string fullPath = "../" + obj.norm_map;

    unsigned char* data = stbi_load(fullPath.c_str(), &w, &h, &channels, STBI_rgb_alpha);

    std::cout << obj.norm_map << "\n";

    if (!data) {

        std::cout << "Failed to load normal map: " << obj.norm_map << "\n";

        return;
    }

    glGenTextures(1, &obj.normalTexture);

    glBindTexture(GL_TEXTURE_2D, obj.normalTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        w,
        h,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        data
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
}