#version 330 core

in vec3 vNormal;

uniform vec3 uColor;

out vec4 outputColor;

void main() {

    vec3 normal = dot(vNormal, vNormal) > 0.000001 ? normalize(vNormal) : vec3(0.0, 1.0, 0.0);

    vec3 lightDirection = normalize(vec3(-0.45, 0.8, 0.6));

    float diffuse = max(dot(normal, lightDirection), 0.0);

    float lighting = 0.28 + diffuse * 0.72;

    outputColor = vec4(uColor * lighting, 1.0);
}
