#version 430

const int MAX_LIGHTS = 8;

struct Light {

    int type; // 0 = POINT, 1 = DIR, 2 = SPOT

    vec3 position;
    vec3 direction;
    vec3 color;

    vec3 attenuation;

    float ambient;

    float bias;
    float pcf;

    float inner;
    float outer;
    float falloff;
};

uniform int numLights;
uniform Light lights[MAX_LIGHTS];

uniform sampler2D tex;

uniform bool useTexture;
uniform vec3 color;

uniform sampler2D normalMap;
uniform sampler2D depthData[MAX_LIGHTS];

uniform bool hasNormalMap;

uniform bool light_active;
uniform bool normal_map_active;
uniform bool shadow_active;

uniform float shininess;

uniform mat4 aView;

in vec2 uv;
in vec3 normal;
in vec3 fragPos;

in vec3 tangent;
in vec3 bitangent;

out vec4 outputColor;

// light
in vec4 fragPosLight[MAX_LIGHTS];
float shadow = 0.0;

vec3 ShadePointLight(Light light, vec3 N, vec3 V, vec3 baseColor) {

    vec3 lightPosView = vec3(aView * vec4(light.position, 1.0));

    vec3 Lvec = lightPosView - fragPos;
    float dist = length(Lvec);
    vec3 L = normalize(Lvec);

    float diff = max(dot(N, L), 0.0);

    float denom = light.attenuation.x + light.attenuation.y * dist + light.attenuation.z * dist * dist;

    float att = 1.0 / max(denom, 0.0001);

    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), shininess);

    vec3 diffuse = diff * light.color * min(att, 1.0);
    vec3 specular = spec * light.color * min(att, 1.0);

    return (light.ambient * light.color * baseColor) + (1.0 - shadow) * ((diffuse * baseColor)  + specular);
}

vec3 ShadeDirLight(Light light, vec3 N, vec3 V, vec3 baseColor) {

    vec3 dirView = normalize(mat3(aView) * light.direction);

    vec3 L = normalize(-dirView);

    float diff = max(dot(N, L), 0.0);

    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), shininess);

    vec3 diffuse = diff * light.color;
    vec3 specular = spec * light.color;

    return (light.ambient * light.color * baseColor) + (1.0 - shadow) * ((diffuse * baseColor) + specular);
}

vec3 ShadeSpotLight(Light light, vec3 N, vec3 V, vec3 baseColor) {

    vec3 lightPosView = vec3(aView * vec4(light.position, 1.0));

    vec3 Lvec = lightPosView - fragPos;

    float dist = length(Lvec);

    vec3 L = normalize(Lvec);

    vec3 dirView = normalize(mat3(aView) * light.direction);

    float alpha = dot(-L, dirView);

    float phi = cos(radians(light.outer));
    float theta = cos(radians(light.inner));

    vec3 diffuse;
    vec3 specular;

    if(alpha < phi)
        return light.ambient * light.color * baseColor;

    float spotEff = 1.0f;

    if(alpha < theta)
        spotEff = clamp(pow((alpha - phi) / (theta - phi), light.falloff), 0.0, 1.0);

    float diff = max(dot(N, L), 0.0);

    float denom = light.attenuation.x + light.attenuation.y * dist + light.attenuation.z * dist * dist;

    float att = 1.0 / max(denom, 0.0001);

    vec3 R = reflect(-L, N);

    float spec = pow(max(dot(V, R), 0.0), shininess);

    diffuse = diff * light.color * min(att, 1.0) * spotEff;

    specular = spec * light.color * min(att, 1.0) * spotEff;

    return (light.ambient * light.color * baseColor) + (1.0 - shadow) * ((diffuse * baseColor) + specular);
}

void main() {

    vec3 baseColor;

    if(useTexture)
        baseColor = texture(tex, uv).rgb;
    else
        baseColor = color;

    if(!useTexture){
    
        outputColor = vec4(baseColor, 1.0);
        return;
    }

    vec3 T = normalize(tangent);
    vec3 N = normalize(normal);

    T = normalize(T - N * dot(N, T));

    vec3 B = normalize(cross(N, T));

    mat3 TBN = mat3(T, B, N);

    vec3 finalNormal;

    if(hasNormalMap && normal_map_active) {

        vec3 normalTS = texture(normalMap, uv).xyz;
        normalTS = normalTS * 2.0 - 1.0;
        finalNormal = normalize(TBN * normalTS);
    }
    else {

        finalNormal = normalize(N);
    }

    vec3 V = normalize(-fragPos);

    vec3 result = vec3(0.0);

    if(!light_active){
        
        outputColor = vec4(baseColor, 1.0);
        return;
    }

    for (int i = 0; i < numLights; i++) {

        if(shadow_active){
        
            shadow = 0.0;

            vec3 proj = fragPosLight[i].xyz / fragPosLight[i].w;
            vec3 norm_proj = proj * 0.5 + 0.5;

            if(norm_proj.x >= 0.0 && norm_proj.x <= 1.0 && norm_proj.y >= 0.0 && norm_proj.y <= 1.0 && norm_proj.z <= 1.0) {

                float currentDepth = norm_proj.z;

                vec2 texelSize = 1.0 / textureSize(depthData[i], 0);

                int radius = int(lights[i].pcf);

                for(int x = -radius; x <= radius; x++) { // like doing the blur

                    for(int y = -radius; y <= radius; y++) {

                        float depth = texture(depthData[i], norm_proj.xy + vec2(x,y) * texelSize).r;

                        if(currentDepth - lights[i].bias > depth)
                            shadow += 1.0;
                    }
                }

                float samples = float(pow((2 * radius + 1), 2));
                shadow /= samples;
            }
        }

        if (lights[i].type == 0)
            result += ShadePointLight(lights[i], finalNormal, V, baseColor);
        else if (lights[i].type == 1)
            result += ShadeDirLight(lights[i], finalNormal, V, baseColor);
        else if (lights[i].type == 2)
            result += ShadeSpotLight(lights[i], finalNormal, V, baseColor);
    }

    outputColor = vec4(result, 1.0);
}