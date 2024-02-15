#version 300 es
precision highp float;

layout (location = 0) out vec4 fragColor;

in vec3 localFragPos;

uniform sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec2 uv = SampleSphericalMap(normalize(localFragPos)); // make sure to normalize localPos
    vec3 color = texture(equirectangularMap, uv).rgb;
    color = clamp(color, vec3(0.0), vec3(100.0));

    fragColor = vec4(color, 1.0);
}