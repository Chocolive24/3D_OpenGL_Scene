#version 300 es
precision highp float;

layout (location = 0) in vec3 aPos;

out vec3 fragPos; // for point shadow mapping.

struct Transform {
    mat4 model;
    // mat4 view;
    // mat4 projection;
};

uniform mat4 lightSpaceMatrix;
uniform Transform transform;

void main()
{
    fragPos = vec3(transform.model * vec4(aPos, 1.0));
    gl_Position = lightSpaceMatrix * transform.model * vec4(aPos, 1.0);
}  
