#version 300 es
precision highp float;

layout (location = 0) in vec2 aPos;
// location 1 = normals
layout (location = 2) in vec2 aTexCoords;

out vec2 texCoords;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); 
    texCoords = aTexCoords;
} 
