#version 430

const int MAX_LIGHTS = 8;

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;
layout(location = 3) uniform mat4 MVP;
layout(location = 4) uniform mat4 model;
layout(location = 5) in vec3 aTangent;
layout(location = 6) in vec3 aBitangent;
layout(location = 7) uniform mat4 aView;
layout(location = 8) uniform mat4 lightVP[MAX_LIGHTS];

out vec2 uv;
out vec3 normal;
out vec3 fragPos;

uniform int numLights;

out vec3 tangent;
out vec3 bitangent;

out vec4 fragPosLight[MAX_LIGHTS];

out vec3 localPos;
out vec3 localNormal;

void main() {

    localPos = aPosition;

    localNormal= aNormal;

    gl_Position = MVP * vec4(aPosition, 1.0);

    for(int i=0; i < numLights; i++)
        fragPosLight[i] = lightVP[i] * model * vec4(aPosition,1.0); // transform it to light space

    uv = aUV;

    fragPos = vec3(aView * model * vec4(aPosition, 1.0));

    mat3 normalMatrix = transpose(inverse(mat3(aView * model)));

    normal = normalize(normalMatrix * aNormal);

    tangent = normalize(normalMatrix * aTangent);

    bitangent = normalize(normalMatrix * aBitangent);

}