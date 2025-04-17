#version 330 core
out vec4 FragColor;

in vec3 WorldPos;
in vec3 Normal;

uniform vec3 cameraPos;
uniform sampler2D envMap;
uniform vec3 sh[9];

const float c1 = 0.429043;
const float c2 = 0.511664;
const float c3 = 0.743125;
const float c4 = 0.886227;
const float c5 = 0.247708;

vec3 calcIrradiance(vec3 n)
{
    return (
        c1 * sh[8] * (n.x * n.x - n.y * n.y) +
        c3 * sh[6] * n.z * n.z +
        c4 * sh[0] -
        c5 * sh[6] +
        2.0 * c1 * sh[4] * n.x * n.y +
        2.0 * c1 * sh[7] * n.x * n.z +
        2.0 * c1 * sh[5] * n.y * n.z +
        2.0 * c2 * sh[3] * n.x +
        2.0 * c2 * sh[1] * n.y +
        2.0 * c2 * sh[2] * n.z
    );
}

vec2 angularUV(vec3 dir)
{
    dir = normalize(dir);
    float m = 2.0 * sqrt(dir.x * dir.x + dir.y * dir.y + (dir.z + 1.0) * (dir.z + 1.0));
    if (m < 1e-5) return vec2(0.5, 0.5);
    return dir.xy / m + 0.5;
}

void main()
{
    vec3 n = normalize(Normal);
    vec3 v = normalize(cameraPos - WorldPos);
    vec3 r = reflect(-v, n);

    vec3 irradiance = calcIrradiance(n);
    vec3 reflection = texture(envMap, angularUV(r)).rgb;
    vec3 color = irradiance * 2.25 * 0.5 * 0.95 + reflection * 0.05;

    FragColor = vec4(color, 1.0);
}
