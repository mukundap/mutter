#ifndef META_KMS_COLOR_UTILITY_H
#define META_KMS_COLOR_UTILITY_H

#include <stdint.h>
#include <stdio.h>
#include <math.h>

typedef struct
{
    double x;           // CIE1931 x
    double y;           // CIE1931 y
    double luminance;   // CIE1931 Y
}Chromaticity;

typedef struct
{
    Chromaticity white;
    Chromaticity red;
    Chromaticity green;
    Chromaticity blue;
}ColorSpace;

//LUT related structures:
typedef struct{
        int  nSamples;
        double maxVal;
        double *pLutData;
}OneDLUT;

extern void create_unity_log_lut(uint32_t NumOfSegments, uint32_t *NumEntriesPerSegment, uint32_t *pOutputLut);
extern void GetCTMForSrcToDestColorSpace(ColorSpace src, ColorSpace dst, double ctmResult[3][3]);
#endif //End of META_KMS_COLOR_UTILITY_H
