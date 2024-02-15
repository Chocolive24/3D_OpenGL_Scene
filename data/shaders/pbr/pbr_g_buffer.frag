#version 300 es
precision highp float;

layout (location = 0) out vec4 gViewPositionMetallic;
layout (location = 1) out vec4 gViewNormalRoughness;
layout (location = 2) out vec4 gAlbedoAmbientOcclusion;

in vec3 fragViewPos;
in vec2 texCoords;
in mat3 tangentToViewMatrix;

struct Material {
  sampler2D albedo_map;
  sampler2D normal_map;
  sampler2D metallic_map;
  sampler2D roughness_map;
  sampler2D ao_map;
};

uniform Material material;

void main()
{    
    // Store the fragment position vector in the RGB channels and the
    // metallic in the A channel of the first gbuffer texture.
    gViewPositionMetallic.rgb = fragViewPos;
    gViewPositionMetallic.a = texture(material.metallic_map, texCoords).r;

    // Store the per-fragment normals and roughness into the gbuffer.
    vec3 n = texture(material.normal_map, texCoords).rgb;
    n = n * 2.0 - 1.0; //[0,1] -> [-1,1]
    gViewNormalRoughness.rgb = normalize(tangentToViewMatrix * n);
    gViewNormalRoughness.a = texture(material.roughness_map, texCoords).r;

    // Store the base color and the ambient occlusion per-fragment.
    gAlbedoAmbientOcclusion.rgb = texture(material.albedo_map, texCoords).rgb;
    gAlbedoAmbientOcclusion.a = texture(material.ao_map, texCoords).r;
}