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

    mat4 rotView = mat4(mat3(transform.view)); // remove translation from the view matrix
    vec4 clipPos = transform.projection * rotView * vec4(localFragPos, 1.0);

    // w instead of z to always have a z value of 1.
    gl_Position = clipPos.xyww;
}
