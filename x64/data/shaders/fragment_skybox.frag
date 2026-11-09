#version 330 core

in vec3 TexCoords;

out vec4 FragColor;

uniform samplerCube skybox;

uniform bool is_hdr;

void main() {
    
    if(is_hdr)
        FragColor = texture(skybox, -TexCoords);
    else
        FragColor = texture(skybox, TexCoords);
}