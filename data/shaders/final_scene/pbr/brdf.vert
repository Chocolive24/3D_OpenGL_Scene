#version 300 es
precision highp float;

layout (location = 0) in vec3 aPos;
// location 1 = normal.
layout (location = 2) in vec2 aTexCoords;

out vec2 texCoords;

void main()
{
    texCoords = aTexCoords;
	gl_Position = vec4(aPos, 1.0);
}
