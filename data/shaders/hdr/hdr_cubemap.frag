#version 300 es
precision highp float;

layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 brightColor;

in vec3 localFragPos;

uniform samplerCube environmentMap;

void main()
{
    vec3 envColor = textureLod(environmentMap, localFragPos, 0.0).rgb;

    // check whether result is higher than some threshold, if so, output as bloom threshold color
    float brightness = dot(envColor, vec3(0.2126, 0.7152, 0.0722));

    if(brightness > 1.0)
        brightColor = vec4(envColor, 1.0);
    else
        brightColor = vec4(0.0, 0.0, 0.0, 1.0);

    fragColor = vec4(envColor, 1.0);
}