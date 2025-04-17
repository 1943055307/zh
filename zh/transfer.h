// transfer.h
#pragma once

void computeSHFromFloatFile(const char* filename, int width, float sh[9][3]);
void convertFloatToHDR(const char* floatPath, const char* hdrOutPath, int width);
int guessFloatWidth(const char* path);