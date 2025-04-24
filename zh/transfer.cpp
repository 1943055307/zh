#define _CRT_SECURE_NO_WARNINGS
#include "transfer.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define PI 3.141593
#define MAXWIDTH 2000 

static float hdr[MAXWIDTH][MAXWIDTH][3]; 
static float coeffs[9][3]; 

static float sinc(float x) {
    return (fabs(x) < 1.0e-4f) ? 1.0f : sinf(x) / x;
}

static void input(const char* filename, const int width) {
    FILE* fp = fopen(filename, "rb");
    assert(fp && "HDR .float file not found or unreadable.");
    for (int i = 0; i < width; ++i)
        for (int j = 0; j < width; ++j)
            for (int k = 0; k < 3; ++k) { 
                float val;
                unsigned char* c = (unsigned char*)&val;
                assert(fscanf(fp, "%c%c%c%c", &c[0], &c[1], &c[2], &c[3]) == 4); 
                hdr[i][j][k] = val;
            }
    fclose(fp);
}

static void updatecoeffs(float hdr[3], float domega, float x, float y, float z) {
    int col;
    for (col = 0; col < 3; ++col) { 
        coeffs[0][col] += hdr[col] * 0.282095f * domega; 
        coeffs[1][col] += hdr[col] * 0.488603f * y * domega; 
        coeffs[2][col] += hdr[col] * 0.488603f * z * domega; 
        coeffs[3][col] += hdr[col] * 0.488603f * x * domega; 
        coeffs[4][col] += hdr[col] * 1.092548f * x * y * domega; 
        coeffs[5][col] += hdr[col] * 1.092548f * y * z * domega; 
        coeffs[6][col] += hdr[col] * 0.315392f * (3 * z * z - 1) * domega; 
        coeffs[7][col] += hdr[col] * 1.092548f * x * z * domega; 
        coeffs[8][col] += hdr[col] * 0.546274f * (x * x - y * y) * domega; 
    }
}

void computeSHFromFloatFile(const char* filename, int width, float sh[9][3]) {
    memset(coeffs, 0, sizeof(coeffs)); 
    input(filename, width); 
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < width; ++j) {
            float u = (j - width / 2.0f) / (width / 2.0f);
            float v = (width / 2.0f - i) / (width / 2.0f);
            float r = sqrtf(u * u + v * v);
            if (r > 1.0f) continue; 
            float theta = PI * r; 
            float phi = atan2f(v, u); 
            float x = sinf(theta) * cosf(phi); 
            float y = sinf(theta) * sinf(phi);
            float z = cosf(theta);
            float domega = (2 * PI / width) * (2 * PI / width) * sinc(theta); 
            updatecoeffs(hdr[i][j], domega, x, y, z); 
        }
    }
    memcpy(sh, coeffs, sizeof(coeffs)); 
}

void convertFloatToHDR(const char* floatPath, const char* hdrOutPath, int width) {
    input(floatPath, width); 
    float* linearRGB = (float*)malloc(sizeof(float) * width * width * 3); 

    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < width; ++j) {
            int index = 3 * (i * width + j); 
            linearRGB[index + 0] = hdr[i][j][0]; 
            linearRGB[index + 1] = hdr[i][j][1]; 
            linearRGB[index + 2] = hdr[i][j][2]; 
        }
    }

    stbi_write_hdr(hdrOutPath, width, width, 3, linearRGB); 
    free(linearRGB); 
}

int guessFloatWidth(const char* path) {
    FILE* fp = fopen(path, "rb");
    if (!fp) return -1; 
    fseek(fp, 0, SEEK_END); 
    long size = ftell(fp); 
    fclose(fp);

    long pixelCount = size / (sizeof(float) * 3); 
    int width = (int)sqrt((double)pixelCount); 
    if (width * width * 3 * sizeof(float) != size) { 
        printf("Invalid .float file format or not square.\n");
        return -1;
    }
    return width; 
}
