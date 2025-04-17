#version 330 core
out vec4 FragColor;

in vec3 WorldDir;

uniform sampler2D envMap;

vec2 angularMapUV(vec3 dir)
{
    float r = length(dir.xy);
    float theta = acos(clamp(dir.z, -1.0, 1.0));
    float phi = theta / 3.1415926;
    return vec2(0.5) + 0.5 * dir.xy / r * phi;
}

void main()
{
    vec3 dir = normalize(WorldDir);
    vec2 uv = angularMapUV(dir);
    vec3 color = texture(envMap, uv).rgb;
    FragColor = vec4(color, 1.0);
}
