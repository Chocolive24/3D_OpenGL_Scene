#version 300 es
precision highp float;

in vec3 fragPos;

uniform vec3 light_pos;
uniform float light_far_plane;

void main()
{
    vec3 delta = fragPos - light_pos;
    gl_FragDepth = length(delta)/light_far_plane;
}