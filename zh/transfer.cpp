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
#define MAXWIDTH 2000 // Maximum width of the HDR image

// Global buffers for HDR data and SH coefficients
static float hdr[MAXWIDTH][MAXWIDTH][3]; // Stores HDR image data
static float coeffs[9][3]; // Stores SH coefficients for 9 basis functions and 3 color channels

// Computes the sinc function, which is sin(x)/x, with a special case for x close to 0
static float sinc(float x) {
    return (fabs(x) < 1.0e-4f) ? 1.0f : sinf(x) / x;
}

// Reads an HDR .float file and stores its data in the global hdr buffer
static void input(const char* filename, const int width) {
    FILE* fp = fopen(filename, "rb");
    assert(fp && "HDR .float file not found or unreadable.");
    for (int i = 0; i < width; ++i)
        for (int j = 0; j < width; ++j)
            for (int k = 0; k < 3; ++k) { // Each pixel has 3 color channels (RGB)
                float val;
                unsigned char* c = (unsigned char*)&val;
                assert(fscanf(fp, "%c%c%c%c", &c[0], &c[1], &c[2], &c[3]) == 4); // Read 4 bytes (float)
                hdr[i][j][k] = val;
            }
    fclose(fp);
}

// Updates the SH coefficients for a single pixel
static void updatecoeffs(float hdr[3], float domega, float x, float y, float z) {
    int col;
    for (col = 0; col < 3; ++col) { // Iterate over RGB channels
        coeffs[0][col] += hdr[col] * 0.282095f * domega; // SH basis Y_0,0
        coeffs[1][col] += hdr[col] * 0.488603f * y * domega; // SH basis Y_1,-1
        coeffs[2][col] += hdr[col] * 0.488603f * z * domega; // SH basis Y_1,0
        coeffs[3][col] += hdr[col] * 0.488603f * x * domega; // SH basis Y_1,1
        coeffs[4][col] += hdr[col] * 1.092548f * x * y * domega; // SH basis Y_2,-2
        coeffs[5][col] += hdr[col] * 1.092548f * y * z * domega; // SH basis Y_2,-1
        coeffs[6][col] += hdr[col] * 0.315392f * (3 * z * z - 1) * domega; // SH basis Y_2,0
        coeffs[7][col] += hdr[col] * 1.092548f * x * z * domega; // SH basis Y_2,1
        coeffs[8][col] += hdr[col] * 0.546274f * (x * x - y * y) * domega; // SH basis Y_2,2
    }
}

// Computes spherical harmonics (SH) coefficients from an HDR .float file
void computeSHFromFloatFile(const char* filename, int width, float sh[9][3]) {
    memset(coeffs, 0, sizeof(coeffs)); // Initialize coefficients to zero
    input(filename, width); // Load HDR data
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < width; ++j) {
            // Convert pixel coordinates to spherical coordinates
            float u = (j - width / 2.0f) / (width / 2.0f);
            float v = (width / 2.0f - i) / (width / 2.0f);
            float r = sqrtf(u * u + v * v);
            if (r > 1.0f) continue; // Skip pixels outside the unit circle
            float theta = PI * r; // Polar angle
            float phi = atan2f(v, u); // Azimuthal angle
            float x = sinf(theta) * cosf(phi); // Cartesian coordinates
            float y = sinf(theta) * sinf(phi);
            float z = cosf(theta);
            float domega = (2 * PI / width) * (2 * PI / width) * sinc(theta); // Solid angle
            updatecoeffs(hdr[i][j], domega, x, y, z); // Update SH coefficients
        }
    }
    memcpy(sh, coeffs, sizeof(coeffs)); // Copy results to output array
}

// Converts an HDR .float file to a standard HDR image and writes it to disk
void convertFloatToHDR(const char* floatPath, const char* hdrOutPath, int width) {
    input(floatPath, width); // Load HDR data
    float* linearRGB = (float*)malloc(sizeof(float) * width * width * 3); // Allocate memory for linear RGB data

    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < width; ++j) {
            int index = 3 * (i * width + j); // Compute linear index
            linearRGB[index + 0] = hdr[i][j][0]; // Red channel
            linearRGB[index + 1] = hdr[i][j][1]; // Green channel
            linearRGB[index + 2] = hdr[i][j][2]; // Blue channel
        }
    }

    stbi_write_hdr(hdrOutPath, width, width, 3, linearRGB); // Write HDR image to file
    free(linearRGB); // Free allocated memory
}

// Guesses the width of a square HDR .float file based on its size
int guessFloatWidth(const char* path) {
    FILE* fp = fopen(path, "rb");
    if (!fp) return -1; // File not found
    fseek(fp, 0, SEEK_END); // Seek to the end of the file
    long size = ftell(fp); // Get file size in bytes
    fclose(fp);

    long pixelCount = size / (sizeof(float) * 3); // Each pixel has 3 float values (RGB)
    int width = (int)sqrt((double)pixelCount); // Compute width assuming a square image
    if (width * width * 3 * sizeof(float) != size) { // Validate file size
        printf("Invalid .float file format or not square.\n");
        return -1;
    }
    return width; // Return the guessed width
}
