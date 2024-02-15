#version 300 es
precision highp float;

layout(location = 0) in vec3 aPos;

out vec3 localFragPos;

struct Transform{
  //mat4 model;
  mat4 view;
  mat4 projection;
};

uniform Transform transform;

void main() {
    localFragPos = aPos;
    gl_Position =  transform.projection * transform.view * vec4(localFragPos, 1.0);
}