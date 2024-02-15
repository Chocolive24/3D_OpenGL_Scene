#version 300 es
precision highp float;

out vec4 fragColor;

in vec2 texCoords;

uniform sampler2D hdrBuffer;
uniform sampler2D bloomBlur;

uniform float bloomStrength; // range (0.03, 0.15) works really well.

void main()
{             
    const float gamma = 2.2;
    vec3 hdrColor = texture(hdrBuffer, texCoords).rgb;
    vec3 bloomColor = texture(bloomBlur, texCoords).rgb;
    vec3 mixed_color = mix(hdrColor, bloomColor, bloomStrength); // linear interpolation;

    // reinhard
    // vec3 result = hdrColor / (hdrColor + vec3(1.0));

    // exposure
    //vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
    // also gamma correct while we're at it
    //result = pow(result, vec3(1.0 / gamma));
    //fragColor = vec4(result, 1.0);

    //fragColor = vec4(mixed_color, 1.0);

    // Narkowicz ACES tone mapping
    mixed_color *= 0.6;
    vec3 mapped = (mixed_color * (2.51f * mixed_color + 0.03f)) /
                  (mixed_color * (2.43f * mixed_color + 0.59f) + 0.14f);
    mapped = clamp(mapped, vec3(0.0), vec3(1.0));

    // gamma correction
    mapped = pow(mapped, vec3(1.0 / gamma));
    fragColor = vec4(mapped, 1.0);
}