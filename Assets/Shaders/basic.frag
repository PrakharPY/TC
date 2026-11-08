#version 330 core

in vec3 FragPosition;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 u_Color;
uniform vec3 u_ViewPosition;

uniform vec3 u_LightDirection;
uniform vec3 u_LightColor;

uniform float u_AmbientStrength;
uniform float u_SpecularStrength;

uniform int u_LightingEnabled;

void main()
{
    // Unlit rendering for grid and coordinate axes
    if (u_LightingEnabled == 0)
    {
        FragColor = vec4(u_Color, 1.0);
        return;
    }

    vec3 normal = normalize(Normal);

    // Ambient lighting
    vec3 ambient =
        u_AmbientStrength * u_LightColor;

    // Diffuse lighting
    vec3 lightDirection =
        normalize(-u_LightDirection);

    float diffuseFactor =
        max(dot(normal, lightDirection), 0.0);

    vec3 diffuse =
        diffuseFactor * u_LightColor;

    // Specular lighting using Blinn-Phong
    vec3 viewDirection =
        normalize(u_ViewPosition - FragPosition);

    vec3 halfwayDirection =
        normalize(lightDirection + viewDirection);

    float specularFactor =
        pow(max(dot(normal, halfwayDirection), 0.0), 32.0);

    vec3 specular =
        u_SpecularStrength * specularFactor * u_LightColor;

    // Final lighting
    vec3 result =
        (ambient + diffuse + specular) * u_Color;

    FragColor = vec4(result, 1.0);
}