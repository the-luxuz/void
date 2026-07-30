uniform sampler2D equirectangularMap;

in vec3 LocalPos;

out vec4 FragColor;

const vec2 invAtan = vec2(0.1591, 0.3183);

vec2 SampleSphericalMap(vec3 v) {

    vec2 uv = vec2(atan(v.z,v.x), asin(v.y));

    uv *= invAtan;

    uv += 0.5;

    return uv;
}

void main() {

    vec3 dir = normalize(LocalPos);

    vec2 uv = SampleSphericalMap(dir);
    uv.y = 1.0 - uv.y;

    FragColor = texture(equirectangularMap, uv);
}