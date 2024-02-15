#version 300 es
precision highp float;

layout (location = 0) out vec3 gViewPosition;
layout (location = 1) out vec3 gViewNormal;
layout (location = 2) out vec3 gAlbedo;
layout (location = 3) out vec3 gMetallic;
layout (location = 4) out vec3 gRoughness;
layout (location = 5) out vec3 gAmbientOcclusion;

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
    gViewPosition = fragViewPos;

    // Store the per-fragment normals and roughness into the gbuffer.
    vec3 n = texture(material.normal_map, texCoords).rgb;
    n = n * 2.0 - 1.0; //[0,1] -> [-1,1]
    gViewNormal = normalize(tangentToViewMatrix * n);

    gAlbedo = texture(material.albedo_map, texCoords).rgb;

    gMetallic = texture(material.metallic_map, texCoords).rgb;
    gRoughness = texture(material.roughness_map, texCoords).rgb;
    gAmbientOcclusion = texture(material.ao_map, texCoords).rgb;
}