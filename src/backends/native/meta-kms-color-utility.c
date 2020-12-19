#include <glib-object.h>
#include "backends/native/meta-kms-color-utility.h"
#include "backends/native/meta-kms.h"

#define DD_MIN(a, b) ((a) < (b) ? (a) : (b))
#define DD_MAX(a, b) ((a) < (b) ? (b) : (a))
#define MAX_24BIT_NUM ((1<<24) -1)
void create_unity_log_lut(uint32_t NumOfSegments, uint32_t *NumEntriesPerSegment, uint32_t *pOutputLut)
{
	uint32_t temp, index = 0;
	pOutputLut[index++] = 0;
	for (int segment=0; segment < NumOfSegments; segment++)
	{
	        uint32_t entry_count = NumEntriesPerSegment[segment];
	        uint32_t start = (1 << (segment - 1));
	        uint32_t end = (1 << segment);
	        for (uint32_t entry = 1; entry <= entry_count; entry++)
	        {
	                temp = start + entry * ((end - start) * 1.0 / entry_count);
	                pOutputLut[index] = DD_MIN(temp, MAX_24BIT_NUM);
	                index++;
	        }
	}
	g_warning("Sameer: Linear LUTs:\n");
	meta_topic(META_DEBUG_KMS, "Sameer: Linear LUTs:\n");
        for (int i=0; i<513; i++)
        {
                meta_topic(META_DEBUG_KMS, "%d, ", pOutputLut[i]);
        }
        meta_topic(META_DEBUG_KMS, "ALL LUTS OK?\n");
}

double MatrixDeterminant3x3(double matrix[3][3])
{
        double result;
        result = matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]);
        result -= matrix[0][1] * (matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0]);
        result += matrix[0][2] * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]);

        return result;
}

int MatrixInverse3x3(double matrix[3][3], double result[3][3])
{
        int retVal = -1;
        double tmp[3][3];
        double determinant = MatrixDeterminant3x3(matrix);
        if(determinant)
        {
        tmp[0][0] = (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]) / determinant;
        tmp[0][1] = (matrix[0][2] * matrix[2][1] - matrix[2][2] * matrix[0][1]) / determinant;
        tmp[0][2] = (matrix[0][1] * matrix[1][2] - matrix[0][2] * matrix[1][1]) / determinant;
        tmp[1][0] = (matrix[1][2] * matrix[2][0] - matrix[1][0] * matrix[2][2]) / determinant;
        tmp[1][1] = (matrix[0][0] * matrix[2][2] - matrix[0][2] * matrix[2][0]) / determinant;
        tmp[1][2] = (matrix[0][2] * matrix[1][0] - matrix[0][0] * matrix[1][2]) / determinant;
        tmp[2][0] = (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]) / determinant;
        tmp[2][1] = (matrix[0][1] * matrix[2][0] - matrix[0][0] * matrix[2][1]) / determinant;
        tmp[2][2] = (matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0]) / determinant;

        result[0][0] = tmp[0][0]; result[0][1] = tmp[0][1]; result[0][2] = tmp[0][2];
        result[1][0] = tmp[1][0]; result[1][1] = tmp[1][1]; result[1][2] = tmp[1][2];
        result[2][0] = tmp[2][0]; result[2][1] = tmp[2][1]; result[2][2] = tmp[2][2];

        retVal = 0;
        }
        return retVal;
}

void MatrixMult3x3With3x1(double matrix1[3][3], double matrix2[3], double result[3])
{
        double tmp[3];
        tmp[0] = matrix1[0][0] * matrix2[0] + matrix1[0][1] * matrix2[1] + matrix1[0][2] * matrix2[2];
        tmp[1] = matrix1[1][0] * matrix2[0] + matrix1[1][1] * matrix2[1] + matrix1[1][2] * matrix2[2];
        tmp[2] = matrix1[2][0] * matrix2[0] + matrix1[2][1] * matrix2[1] + matrix1[2][2] * matrix2[2];

        result[0]=tmp[0];
        result[1]=tmp[1];
        result[2]=tmp[2];

}

void MatrixMult3x3(double matrix1[3][3], double matrix2[3][3], double result[3][3])
{
        double tmp[3][3];
        for(uint8_t y=0; y<3; y++)
        {
                for(uint8_t x=0; x<3; x++)
                {
                        tmp[y][x]= matrix1[y][0] * matrix2[0][x] + matrix1[y][1] * matrix2[1][x] + matrix1[y][2] * matrix2[2][x];
                }

        }
        for(uint8_t y=0; y<3;y++)
        {
                for(uint8_t x=0; x<3; x++)
                {
                        result[y][x] = tmp[y][x];
                }
        }
}

void CreateRGB2XYZMatrix(ColorSpace *pCspace, double rgb2xyz[3][3])
{
/*
   http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
*/
        double XYZsum[3];
        double z[4];
        double XYZw[3];
        Chromaticity* pChroma = &pCspace->white;
        for(uint8_t i=0; i< 4; i++)
        {
                z[i]=1- pChroma[i].x - pChroma[i].y;
        }
        XYZw[0] = pCspace->white.x / pCspace->white.y;
        XYZw[1]=1;
        XYZw[2]=z[0] / pCspace->white.y;

        double xyzrgb[3][3] =
        {
                { pCspace->red.x, pCspace->green.x, pCspace->blue.x },
                { pCspace->red.y, pCspace->green.y, pCspace->blue.y },
                { z[1], z[2], z[3] }
        };
        double mat1[3][3];
        MatrixInverse3x3(xyzrgb, mat1);
        MatrixMult3x3With3x1(mat1, XYZw, XYZsum);
        double mat2[3][3] = { { XYZsum[0], 0, 0 }, { 0, XYZsum[1], 0 }, { 0, 0, XYZsum[2] } };
        MatrixMult3x3(xyzrgb, mat2, rgb2xyz);
}

void CreateGamutScalingMatrix(ColorSpace* pSrc, ColorSpace* pDst, double result[3][3])
{
        double mat1[3][3], mat2[3][3], tmp[3][3];
        CreateRGB2XYZMatrix(pSrc, mat1);
        CreateRGB2XYZMatrix(pDst, mat2);

        MatrixInverse3x3(mat2, tmp);
        MatrixMult3x3(tmp, mat1, result);
}
void GetCTMForSrcToDestColorSpace(ColorSpace srcColorSpace, ColorSpace destColorSpace, double result[3][3])
{
/*
 https://en.wikipedia.org/wiki/Rec._2020#System_colorimetry
 https://en.wikipedia.org/wiki/Rec._709#Primary_chromaticities
*/
CreateGamutScalingMatrix(&srcColorSpace, &destColorSpace, result);
}
#if 0
void GetCTMForBT709ToBT2020(double result[3][3])
{
/*
 https://en.wikipedia.org/wiki/Rec._2020#System_colorimetry
 https://en.wikipedia.org/wiki/Rec._709#Primary_chromaticities
*/
        ColorSpace bt2020, bt709;

        bt2020.white.x = 0.3127;
        bt2020.white.y = 0.3290;
        bt2020.white.luminance = 100.0;
        bt2020.red.x = 0.708;
        bt2020.red.y = 0.292;
        bt2020.green.x = 0.170;
        bt2020.green.y = 0.797;
        bt2020.blue.x = 0.131;
        bt2020.blue.y = 0.046;

        bt709.white.x = 0.3127;
        bt709.white.y = 0.3290;
        bt709.white.luminance = 100.0;
        bt709.red.x = 0.64;
        bt709.red.y = 0.33;
        bt709.green.x = 0.30;
        bt709.green.y = 0.60;
        bt709.blue.x = 0.15;
        bt709.blue.y = 0.06;
        CreateGamutScalingMatrix(&bt709, &bt2020, result);
}

void GetCTMForBT2020ToBT709(double result[3][3])
{
/*
 https://en.wikipedia.org/wiki/Rec._2020#System_colorimetry
 https://en.wikipedia.org/wiki/Rec._709#Primary_chromaticities
*/
        ColorSpace bt2020, bt709;

        bt2020.white.x = 0.3127;
        bt2020.white.y = 0.3290;
        bt2020.white.luminance = 100.0;
        bt2020.red.x = 0.708;
        bt2020.red.y = 0.292;
        bt2020.green.x = 0.170;
        bt2020.green.y = 0.797;
        bt2020.blue.x = 0.131;
        bt2020.blue.y = 0.046;

        bt709.white.x = 0.3127;
        bt709.white.y = 0.3290;
        bt709.white.luminance = 100.0;
        bt709.red.x = 0.64;
        bt709.red.y = 0.33;
        bt709.green.x = 0.30;
        bt709.green.y = 0.60;
        bt709.blue.x = 0.15;
        bt709.blue.y = 0.06;
        CreateGamutScalingMatrix(&bt2020, &bt709, result);
}
#endif
