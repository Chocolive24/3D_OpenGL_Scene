#version 300 es
precision highp float;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 brightColor; // For the bloom.

in vec2 texCoords;

// IBL
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D   brdfLUT;

// G-buffer values.
uniform sampler2D gViewPosition;
uniform sampler2D gViewNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gMetallic;
uniform sampler2D gRoughness;
uniform sampler2D gAmbientOcclusion;

uniform sampler2D ssao;

uniform sampler2D shadowMap;

struct DirectionalLight {
    vec3 view_direction;
    vec3 color;
};

uniform DirectionalLight directional_light;

uniform float combined_ao_factor;

uniform vec3 lightViewPositions[4];
uniform vec3 lightColors[4];

// ATTENTION; PEUT ETRE PAS BIEN DE CASTER LES MAT4 EN MAT3 !!!!
uniform mat4 inverseViewMatrix;
uniform mat4 lightSpaceMatrix;

const float PI = 3.14159265359;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 viewNormal)
{
    // Perform perspective divide (return value in range [-1, 1]).
    // Useless for orthographic projection but obligatory for persepctive projection.
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // Transform the NDC coordinates to the range [0,1] to get a position in the depth map.
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) {
        return 0.0;
    }

    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    vec3 lightDir = -directional_light.view_direction; //normalize(lightPos - fragPos);
    float bias = max(0.05 * (1.0 - dot(viewNormal, lightDir)), 0.005);

    // check whether current frag pos is in shadow
    //float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;

    // PCF
    float shadow = 0.0;
    vec2 texelSize = vec2(1.0) / vec2(textureSize(shadowMap, 0));

    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

// F0 is the surface reflection at zero incidence or how much the surface reflects 
// if looking directly at the surface.
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    // The clamp prevents from black spots.
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 CalculateDirectionalLightRadiance(vec3 viewDir, vec3 viewNormal, float roughness,
                                       float metallic, vec3 albedo, vec3 fragViewPos, vec3 F0)
{
    // Directional light (+ shadow).
    vec3 L = normalize(-directional_light.view_direction);
    vec3 H = normalize(viewDir + L);

    vec3 radiance = directional_light.color;

    float NDF = DistributionGGX(viewNormal, H, roughness);
    float G   = GeometrySmith(viewNormal, viewDir, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, viewDir), 0.0), F0);

    // Calculate the Cook-Torrance BRDF.
    vec3 numerator    = NDF * G * F;
    // 0.0001 avoid to divide by 0 if the dot is equal to 0.
    float denominator = 4.0 * max(dot(viewNormal, viewDir), 0.0) * max(dot(viewNormal, L), 0.0)  + 0.0001;
    vec3 specular     = numerator / denominator;

    // kS is equal to Fresnel
    vec3 kS = F;

    // for energy conservation, the diffuse and specular light can't
    // be above 1.0 (unless the surface emits light); to preserve this
    // relationship the diffuse component (kD) should equal 1.0 - kS.
    vec3 kD = vec3(1.0) - kS;
    // multiply kD by the inverse metalness such that only non-metals
    // have diffuse lighting, or a linear blend if partly metal (pure metals
    // have no diffuse light).
    kD *= 1.0 - metallic;

    float NdotL = max(dot(viewNormal, L), 0.0);

    // Calculate shadow.
    vec4 fragWorldPos = inverseViewMatrix * vec4(fragViewPos, 1.0);
    vec4 fragPosLightSpace = lightSpaceMatrix * fragWorldPos;
    //vec3 worldNormal = mat3(inverseViewMatrix) * viewNormal;

    float shadow = ShadowCalculation(fragPosLightSpace, viewNormal);

    // Outgoing directional light radiance with shadow.
    return (1.0 - shadow) * (kD * albedo / PI + specular) * radiance * NdotL;
}

void main() {
    vec3  albedo    = texture(gAlbedo, texCoords).rgb; // loaded in SRGB
    float metallic  = dot(texture(gMetallic, texCoords).rgb, vec3(1.0));
    float roughness = dot(texture(gRoughness, texCoords).rgb, vec3(1.0));
    float ao        = dot(texture(gAmbientOcclusion, texCoords).rgb, vec3(1.0));
    float ssao = texture(ssao, texCoords).r;

    float combined_ao = mix(ssao, ao, combined_ao_factor);

    vec3 fragViewPos = texture(gViewPosition, texCoords).rgb;

    vec3 viewNormal = texture(gViewNormal, texCoords).rgb;
    vec3 viewDir = normalize(-fragViewPos);
    vec3 viewReflection = reflect(-viewDir, viewNormal);

    // Transform the reflection vector from view to world space.
    vec3 worldReflection = mat3(inverseViewMatrix) * viewReflection;

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);

    Lo += CalculateDirectionalLightRadiance(viewDir, viewNormal, roughness,
                                            metallic, albedo, fragViewPos, F0);

    // Points lights.
    for(int i = 0; i < 4; ++i)
    {
        vec3 L = normalize(lightViewPositions[i] - fragViewPos);
        vec3 H = normalize(viewDir + L);
  
        float distance    = length(lightViewPositions[i] - fragViewPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance     = lightColors[i] * attenuation; 

        float NDF = DistributionGGX(viewNormal, H, roughness);
        float G   = GeometrySmith(viewNormal, viewDir, L, roughness);
        vec3 F = FresnelSchlick(max(dot(H, viewDir), 0.0), F0);

        // Calculate the Cook-Torrance BRDF.
        vec3 numerator    = NDF * G * F;
        // 0.0001 avoid to divide by 0 if the dot is equal to 0.
        float denominator = 4.0 * max(dot(viewNormal, viewDir), 0.0) * max(dot(viewNormal, L), 0.0)  + 0.0001;
        vec3 specular     = numerator / denominator; 

        // kS is equal to Fresnel
        vec3 kS = F;

        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;

        // Calculate outgoing radiance.
        float NdotL = max(dot(viewNormal, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // ambient lighting (we now use IBL as the ambient term)
    vec3 F = FresnelSchlickRoughness(max(dot(viewNormal, viewDir), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 irradiance = texture(irradianceMap, viewNormal).rgb;
    vec3 diffuse = irradiance * albedo;

    // sample both the pre-filter map and the BRDF lut and combine them together as per the
    // Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, worldReflection,  roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(viewNormal, viewDir), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * combined_ao;

    vec3 color = ambient + Lo;

    // check whether result is higher than some threshold, if so, output as bloom threshold color
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));

    if(brightness > 1.0)
        brightColor = vec4(color, 1.0);
    else
        brightColor = vec4(0.0, 0.0, 0.0, 1.0);

    fragColor = vec4(color, 1.0);
}