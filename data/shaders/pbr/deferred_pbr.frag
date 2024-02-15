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
uniform sampler2D gViewPositionMetallic;
uniform sampler2D gViewNormalRoughness;
uniform sampler2D gAlbedoAmbientOcclusion;
uniform sampler2D gEmissive;

uniform sampler2D ssao;

uniform sampler2D shadowMap; // directional light shadows.
uniform samplerCube shadowCubeMap; // point light shadows.

struct DirectionalLight {
    vec3 world_direction;
    vec3 color;
};

uniform DirectionalLight directional_light;

struct PointLight {
    vec3 position;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
};

uniform PointLight point_light;
uniform float light_far_plane;

uniform float combined_ao_factor;
uniform float emissive_factor;

const int kLightNbr = 1;
uniform PointLight point_lights[kLightNbr];

uniform vec3 viewPos;
uniform mat4 inverseViewMatrix;

uniform mat4 lightSpaceMatrix;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 FresnelSchlick(float cosTheta, vec3 F0);
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);
float ShadowCalculation(vec4 fragPosLightSpace, vec3 worldNormal);
float PointLightShadowCalculation(vec3 fragWorldPos, vec3 worldNormal, PointLight point_light);
vec3 CalculateAmbientLighting(vec3 worldNormal, vec3 worldViewDir, vec3 F0, float roughness,
                              float metallic, vec3 albedo, vec3 reflection, float combined_ao);
vec3 CalculateDirectionalLightRadiance(vec3 worldViewDir, vec3 worldNormal, float roughness,
                                       float metallic, vec3 albedo, vec4 fragWorldPos, vec3 F0);
vec3 CalculatePointLightRadiance(PointLight point_light, vec3 worldViewDir, vec3 worldNormal,
                                 float roughness, float metallic, vec3 albedo, vec4 fragWorldPos, vec3 F0);

void main() {
    vec3  albedo    = texture(gAlbedoAmbientOcclusion, texCoords).rgb; // loaded in SRGB
    float metallic  = texture(gViewPositionMetallic, texCoords).a;
    float roughness = texture(gViewNormalRoughness, texCoords).a;
    float ao        = texture(gAlbedoAmbientOcclusion, texCoords).a;
    float ssao = texture(ssao, texCoords).r;

    float combined_ao = ssao * ao; //mix(ssao, ao, combined_ao_factor);

    vec3 fragViewPos = texture(gViewPositionMetallic, texCoords).rgb;
    vec4 fragWorldPos = inverseViewMatrix * vec4(fragViewPos, 1.0);

    vec3 viewNormal = texture(gViewNormalRoughness, texCoords).rgb;
    vec3 worldNormal = mat3(inverseViewMatrix) * viewNormal;

    vec3 worldViewDir = normalize(viewPos - vec3(fragWorldPos));
    vec3 reflection = reflect(-worldViewDir, worldNormal);

    // Transform the reflection vector from view to world space.
    //vec3 worldReflection = mat3(inverseViewMatrix) * viewReflection;

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // Irradiance.
    vec3 Lo = vec3(0.0);

    Lo += CalculateAmbientLighting(worldNormal, worldViewDir, F0, roughness,
                                   metallic, albedo, reflection, combined_ao);

    Lo += CalculateDirectionalLightRadiance(worldViewDir, worldNormal, roughness,
                                            metallic, albedo, fragWorldPos, F0);

    // Points lights.
    for(int i = 0; i < kLightNbr; i++)
    {
       Lo += CalculatePointLightRadiance(point_lights[i], worldViewDir, worldNormal,
                                         roughness, metallic, albedo, fragWorldPos, F0);
    }

    vec3 color = Lo + (texture(gEmissive, texCoords).rgb * emissive_factor);

    // check whether result is higher than some threshold, if so, output as bloom threshold color
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));

    if(brightness > 1.0)
        brightColor = vec4(color, 1.0);
    else
        brightColor = vec4(0.0, 0.0, 0.0, 1.0);

    fragColor = vec4(color, 1.0);
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

float ShadowCalculation(vec4 fragPosLightSpace, vec3 worldNormal)
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

    vec3 lightDir = -directional_light.world_direction; //normalize(lightPos - fragPos);
    //float bias = max(0.05 * (1.0 - dot(worldNormal, lightDir)), 0.005);

    float bias = 0.003;
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

float PointLightShadowCalculation(vec3 fragWorldPos, vec3 worldNormal, PointLight point_light) {
    vec3 fragToLight = fragWorldPos - point_light.position;
    float currentDepth = length(fragToLight);
    float closestDepth = texture(shadowCubeMap, fragToLight).r;
    closestDepth *= light_far_plane;
    // get depth of current fragment from light's perspective

    float shadow  = 0.0;
    float bias = 0.003;
    const float samples = 4.0;
    const float offset  = 0.1;
    for(float x = -offset; x < offset; x += offset / (samples * 0.5))
    {
        for(float y = -offset; y < offset; y += offset / (samples * 0.5))
        {
            for(float z = -offset; z < offset; z += offset / (samples * 0.5))
            {
                float closestDepth = texture(shadowCubeMap, fragToLight + vec3(x, y, z)).r;
                closestDepth *= light_far_plane;   // undo mapping [0;1]

                if(currentDepth - bias > closestDepth)
                    shadow += 1.0;
            }
        }
    }
    shadow /= (samples * samples * samples);
    return shadow;
}

vec3 CalculateAmbientLighting(vec3 worldNormal, vec3 worldViewDir, vec3 F0, float roughness,
                              float metallic, vec3 albedo, vec3 reflection, float combined_ao) {
    // Ambient lighting using IBL.
    vec3 F = FresnelSchlickRoughness(max(dot(worldNormal, worldViewDir), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 irradiance = texture(irradianceMap, worldNormal).rgb;
    vec3 diffuse = irradiance * albedo;

    // sample both the pre-filter map and the BRDF lut and combine them together as per the
    // Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, reflection,  roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(worldNormal, worldViewDir), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * combined_ao;

    return ambient;
}

vec3 CalculateDirectionalLightRadiance(vec3 worldViewDir, vec3 worldNormal, float roughness,
                                       float metallic, vec3 albedo, vec4 fragWorldPos, vec3 F0)
{
    vec3 L = normalize(-directional_light.world_direction);
    vec3 H = normalize(worldViewDir + L);

    vec3 radiance = directional_light.color;

    float NDF = DistributionGGX(worldNormal, H, roughness);
    float G   = GeometrySmith(worldNormal, worldViewDir, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, worldViewDir), 0.0), F0);

    // Calculate the Cook-Torrance BRDF.
    vec3 numerator    = NDF * G * F;
    // 0.0001 avoid to divide by 0 if the dot is equal to 0.
    float denominator = 4.0 * max(dot(worldNormal, worldViewDir), 0.0) * max(dot(worldNormal, L), 0.0)  + 0.0001;
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

    float NdotL = max(dot(worldNormal, L), 0.0);

    // Calculate shadow.
    //vec4 fragWorldPos = inverseViewMatrix * vec4(fragWorldPos, 1.0);
    vec4 fragPosLightSpace = lightSpaceMatrix * fragWorldPos;

    float shadow = ShadowCalculation(fragPosLightSpace, worldNormal);

    // Outgoing directional light radiance with shadow.
    return (1.0 - shadow) * (kD * albedo / PI + specular) * radiance * NdotL;
}

vec3 CalculatePointLightRadiance(PointLight point_light, vec3 worldViewDir, vec3 worldNormal,
                                 float roughness, float metallic, vec3 albedo, vec4 fragWorldPos, vec3 F0)
{
    vec3 L = normalize(point_light.position - vec3(fragWorldPos));
    vec3 H = normalize(worldViewDir + L);

    //float distance    = length(point_light.position - vec3(fragWorldPos));
    //float attenuation = 1.0 / (distance * distance);

    // Attenuation
    float distance = length(point_light.position - vec3(fragWorldPos));
    float attenuation = 1.0 / (point_light.constant + point_light.linear * distance +
                        point_light.quadratic * (distance * distance));

    vec3 radiance = point_light.color * attenuation;

    float NDF = DistributionGGX(worldNormal, H, roughness);
    float G   = GeometrySmith(worldNormal, worldViewDir, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, worldViewDir), 0.0), F0);

    // Calculate the Cook-Torrance BRDF.
    vec3 numerator    = NDF * G * F;
    // 0.0001 avoid to divide by 0 if the dot is equal to 0.
    float denominator = 4.0 * max(dot(worldNormal, worldViewDir), 0.0) * max(dot(worldNormal, L), 0.0)  + 0.0001;
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
    float NdotL = max(dot(worldNormal, L), 0.0);

    // Calculate shadow.
    float shadow = PointLightShadowCalculation(vec3(fragWorldPos), worldNormal, point_light);

    return (1.0 - shadow) * (kD * albedo / PI + specular) * radiance * NdotL;
}