#version 330 core
out vec4 FragColor;

in vec3 WorldPos;
in vec3 Normal;

uniform vec3 cameraPos;
uniform sampler2D envMap;
uniform vec3 sh[9];
uniform float weight;

const float PI = 3.14159265359;
const float c1 = 0.429043;
const float c2 = 0.511664;
const float c3 = 0.743125;
const float c4 = 0.886227;
const float c5 = 0.247708;

vec3 calcIrradianceSH3_mine(vec3 n)
{
    float PI = 3.14159265359;
    float SH[9];
    SH[0] = 1.0 / (2.0 * sqrt(PI));                                     // l=0
    SH[1] = -sqrt(3.0 / (4.0 * PI)) * n.y * (2.0 / 3.0);                // l=1
    SH[2] =  sqrt(3.0 / (4.0 * PI)) * n.z * (2.0 / 3.0);
    SH[3] = -sqrt(3.0 / (4.0 * PI)) * n.x * (2.0 / 3.0);
    SH[4] =  sqrt(15.0 / (4.0 * PI)) * n.x * n.y * 0.25;                // l=2
    SH[5] = -sqrt(15.0 / (4.0 * PI)) * n.y * n.z * 0.25;
    SH[6] =  sqrt(5.0 / (16.0 * PI)) * (3.0 * n.z * n.z - 1.0) * 0.25;
    SH[7] = -sqrt(15.0 / (4.0 * PI)) * n.x * n.z * 0.25;
    SH[8] =  sqrt(15.0 / (16.0 * PI)) * (n.x * n.x - n.y * n.y) * 0.25;

    vec3 result = vec3(0.0);
    for (int i = 0; i < 9; i++) {
        result += sh[i] * SH[i];
    }
    return result;
}

vec3 calcIrradianceSH3_Sloan(vec3 n)
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

vec3 calcIrradianceZH3(vec3 n)
{
    vec3 result = vec3(0.0);

    for (int channel = 0; channel < 3; channel++) {
        float sh0 = sh[0][channel];
        float sh1 = sh[1][channel];
        float sh2 = sh[2][channel];
        float sh3 = sh[3][channel];
        float sh4 = sh[4][channel];
        float sh5 = sh[5][channel];
        float sh6 = sh[6][channel];
        float sh7 = sh[7][channel];
        float sh8 = sh[8][channel];

        // === Linear part ===
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

        // === ZH part ===
        vec3 optDir = normalize(vec3(-sh3, -sh1, sh2));
        float fZ = dot(optDir, n);
        float zhBasis = sqrt(5.0 / (16.0 * PI)) * (3.0 * fZ * fZ - 1.0);

        // Evaluate Y₂ₘ(optDir) with convolution scale (q vector)
        float q4 = sqrt(15.0 / (4.0 * PI)) * optDir.x * optDir.y;
        float q5 = -sqrt(15.0 / (4.0 * PI)) * optDir.y * optDir.z;
        float q6 = sqrt(5.0 / (16.0 * PI)) * (3.0 * optDir.z * optDir.z - 1.0);
        float q7 = -sqrt(15.0 / (4.0 * PI)) * optDir.x * optDir.z;
        float q8 = sqrt(15.0 / (16.0 * PI)) * (optDir.x * optDir.x - optDir.y * optDir.y);

        // Compute dot(q, f₂) = k₂ (projection onto ZH l=2 band)
        float k2 = q4 * sh4 + q5 * sh5 + q6 * sh6 + q7 * sh7 + q8 * sh8;
        k2 *= sqrt(4.0 * PI / 5.0);  // Normalize per ZH projection formula

        // Combine linear + zonal term
        channelIrradiance += k2 * zhBasis * 0.25;
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

    vec3 irradiance = calcIrradianceZH3(n);
    //vec3 irradiance = calcIrradianceSH3_mine(n);
    //vec3 irradiance = calcIrradianceSH3_Sloan(n);
    vec3 reflection = texture(envMap, angularUV(r)).rgb;
    irradiance *= weight;
    vec3 color = vec3(pow(irradiance, vec3(1.0 / 2.2))) ;
    color *= 0.95;
    color += reflection * 0.05;
    //vec3 color = irradiance * 2.25 * 0.5 * 0.95 * 0.333;

    FragColor = vec4(color, 1.0);
}
