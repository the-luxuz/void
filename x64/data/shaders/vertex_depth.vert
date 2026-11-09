#version 430

layout(location = 0) in vec3 aPosition;

layout(location = 3) uniform mat4 lightMVP;

void main()
{
    gl_Position = lightMVP * vec4(aPosition,1.0);
}