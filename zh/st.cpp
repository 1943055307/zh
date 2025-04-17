// ZH3: Quadratic Zonal Harmonics, i3D 2024
// http://torust.me/ZH3.pdf
// Thomas Roughton, Peter-Pike Sloan, Ari Silvennoinen, Michal Iwanicki, and Peter Shirley; Activision Research.
//
// This ShaderToy compares linear SH and quadratic SH irradiance reconstruction
// with both hallucinated and computed/stored ZH3.
//
// Images are: [Linear SH]     [ZH3 (Hallucinated)]    [ZH3 (Hallucinated) (Lum Axis)]              
//             [Quadratic SH]  [ZH3 (Computed)]        [ZH3 (Computed) (Lum Axis)]   
// [Quadratic SH] in the bottom-left is the reference. 
//
// Mouse X (phi) and Y (theta) control the camera direction in world space.
//
// The choice of environment map is given by the 'environment' variable on line 86. 

const bool drawBackground = false; // Whether to draw the SH radiance behind the probes.

// St Peter's Basilica: https://www.pauldebevec.com/Probes/, 
// with SH coefficients from https://www.shadertoy.com/view/Mt23zW.
const vec3 stpeter[9] = vec3[](
    8.0 * vec3( 0.3623915,  0.2624130,  0.2326261 ),
    8.0 * vec3( 0.1759131,  0.1436266,  0.1260569 ),
    8.0 * vec3(-0.0247311, -0.0101254, -0.0010745 ),
    8.0 * vec3( 0.0346500,  0.0223184,  0.0101350 ),
    8.0 * vec3( 0.0198140,  0.0144073,  0.0043987 ),
    8.0 * vec3(-0.0469596, -0.0254485, -0.0117786 ),
    8.0 * vec3(-0.0898667, -0.0760911, -0.0740964 ),
    8.0 * vec3( 0.0050194,  0.0038841,  0.0001374 ),
    8.0 * vec3(-0.0818750, -0.0321501,  0.0033399 ) 
);

// Pisa Courtyard: https://vgl.ict.usc.edu/Data/HighResProbes/
const vec3 pisa[9] = vec3[](
    vec3( 0.732668,  0.610310,  0.585349),
    vec3(-0.506687, -0.417720, -0.429780),
    vec3(-0.031983, -0.252292, -0.411999),
    vec3( 0.103438,  0.139038,  0.167152),
    vec3(-0.102849, -0.141602, -0.169318),
    vec3(-0.038356,  0.214884,  0.408204),
    vec3( 0.119999,  0.129040,  0.123580),
    vec3( 0.009767, -0.113339, -0.185314),
    vec3(-0.017577,  0.000823, -0.014609)
);

// Ennis-Brown House Dining Room: https://vgl.ict.usc.edu/Data/HighResProbes/
const vec3 ennis[9] = vec3[](
    0.2 * vec3( 4.531689,  4.308981,  4.520285),
    0.2 * vec3(-1.057496, -1.336207, -1.825209),
    0.2 * vec3(-6.188428, -6.201931, -6.694056),
    0.2 * vec3(-0.407847, -0.405074, -0.416838),
    0.2 * vec3( 0.179401,  0.180498,  0.212682),
    0.2 * vec3( 2.653277,  2.971460,  3.824151),
    0.2 * vec3( 7.051956,  6.934999,  7.286222),
    0.2 * vec3( 0.795407,  0.720389,  0.719510),
    0.2 * vec3( 0.079124, -0.100926, -0.384239)
);

// Uffizi: https://vgl.ict.usc.edu/Data/HighResProbes/
const vec3 uffizi[9] = vec3[](
    (1.0 / 3.0) * vec3( 3.171716,  3.076861,  3.500972),
    (1.0 / 3.0) * vec3(-3.701456, -3.673000, -4.282978),
    (1.0 / 3.0) * vec3(-0.077917, -0.080867, -0.109681),
    (1.0 / 3.0) * vec3( 0.027043,  0.024597,  0.031224),
    (1.0 / 3.0) * vec3(-0.095452, -0.088531, -0.102552),
    (1.0 / 3.0) * vec3( 0.181852,  0.183784,  0.241100),
    (1.0 / 3.0) * vec3(-0.697054, -0.708923, -0.866909),
    (1.0 / 3.0) * vec3(-0.005508, -0.003777, -0.002297),
    (1.0 / 3.0) * vec3(-3.610548, -3.575409, -4.168227)
);

// A production probe with ZH3 color fringing issues
// if the shared luminance axis isn't used.
// (The fringing is much more prominent when 
// the zonal axis/linear coeffs are solved for).
const vec3 productionProbe[9] = vec3[](
    vec3( 1.063877,  1.005969,  1.139061),
    vec3(-0.171134, -0.102052, -0.025571),
    vec3( 0.004089,  0.148585,  0.437148),
    vec3(-0.358268, -0.343352, -0.371353),
    vec3(-0.277614, -0.279779, -0.313043),
    vec3( 0.514214,  0.427734,  0.360189),
    vec3(-0.084615, -0.073269, -0.032683),
    vec3(-0.362233, -0.387163, -0.451868),
    vec3( 0.149837,  0.170355,  0.221346)
);

const vec3 environment[9] = pisa;

const float PI = 3.141592653589793;

// Luminance of a colour using sRGB coefficients.
float Color_Luminance(vec3 color) {
    return dot(color, vec3(0.2126f, 0.7152f, 0.0722f));
}

// Evaluate linear SH in a direction.
void SH2_InDirection(vec3 direction, out float sh[4]) {
     sh[0] = 0.5 * sqrt(1.0 / PI);
        
     sh[1] = -sqrt(0.75 / PI) * direction.y;
     sh[2] = sqrt(0.75 / PI) * direction.z;
     sh[3] = -sqrt(0.75 / PI) * direction.x;
}

// Evaluate quadratic SH in a direction.
void SH3_InDirection(vec3 direction, out float sh[9]) {
     sh[0] = 0.5 * sqrt(1.0 / PI);
        
     sh[1] = -sqrt(0.75 / PI) * direction.y;
     sh[2] = sqrt(0.75 / PI) * direction.z;
     sh[3] = -sqrt(0.75 / PI) * direction.x;
     
     sh[4] = 0.5 * sqrt(15.0 / PI) * direction.x * direction.y;
     sh[5] = -0.5 * sqrt(15.0 / PI) * direction.y * direction.z;
     sh[6] = 0.25 * sqrt(5.0 / PI) * (-1.0 + 3.0 * direction.z * direction.z);
     sh[7] = -0.5 * sqrt(15.0 / PI) * direction.z * direction.x;
     sh[8] = 0.25 * sqrt(15.0 / PI) * (direction.x * direction.x - direction.y * direction.y);
}

// Convolve linear SH with a normalized cosine lobe to produce an irradiance function.
void SH2_ConvCos(inout float sh[4]) {
    sh[0] *= 1.0;
    
    sh[1] *= 2.0 / 3.0;
    sh[2] *= 2.0 / 3.0;
    sh[3] *= 2.0 / 3.0;
}

// Convolve quadratic SH with a normalized cosine lobe to produce an irradiance function.
void SH3_ConvCos(inout float sh[9]) {
    sh[0] *= 1.0;
    
    sh[1] *= 2.0 / 3.0;
    sh[2] *= 2.0 / 3.0;
    sh[3] *= 2.0 / 3.0;
    
    sh[4] *= 0.25;
    sh[5] *= 0.25;
    sh[6] *= 0.25;
    sh[7] *= 0.25;
    sh[8] *= 0.25;
}

// Dot two linear SH vectors, used for reconstructing c in the direction given by sh.
float SH2_Dot(float c[4], float sh[4]) {
    float result = 0.0;
    
    for (int i = 0; i < 4; i += 1) {
        result += c[i] * sh[i];
    }
    
    return result;
}

// Dot two linear SH vectors, used for reconstructing c in the direction given by sh.
vec3 SH2_Dot(vec3 c[4], float sh[4]) {
    vec3 result = vec3(0.0, 0.0, 0.0);
    
    for (int i = 0; i < 4; i += 1) {
        result += c[i] * sh[i];
    }
    
    return result;
}

// Dot two quadratic SH vectors, used for reconstructing c in the direction given by sh.
float SH3_Dot(float c[9], float sh[9]) {
    float result = 0.0;
    
    for (int i = 0; i < 9; i += 1) {
        result += c[i] * sh[i];
    }
    
    return result;
}

// Dot two quadratic SH vectors, used for reconstructing c in the direction given by sh.
vec3 SH3_Dot(vec3 c[9], float sh[9]) {
    vec3 result = vec3(0.0, 0.0, 0.0);
    
    for (int i = 0; i < 9; i += 1) {
        result += c[i] * sh[i];
    }
    
    return result;
}

// Extract the zonal L2 SH coefficient in the direction N. 
float SH_ExtractL2Zonal(float sh[9], vec3 N) {
    float inDirection[9];
    SH3_InDirection(N, inDirection);
    
    float s = 0.0;
    for (int i = 4; i < 9; i += 1) {
        s += inDirection[i] * sh[i];
    }
    
    s /= 0.5 * sqrt(5.0 / PI);

    return s;
}

// Extract the zonal L2 SH coefficient in the direction N.
vec3 SH_ExtractL2Zonal(vec3 sh[9], vec3 N) {
    float inDirection[9];
    SH3_InDirection(N, inDirection);
    
    vec3 s = vec3(0.0);
    for (int i = 4; i < 9; i += 1) {
        s += inDirection[i] * sh[i];
    }
    
    s /= 0.5 * sqrt(5.0 / PI);

    return s;
}

// Evaluate irradiance in direction nor from the linear SH env.
float SH2_EvalIrradiance(float env[4], vec3 nor) {
    float shDir[4];
    SH2_InDirection(nor, shDir);
    
    SH2_ConvCos(shDir);
    return SH2_Dot(env, shDir);
}

// Evaluate irradiance in direction nor from the linear SH env.
vec3 SH2_EvalIrradiance(vec3 env[4], vec3 nor) {
    float shDir[4];
    SH2_InDirection(nor, shDir);
    
    SH2_ConvCos(shDir);
    return SH2_Dot(env, shDir);
}

// Evaluate radiance in direction nor from the quadratic SH env.
vec3 SH3_EvalRadiance(vec3 env[9], vec3 nor) {
    float shDir[9];
    SH3_InDirection(nor, shDir);
    
    return SH3_Dot(env, shDir);
}

// Evaluate irradiance in direction nor from the quadratic SH env.
vec3 SH3_EvalIrradiance(vec3 env[9], vec3 nor) {
    float shDir[9];
    SH3_InDirection(nor, shDir);
    
    SH3_ConvCos(shDir);
    return SH3_Dot(env, shDir);
}

// Evaluate irradiance in direction normal from the quadratic SH sh,
// extracting the ZH3 coefficient and then using that and linear SH
// for reconstruction.
float ZH3_EvalIrradiance(float sh[9], vec3 normal) {
    vec3 zonalAxis = normalize(vec3(-sh[3], -sh[1], sh[2]));
    
    float zonalL2Coeff = SH_ExtractL2Zonal(sh, zonalAxis); // Usually this would be passed in instead of the quadratic SH.
    
    float fZ = dot(zonalAxis, normal);
    float zhNormal = sqrt(5.0f / (16.0f * PI)) * (3.0f * fZ * fZ - 1.0f);

    float linearSH[4] = float[](sh[0], sh[1], sh[2], sh[3]);
    float result = SH2_EvalIrradiance(linearSH, normal);
    result += 0.25f * zhNormal * zonalL2Coeff;
    return result;
}

// Evaluate irradiance in direction normal from the quadratic SH sh,
// extracting the ZH3 coefficient and then using that and linear SH
// for reconstruction.
vec3 ZH3_EvalIrradiance(vec3 sh[9], vec3 normal) {
    vec3 result = vec3(0.0);
    for (int c = 0; c < 3; c += 1) {
        float shChannel[9];
        for (int i = 0; i < 9; i += 1) {
            shChannel[i] = sh[i][c];
        }
        
        result[c] = ZH3_EvalIrradiance(shChannel, normal);
    }
    return result;
}

// Evaluate irradiance in direction normal from the quadratic SH sh,
// computing a shared luminance axis from the linear components,
// extracting the ZH3 coefficienst along that axis,
// and then using ZH3 and linear SH for reconstruction in the direction normal.
vec3 ZH3_EvalIrradianceLumAxis(vec3 sh[9], vec3 normal) {
    vec3 zonalAxis = normalize(vec3(-Color_Luminance(sh[3]), -Color_Luminance(sh[1]), Color_Luminance(sh[2])));
    
    vec3 zonalL2Coeff = SH_ExtractL2Zonal(sh, zonalAxis); // Usually this would be passed in instead of the quadratic SH.
    
    float fZ = dot(zonalAxis, normal);
    float zhNormal = sqrt(5.0f / (16.0f * PI)) * (3.0f * fZ * fZ - 1.0f);

    vec3 linearSH[4] = vec3[](sh[0], sh[1], sh[2], sh[3]);
    vec3 result = SH2_EvalIrradiance(linearSH, normal);
    result += 0.25f * zhNormal * zonalL2Coeff;
    return result;
}


// Evaluate irradiance in direction normal from the linear SH sh,
// hallucinating the ZH3 coefficient and then using that and linear SH
// for reconstruction.
float ZH3Hallucinate_EvalIrradiance(float sh[4], vec3 normal) {
    vec3 zonalAxis = vec3(-sh[3], -sh[1], sh[2]);
    float l1Length = length(zonalAxis);
    zonalAxis /= l1Length;

    float ratio = l1Length / sh[0];
	float zonalL2Coeff = sh[0] * ratio * (0.08 + 0.6 * ratio); // Curve-fit.
    
    float fZ = dot(zonalAxis, normal);
    float zhNormal = sqrt(5.0f / (16.0f * PI)) * (3.0f * fZ * fZ - 1.0f);
    
    float result = SH2_EvalIrradiance(sh, normal);
    result += 0.25f * zhNormal * zonalL2Coeff;
    return result;
}

// Evaluate irradiance in direction normal from the linear SH sh,
// hallucinating the ZH3 coefficient and then using that and linear SH
// for reconstruction.
vec3 ZH3Hallucinate_EvalIrradiance(vec3 sh[4], vec3 normal) {
    vec3 result = vec3(0.0);
    result.r = ZH3Hallucinate_EvalIrradiance(float[](sh[0].r, sh[1].r, sh[2].r, sh[3].r), normal);
    result.g = ZH3Hallucinate_EvalIrradiance(float[](sh[0].g, sh[1].g, sh[2].g, sh[3].g), normal);
    result.b = ZH3Hallucinate_EvalIrradiance(float[](sh[0].b, sh[1].b, sh[2].b, sh[3].b), normal);
    return result;
}

// Evaluate irradiance in direction normal from the linear SH sh,
// computing a shared luminance axis from the linear components,
// hallucinating the ZH3 coefficients along that axis,
// and then using ZH3 and linear SH for reconstruction in the direction normal.
vec3 ZH3Hallucinate_EvalIrradianceLumAxis(vec3 sh[4], vec3 normal) {
    vec3 zonalAxis = normalize(vec3(-Color_Luminance(sh[3]), -Color_Luminance(sh[1]), Color_Luminance(sh[2])));

    vec3 ratio = vec3(0.0);
    ratio.r = dot(vec3(-sh[3].r, -sh[1].r, sh[2].r), zonalAxis);
    ratio.g = dot(vec3(-sh[3].g, -sh[1].g, sh[2].g), zonalAxis);
    ratio.b = dot(vec3(-sh[3].b, -sh[1].b, sh[2].b), zonalAxis);
    ratio /= sh[0];
    ratio = abs(ratio);

	vec3 zonalL2Coeff = sh[0] * ratio * (0.08 + 0.6 * ratio); // Curve-fit.
    
    float fZ = dot(zonalAxis, normal);
    float zhNormal = sqrt(5.0f / (16.0f * PI)) * (3.0f * fZ * fZ - 1.0f);

    vec3 result = SH2_EvalIrradiance(sh, normal);
    result += 0.25f * zhNormal * zonalL2Coeff;
    return result;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    fragCoord.y = iResolution.y - fragCoord.y;
    
    float phi = 2.0 * PI * iMouse.x / iResolution.x - PI;
    float theta = clamp(PI * iMouse.y / iResolution.y, 0.001, PI - 0.001);
    float sinTheta = sin(theta);
    vec3 forward = vec3(sinTheta * cos(phi), cos(theta), sinTheta * sin(phi));
    
    vec3 right = normalize(cross(forward, vec3(0, 1, 0)));
    vec3 up = cross(right, forward);
    mat3 cam = mat3(right, up, forward);

    vec2 p = (2.0 * fragCoord.xy - iResolution.xy) / iResolution.y;
    vec3 rd = cam * normalize(vec3(p.x, -p.y, 1.0));   
    vec3 color = drawBackground ? SH3_EvalRadiance(environment, rd) : vec3(0.0);

    // Boxes.
    float boxSize = min(iResolution.x / 3.0, iResolution.y / 2.0);
    const float sphereRadius = 0.47;
    const float sphereRadiusSq = sphereRadius * sphereRadius;
    
    vec2 margin = 0.5 * vec2(iResolution.x - 3.0 * boxSize, iResolution.y - 2.0 * boxSize);
    vec2 coord = fragCoord - margin;
    
    float boxX = floor(coord.x / boxSize);
    float boxY = floor(coord.y / boxSize);
    coord.x -= boxX * boxSize;
    coord.y -= boxY * boxSize;
    
    float boxNumber = 2.0 * boxX + boxY;
    if (clamp(boxX, 0.0, 2.0) != boxX || clamp(boxY, 0.0, 1.0) != boxY) {
        boxNumber = -1.0;
    }
   
    vec2 uv = coord / boxSize; 
    vec2 spherePoint = (uv - 0.5) / sphereRadius;

    float rSq = dot(spherePoint, spherePoint);

    if (rSq <= 1.0) {
        vec3 localNormal = vec3(spherePoint.x, -spherePoint.y, -sqrt(1.0 - rSq));
        vec3 worldNormal = cam * localNormal;
        
        if (boxNumber == 0.0) {
            // Top-left: linear SH.
            vec3 linearEnv[4] = vec3[](environment[0], environment[1], environment[2], environment[3]);
            color = SH2_EvalIrradiance(linearEnv, worldNormal);
        } else if (boxNumber == 1.0) {
            // Bottom-left: quadratic SH.
            color = SH3_EvalIrradiance(environment, worldNormal);
        } else if (boxNumber == 2.0) {
            // Top-centre: ZH3 hallucinate.
            vec3 linearEnv[4] = vec3[](environment[0], environment[1], environment[2], environment[3]);
            color = ZH3Hallucinate_EvalIrradiance(linearEnv, worldNormal);
        }  else if (boxNumber == 3.0) {
            // Bottom-centre: ZH3.
            color = ZH3_EvalIrradiance(environment, worldNormal);
        } else if (boxNumber == 4.0) {
            // Top-right: ZH3 hallucinate lum axis.
            vec3 linearEnv[4] = vec3[](environment[0], environment[1], environment[2], environment[3]);
            color = ZH3Hallucinate_EvalIrradianceLumAxis(linearEnv, worldNormal);
        } else if (boxNumber == 5.0) {
            // Bottom-right: ZH3 lum axis.
            color = ZH3_EvalIrradianceLumAxis(environment, worldNormal);
        } 
    }

	fragColor = vec4(pow(color, vec3(1.0 / 2.2)), 1.0);
}