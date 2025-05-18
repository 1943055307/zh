#version 330 core
out vec4 FragColor;

in vec3 WorldPos;
in vec3 Normal;

uniform vec3 cameraPos;
uniform sampler2D envMap;
uniform vec3 sh[4];
uniform vec3 rsh[9];
uniform float weight;
uniform float k2[3];

const float PI = 3.14159265359;

vec3 calcIrradianceSH2_mine(vec3 n)
{
    float SH[4];
    SH[0] = 1.0 / (2.0 * sqrt(PI));                                     
    SH[1] = -sqrt(3.0 / (4.0 * PI)) * n.y * (2.0 / 3.0);                
    SH[2] =  sqrt(3.0 / (4.0 * PI)) * n.z * (2.0 / 3.0);
    SH[3] = -sqrt(3.0 / (4.0 * PI)) * n.x * (2.0 / 3.0);

    vec3 result = vec3(0.0);
    for (int i = 0; i < 4; i++) {
        result += rsh[i] * SH[i];
    }
    return result;
}

vec3 calcIrradianceSH3_mine(vec3 n)
{
    float SH[9];
    SH[0] = 1.0 / (2.0 * sqrt(PI));                                     
    SH[1] = -sqrt(3.0 / (4.0 * PI)) * n.y * (2.0 / 3.0);                
    SH[2] =  sqrt(3.0 / (4.0 * PI)) * n.z * (2.0 / 3.0);
    SH[3] = -sqrt(3.0 / (4.0 * PI)) * n.x * (2.0 / 3.0);
    SH[4] =  sqrt(15.0 / (4.0 * PI)) * n.x * n.y * 0.25;                
    SH[5] = -sqrt(15.0 / (4.0 * PI)) * n.y * n.z * 0.25;
    SH[6] =  sqrt(5.0 / (16.0 * PI)) * (3.0 * n.z * n.z - 1.0) * 0.25;
    SH[7] = -sqrt(15.0 / (4.0 * PI)) * n.x * n.z * 0.25;
    SH[8] =  sqrt(15.0 / (16.0 * PI)) * (n.x * n.x - n.y * n.y) * 0.25;

    vec3 result = vec3(0.0);
    for (int i = 0; i < 9; i++) {
        result += rsh[i] * SH[i];
    }
    return result;
}

vec3 calcIrradianceZH3(vec3 n)
{
    vec3 result = vec3(0.0);

    for (int channel = 0; channel < 3; channel++) {
        float sh0 = sh[0][channel];
        float sh1 = sh[1][channel];
        float sh2 = sh[2][channel];
        float sh3 = sh[3][channel];

        float linBasis[4];
        linBasis[0] = 1.0 / (2.0 * sqrt(PI));
        linBasis[1] = -sqrt(3.0 / (4.0 * PI)) * n.y;
        linBasis[2] =  sqrt(3.0 / (4.0 * PI)) * n.z;
        linBasis[3] = -sqrt(3.0 / (4.0 * PI)) * n.x;

        linBasis[1] *= 2.0 / 3.0;
        linBasis[2] *= 2.0 / 3.0;
        linBasis[3] *= 2.0 / 3.0;

        float linCoeff[4] = float[4](sh0, sh1, sh2, sh3);
        float channelIrradiance = 0.0;
        for (int i = 0; i < 4; i++) {
            channelIrradiance += linCoeff[i] * linBasis[i];
        }

        vec3 optDir = normalize(vec3(-sh3, -sh1, sh2));
        float fZ = dot(optDir, n);
        float zhBasis = sqrt(5.0 / (16.0 * PI)) * (3.0 * fZ * fZ - 1.0);

        channelIrradiance += k2[channel] * zhBasis * 0.25;
        result[channel] = channelIrradiance;
    }

    return result;
}

vec3 calcIrradianceShared(vec3 n)
{
    vec3 result = vec3(0.0);
    float RGBWeight[3];
    RGBWeight[0] = 0.2126f;
    RGBWeight[1] = 0.7152f;
    RGBWeight[2] = 0.0722f;
    float sh_3 = 0.0;
    float sh_1 = 0.0;
    float sh_2 = 0.0;
    for (int channel = 0; channel < 3; channel++) {
        sh_3 += sh[3][channel] * RGBWeight[channel];
        sh_1 += sh[1][channel] * RGBWeight[channel];
        sh_2 += sh[2][channel] * RGBWeight[channel];
    }
    vec3 optDir = normalize(vec3(-sh_3, -sh_1, sh_2));
    float fZ = dot(optDir, n);
    float zhBasis = sqrt(5.0 / (16.0 * PI)) * (3.0 * fZ * fZ - 1.0);

    for (int channel = 0; channel < 3; channel++) {
        float sh0 = sh[0][channel];
        float sh1 = sh[1][channel];
        float sh2 = sh[2][channel];
        float sh3 = sh[3][channel];

        float linBasis[4];
        linBasis[0] = 1.0 / (2.0 * sqrt(PI));
        linBasis[1] = -sqrt(3.0 / (4.0 * PI)) * n.y;
        linBasis[2] =  sqrt(3.0 / (4.0 * PI)) * n.z;
        linBasis[3] = -sqrt(3.0 / (4.0 * PI)) * n.x;

        linBasis[1] *= 2.0 / 3.0;
        linBasis[2] *= 2.0 / 3.0;
        linBasis[3] *= 2.0 / 3.0;

        float linCoeff[4] = float[4](sh0, sh1, sh2, sh3);
        float channelIrradiance = 0.0;
        for (int i = 0; i < 4; i++) {
            channelIrradiance += linCoeff[i] * linBasis[i];
        }

        channelIrradiance += k2[channel] * zhBasis * 0.25;
        result[channel] = channelIrradiance;
    }

    return result;
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

    //vec3 irradiance = calcIrradianceZH3(n);
    //vec3 irradiance = calcIrradianceSH3_mine(n);
    //vec3 irradiance = calcIrradianceSH2_mine(n);
    vec3 irradiance = calcIrradianceShared(n);
    vec3 reflection = texture(envMap, angularUV(r)).rgb;
    irradiance *= weight;
    vec3 color = vec3(pow(irradiance, vec3(1.0 / 2.2))) ;
    color *= 0.95;
    color += reflection * 0.05;

    FragColor = vec4(color, 1.0);
}