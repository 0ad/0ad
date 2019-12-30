// Copyright (c) 2009-2011 Ignacio Castano <castano@gmail.com>
// Copyright (c) 2007-2009 NVIDIA Corporation -- Ignacio Castano <icastano@nvidia.com>
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include "QuickCompressDXT.h"
#include "OptimalCompressDXT.h"

#include "nvimage/ColorBlock.h"
#include "nvimage/BlockDXT.h"

#include "nvmath/Color.inl"
#include "nvmath/Vector.inl"
#include "nvmath/Fitting.h"

#include "nvcore/Utils.h" // swap

#include <string.h> // memset
#include <float.h> // FLT_MAX 

using namespace nv;
using namespace QuickCompress;



inline static void extractColorBlockRGB(const ColorBlock & rgba, Vector3 block[16])
{
	for (int i = 0; i < 16; i++)
	{
		const Color32 c = rgba.color(i);
		block[i] = Vector3(c.r, c.g, c.b);
	}
}

inline static uint extractColorBlockRGBA(const ColorBlock & rgba, Vector3 block[16])
{
	int num = 0;
	
	for (int i = 0; i < 16; i++)
	{
		const Color32 c = rgba.color(i);
		if (c.a > 127)
		{
			block[num++] = Vector3(c.r, c.g, c.b);
		}
	}
	
	return num;
}


// find minimum and maximum colors based on bounding box in color space
inline static void findMinMaxColorsBox(const Vector3 * block, uint num, Vector3 * restrict maxColor, Vector3 * restrict minColor)
{
	*maxColor = Vector3(0, 0, 0);
	*minColor = Vector3(255, 255, 255);
	
	for (uint i = 0; i < num; i++)
	{
		*maxColor = max(*maxColor, block[i]);
		*minColor = min(*minColor, block[i]);
	}
}


inline static void selectDiagonal(const Vector3 * block, uint num, Vector3 * restrict maxColor, Vector3 * restrict minColor)
{
	Vector3 center = (*maxColor + *minColor) * 0.5f;

	Vector2 covariance = Vector2(0.0f);
	for (uint i = 0; i < num; i++)
	{
		Vector3 t = block[i] - center;
		covariance += t.xy() * t.z;
	}

	float x0 = maxColor->x;
	float y0 = maxColor->y;
	float x1 = minColor->x;
	float y1 = minColor->y;
	
	if (covariance.x < 0) {
		swap(x0, x1);
	}
	if (covariance.y < 0) {
		swap(y0, y1);
	}
	
	maxColor->set(x0, y0, maxColor->z);
	minColor->set(x1, y1, minColor->z);
}

inline static void insetBBox(Vector3 * restrict maxColor, Vector3 * restrict minColor)
{
	Vector3 inset = (*maxColor - *minColor) / 16.0f - (8.0f / 255.0f) / 16.0f;
	*maxColor = clamp(*maxColor - inset, 0.0f, 255.0f);
	*minColor = clamp(*minColor + inset, 0.0f, 255.0f);
}

#include "nvmath/ftoi.h"

// Takes a normalized color in [0, 255] range and returns 
inline static uint16 roundAndExpand(Vector3 * restrict v)
{
	uint r = ftoi_floor(clamp(v->x * (31.0f / 255.0f), 0.0f, 31.0f));
	uint g = ftoi_floor(clamp(v->y * (63.0f / 255.0f), 0.0f, 63.0f));
	uint b = ftoi_floor(clamp(v->z * (31.0f / 255.0f), 0.0f, 31.0f));

    float r0 = float(((r+0) << 3) | ((r+0) >> 2));
    float r1 = float(((r+1) << 3) | ((r+1) >> 2));
    if (fabs(v->x - r1) < fabs(v->x - r0)) r = min(r+1, 31U);

    float g0 = float(((g+0) << 2) | ((g+0) >> 4));
    float g1 = float(((g+1) << 2) | ((g+1) >> 4));
    if (fabs(v->y - g1) < fabs(v->y - g0)) g = min(g+1, 63U);

    float b0 = float(((b+0) << 3) | ((b+0) >> 2));
    float b1 = float(((b+1) << 3) | ((b+1) >> 2));
    if (fabs(v->z - b1) < fabs(v->z - b0)) b = min(b+1, 31U);


	uint16 w = (r << 11) | (g << 5) | b;

	r = (r << 3) | (r >> 2);
	g = (g << 2) | (g >> 4);
	b = (b << 3) | (b >> 2);
	*v = Vector3(float(r), float(g), float(b));
	
	return w;
}

// Takes a normalized color in [0, 255] range and returns 
inline static uint16 roundAndExpand01(Vector3 * restrict v)
{
	uint r = ftoi_floor(clamp(v->x * 31.0f, 0.0f, 31.0f));
	uint g = ftoi_floor(clamp(v->y * 63.0f, 0.0f, 63.0f));
	uint b = ftoi_floor(clamp(v->z * 31.0f, 0.0f, 31.0f));

    float r0 = float(((r+0) << 3) | ((r+0) >> 2));
    float r1 = float(((r+1) << 3) | ((r+1) >> 2));
    if (fabs(v->x - r1) < fabs(v->x - r0)) r = min(r+1, 31U);

    float g0 = float(((g+0) << 2) | ((g+0) >> 4));
    float g1 = float(((g+1) << 2) | ((g+1) >> 4));
    if (fabs(v->y - g1) < fabs(v->y - g0)) g = min(g+1, 63U);

    float b0 = float(((b+0) << 3) | ((b+0) >> 2));
    float b1 = float(((b+1) << 3) | ((b+1) >> 2));
    if (fabs(v->z - b1) < fabs(v->z - b0)) b = min(b+1, 31U);


	uint16 w = (r << 11) | (g << 5) | b;

	r = (r << 3) | (r >> 2);
	g = (g << 2) | (g >> 4);
	b = (b << 3) | (b >> 2);
	*v = Vector3(float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f);
	
	return w;
}



inline static float colorDistance(Vector3::Arg c0, Vector3::Arg c1)
{
	return dot(c0-c1, c0-c1);
}

Vector3 round255(const Vector3 & v) {
    //return Vector3(ftoi_round(255 * v.x), ftoi_round(255 * v.y), ftoi_round(255 * v.z)) * (1.0f / 255);
    //return Vector3(floorf(v.x + 0.5f), floorf(v.y + 0.5f), floorf(v.z + 0.5f));
    return v;
}


inline static uint computeIndices4(const Vector3 block[16], Vector3::Arg maxColor, Vector3::Arg minColor)
{
	Vector3 palette[4];
	palette[0] = maxColor;
	palette[1] = minColor;
	//palette[2] = round255((2 * palette[0] + palette[1]) / 3.0f);
	//palette[3] = round255((2 * palette[1] + palette[0]) / 3.0f);
	palette[2] = lerp(palette[0], palette[1], 1.0f / 3.0f);
	palette[3] = lerp(palette[0], palette[1], 2.0f / 3.0f);
	
	uint indices = 0;
	for(int i = 0; i < 16; i++)
	{
		float d0 = colorDistance(palette[0], block[i]);
		float d1 = colorDistance(palette[1], block[i]);
		float d2 = colorDistance(palette[2], block[i]);
		float d3 = colorDistance(palette[3], block[i]);
		
		uint b0 = d0 > d3;
		uint b1 = d1 > d2;
		uint b2 = d0 > d2;
		uint b3 = d1 > d3;
		uint b4 = d2 > d3;
		
		uint x0 = b1 & b2;
		uint x1 = b0 & b3;
		uint x2 = b0 & b4;
		
		indices |= (x2 | ((x0 | x1) << 1)) << (2 * i);
	}

	return indices;
}

// maxColor and minColor are expected to be in the same range as the color set.
/*
inline static uint computeIndices4(const ColorSet & set, Vector3::Arg maxColor, Vector3::Arg minColor)
{
	Vector3 palette[4];
	palette[0] = maxColor;
	palette[1] = minColor;
	palette[2] = lerp(palette[0], palette[1], 1.0f / 3.0f);
	palette[3] = lerp(palette[0], palette[1], 2.0f / 3.0f);
	
    Vector3 mem[(4+2)*2];
    memset(mem, 0, sizeof(mem));

	Vector3 * row0 = mem;
	Vector3 * row1 = mem + (4+2);

	uint indices = 0;
    //for(int i = 0; i < 16; i++)
	for (uint y = 0; y < 4; y++) {
		for (uint x = 0; x < 4; x++) {
            int i = y*4+x;

            if (!set.isValidIndex(i)) {
                // Skip masked pixels and out of bounds.
                continue;
            }

            Vector3 color = set.color(i).xyz();

            // Add error.
            color += row0[1+x];

		    float d0 = colorDistance(palette[0], color);
		    float d1 = colorDistance(palette[1], color);
		    float d2 = colorDistance(palette[2], color);
		    float d3 = colorDistance(palette[3], color);
    		
		    uint b0 = d0 > d3;
		    uint b1 = d1 > d2;
		    uint b2 = d0 > d2;
		    uint b3 = d1 > d3;
		    uint b4 = d2 > d3;
    		
		    uint x0 = b1 & b2;
		    uint x1 = b0 & b3;
		    uint x2 = b0 & b4;

            int index = x2 | ((x0 | x1) << 1);
		    indices |= index << (2 * i);

		    // Compute new error.
		    Vector3 diff = color - palette[index];
            
		    // Propagate new error.
		    //row0[1+x+1] += 7.0f / 16.0f * diff;
		    //row1[1+x-1] += 3.0f / 16.0f * diff;
		    //row1[1+x+0] += 5.0f / 16.0f * diff;
		    //row1[1+x+1] += 1.0f / 16.0f * diff;
        }

		swap(row0, row1);
		memset(row1, 0, sizeof(Vector3) * (4+2));
	}

	return indices;
}*/

inline static float evaluatePaletteError4(const Vector3 block[16], Vector3::Arg maxColor, Vector3::Arg minColor)
{
	Vector3 palette[4];
	palette[0] = maxColor;
	palette[1] = minColor;
	//palette[2] = round255((2 * palette[0] + palette[1]) / 3.0f);
	//palette[3] = round255((2 * palette[1] + palette[0]) / 3.0f);
	palette[2] = lerp(palette[0], palette[1], 1.0f / 3.0f);
	palette[3] = lerp(palette[0], palette[1], 2.0f / 3.0f);
	
	float total = 0.0f;
	for (int i = 0; i < 16; i++)
	{
		float d0 = colorDistance(palette[0], block[i]);
		float d1 = colorDistance(palette[1], block[i]);
		float d2 = colorDistance(palette[2], block[i]);
		float d3 = colorDistance(palette[3], block[i]);

		total += min(min(d0, d1), min(d2, d3));
	}

	return total;
}

inline static float evaluatePaletteError3(const Vector3 block[16], Vector3::Arg maxColor, Vector3::Arg minColor)
{
	Vector3 palette[4];
	palette[0] = minColor;
	palette[1] = maxColor;
	palette[2] = (palette[0] + palette[1]) * 0.5f;
	palette[3] = Vector3(0);
	
	float total = 0.0f;
	for (int i = 0; i < 16; i++)
	{
		float d0 = colorDistance(palette[0], block[i]);
		float d1 = colorDistance(palette[1], block[i]);
		float d2 = colorDistance(palette[2], block[i]);
		//float d3 = colorDistance(palette[3], block[i]);

		//total += min(min(d0, d1), min(d2, d3));
        total += min(min(d0, d1), d2);
	}

	return total;
}


// maxColor and minColor are expected to be in the same range as the color set.
/*inline static uint computeIndices3(const ColorSet & set, Vector3::Arg maxColor, Vector3::Arg minColor)
{
	Vector3 palette[4];
	palette[0] = minColor;
	palette[1] = maxColor;
	palette[2] = (palette[0] + palette[1]) * 0.5f;
	
	uint indices = 0;
	for(int i = 0; i < 16; i++)
	{
        if (!set.isValidIndex(i)) {
            // Skip masked pixels and out of bounds.
            indices |= 3 << (2 * i);
            continue;
        }

        Vector3 color = set.color(i).xyz();
		
		float d0 = colorDistance(palette[0], color);
		float d1 = colorDistance(palette[1], color);
		float d2 = colorDistance(palette[2], color);
		
		uint index;
		if (d0 < d1 && d0 < d2) index = 0;
		else if (d1 < d2) index = 1;
		else index = 2;
		
		indices |= index << (2 * i);
	}

	return indices;
}*/

inline static uint computeIndices3(const Vector3 block[16], Vector3::Arg maxColor, Vector3::Arg minColor)
{
	Vector3 palette[4];
	palette[0] = minColor;
	palette[1] = maxColor;
	palette[2] = (palette[0] + palette[1]) * 0.5f;
	
	uint indices = 0;
	for(int i = 0; i < 16; i++)
	{
		float d0 = colorDistance(palette[0], block[i]);
		float d1 = colorDistance(palette[1], block[i]);
		float d2 = colorDistance(palette[2], block[i]);
		
		uint index;
		if (d0 < d1 && d0 < d2) index = 0;
		else if (d1 < d2) index = 1;
		else index = 2;
		
		indices |= index << (2 * i);
	}

	return indices;
}




static void optimizeEndPoints4(Vector3 block[16], BlockDXT1 * dxtBlock)
{
	float alpha2_sum = 0.0f;
	float beta2_sum = 0.0f;
	float alphabeta_sum = 0.0f;
	Vector3 alphax_sum(0.0f);
	Vector3 betax_sum(0.0f);
	
	for( int i = 0; i < 16; ++i )
	{
		const uint bits = dxtBlock->indices >> (2 * i);
		
		float beta = float(bits & 1);
		if (bits & 2) beta = (1 + beta) / 3.0f;
		float alpha = 1.0f - beta;
		
		alpha2_sum += alpha * alpha;
		beta2_sum += beta * beta;
		alphabeta_sum += alpha * beta;
		alphax_sum += alpha * block[i];
		betax_sum += beta * block[i];
	}

	float denom = alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum;
	if (equal(denom, 0.0f)) return;
	
	float factor = 1.0f / denom;
	
	Vector3 a = (alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor;
	Vector3 b = (betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor;

	a = clamp(a, 0, 255);
	b = clamp(b, 0, 255);
	
	uint16 color0 = roundAndExpand(&a);
	uint16 color1 = roundAndExpand(&b);

	if (color0 < color1)
	{
		swap(a, b);
		swap(color0, color1);
	}

	dxtBlock->col0 = Color16(color0);
	dxtBlock->col1 = Color16(color1);
	dxtBlock->indices = computeIndices4(block, a, b);
}

static void optimizeEndPoints3(Vector3 block[16], BlockDXT1 * dxtBlock)
{
	float alpha2_sum = 0.0f;
	float beta2_sum = 0.0f;
	float alphabeta_sum = 0.0f;
	Vector3 alphax_sum(0.0f);
	Vector3 betax_sum(0.0f);
	
	for( int i = 0; i < 16; ++i )
	{
		const uint bits = dxtBlock->indices >> (2 * i);

		float beta = float(bits & 1);
		if (bits & 2) beta = 0.5f;
		float alpha = 1.0f - beta;

		alpha2_sum += alpha * alpha;
		beta2_sum += beta * beta;
		alphabeta_sum += alpha * beta;
		alphax_sum += alpha * block[i];
		betax_sum += beta * block[i];
	}

	float denom = alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum;
	if (equal(denom, 0.0f)) return;
	
	float factor = 1.0f / denom;
	
	Vector3 a = (alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor;
	Vector3 b = (betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor;

	a = clamp(a, 0, 255);
	b = clamp(b, 0, 255);
	
	uint16 color0 = roundAndExpand(&a);
	uint16 color1 = roundAndExpand(&b);

	if (color0 < color1)
	{
		swap(a, b);
		swap(color0, color1);
	}

	dxtBlock->col0 = Color16(color1);
	dxtBlock->col1 = Color16(color0);
	dxtBlock->indices = computeIndices3(block, a, b);
}

namespace
{

	static uint computeAlphaIndices(const AlphaBlock4x4 & src, AlphaBlockDXT5 * block)
	{
		uint8 alphas[8];
		block->evaluatePalette(alphas, false); // @@ Use target decoder.

		uint totalError = 0;

		for (uint i = 0; i < 16; i++)
		{
			uint8 alpha = src.alpha[i];

			uint besterror = 256*256;
			uint best = 8;
			for(uint p = 0; p < 8; p++)
			{
				int d = alphas[p] - alpha;
				uint error = d * d;

				if (error < besterror)
				{
					besterror = error;
					best = p;
				}
			}
			nvDebugCheck(best < 8);

			totalError += besterror;
			block->setIndex(i, best);
		}

		return totalError;
	}

	static void optimizeAlpha8(const AlphaBlock4x4 & src, AlphaBlockDXT5 * block)
	{
		float alpha2_sum = 0;
		float beta2_sum = 0;
		float alphabeta_sum = 0;
		float alphax_sum = 0;
		float betax_sum = 0;

		for (int i = 0; i < 16; i++)
		{
			uint idx = block->index(i);
			float alpha;
			if (idx < 2) alpha = 1.0f - idx;
			else alpha = (8.0f - idx) / 7.0f;
			
			float beta = 1 - alpha;

			alpha2_sum += alpha * alpha;
			beta2_sum += beta * beta;
			alphabeta_sum += alpha * beta;
			alphax_sum += alpha * src.alpha[i];
			betax_sum += beta * src.alpha[i];
		}

		const float factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);

		float a = (alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor;
		float b = (betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor;

		uint alpha0 = uint(min(max(a, 0.0f), 255.0f));
		uint alpha1 = uint(min(max(b, 0.0f), 255.0f));

		if (alpha0 < alpha1)
		{
			swap(alpha0, alpha1);

			// Flip indices:
			for (int i = 0; i < 16; i++)
			{
				uint idx = block->index(i);
				if (idx < 2) block->setIndex(i, 1 - idx);
				else block->setIndex(i, 9 - idx);
			}
		}
		else if (alpha0 == alpha1)
		{
			for (int i = 0; i < 16; i++)
			{
				block->setIndex(i, 0);
			}
		}

		block->alpha0 = alpha0;
		block->alpha1 = alpha1;
	}

	/*
	static void optimizeAlpha6(const ColorBlock & rgba, AlphaBlockDXT5 * block)
	{
		float alpha2_sum = 0;
		float beta2_sum = 0;
		float alphabeta_sum = 0;
		float alphax_sum = 0;
		float betax_sum = 0;

		for (int i = 0; i < 16; i++)
		{
			uint8 x = rgba.color(i).a;
			if (x == 0 || x == 255) continue;

			uint bits = block->index(i);
			if (bits == 6 || bits == 7) continue;

			float alpha;
			if (bits == 0) alpha = 1.0f;
			else if (bits == 1) alpha = 0.0f;
			else alpha = (6.0f - block->index(i)) / 5.0f;
			
			float beta = 1 - alpha;

			alpha2_sum += alpha * alpha;
			beta2_sum += beta * beta;
			alphabeta_sum += alpha * beta;
			alphax_sum += alpha * x;
			betax_sum += beta * x;
		}

		const float factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);

		float a = (alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor;
		float b = (betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor;

		uint alpha0 = uint(min(max(a, 0.0f), 255.0f));
		uint alpha1 = uint(min(max(b, 0.0f), 255.0f));

		if (alpha0 > alpha1)
		{
			swap(alpha0, alpha1);
		}

		block->alpha0 = alpha0;
		block->alpha1 = alpha1;
	}
	*/

	static bool sameIndices(const AlphaBlockDXT5 & block0, const AlphaBlockDXT5 & block1)
	{
		const uint64 mask = ~uint64(0xFFFF);
		return (block0.u | mask) == (block1.u | mask);
	}

} // namespace



void QuickCompress::compressDXT1(const ColorBlock & rgba, BlockDXT1 * dxtBlock)
{
	if (rgba.isSingleColor())
	{
		OptimalCompress::compressDXT1(rgba.color(0), dxtBlock);
	}
	else
	{
		// read block
		Vector3 block[16];
		extractColorBlockRGB(rgba, block);

#if 1
		// find min and max colors
		Vector3 maxColor, minColor;
		findMinMaxColorsBox(block, 16, &maxColor, &minColor);
		
		selectDiagonal(block, 16, &maxColor, &minColor);
		
		insetBBox(&maxColor, &minColor);
#else
		float weights[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
		Vector3 cluster[4];
		int count = Compute4Means(16, block, weights, Vector3(1, 1, 1), cluster);

		Vector3 maxColor, minColor;
		float bestError = FLT_MAX;

		for (int i = 1; i < 4; i++)
		{
			for (int j = 0; j < i; j++)
			{
		        uint16 color0 = roundAndExpand(&cluster[i]);
		        uint16 color1 = roundAndExpand(&cluster[j]);

				float error = evaluatePaletteError4(block, cluster[i], cluster[j]);
				if (error < bestError) {
					bestError = error;
					maxColor = cluster[i];
					minColor = cluster[j];
				}
			}
		}
#endif

		uint16 color0 = roundAndExpand(&maxColor);
		uint16 color1 = roundAndExpand(&minColor);

		if (color0 < color1)
		{
			swap(maxColor, minColor);
			swap(color0, color1);
		}

		dxtBlock->col0 = Color16(color0);
		dxtBlock->col1 = Color16(color1);
		dxtBlock->indices = computeIndices4(block, maxColor, minColor);

		optimizeEndPoints4(block, dxtBlock);
	}
}


void QuickCompress::compressDXT1a(const ColorBlock & rgba, BlockDXT1 * dxtBlock)
{
	bool hasAlpha = false;
	
	for (uint i = 0; i < 16; i++)
	{
		if (rgba.color(i).a == 0) {
			hasAlpha = true;
			break;
		}
	}
	
	if (!hasAlpha)
	{
		compressDXT1(rgba, dxtBlock);
	}
	// @@ Handle single RGB, with varying alpha? We need tables for single color compressor in 3 color mode.
	//else if (rgba.isSingleColorNoAlpha()) { ... }
	else 
	{
		// read block
		Vector3 block[16];
		uint num = extractColorBlockRGBA(rgba, block);
		
		// find min and max colors
		Vector3 maxColor, minColor;
		findMinMaxColorsBox(block, num, &maxColor, &minColor);
		
		selectDiagonal(block, num, &maxColor, &minColor);
		
		insetBBox(&maxColor, &minColor);
		
		uint16 color0 = roundAndExpand(&maxColor);
		uint16 color1 = roundAndExpand(&minColor);
		
		if (color0 < color1)
		{
			swap(maxColor, minColor);
			swap(color0, color1);
		}
		
		dxtBlock->col0 = Color16(color1);
		dxtBlock->col1 = Color16(color0);
		dxtBlock->indices = computeIndices3(block, maxColor, minColor);
		
		//	optimizeEndPoints(block, dxtBlock);
	}
}


void QuickCompress::compressDXT3(const ColorBlock & src, BlockDXT3 * dxtBlock)
{
	compressDXT1(src, &dxtBlock->color);
	OptimalCompress::compressDXT3A(src, &dxtBlock->alpha);
}

void QuickCompress::compressDXT5A(const ColorBlock & src, AlphaBlockDXT5 * dst, int iterationCount/*=8*/)
{
    AlphaBlock4x4 tmp;
    tmp.init(src, 3);
    compressDXT5A(tmp, dst, iterationCount);
}

void QuickCompress::compressDXT5A(const AlphaBlock4x4 & src, AlphaBlockDXT5 * dst, int iterationCount/*=8*/)
{
	uint8 alpha0 = 0;
	uint8 alpha1 = 255;
	
	// Get min/max alpha.
	for (uint i = 0; i < 16; i++)
	{
		uint8 alpha = src.alpha[i];
		alpha0 = max(alpha0, alpha);
		alpha1 = min(alpha1, alpha);
	}
	
	AlphaBlockDXT5 block;
	block.alpha0 = alpha0 - (alpha0 - alpha1) / 34;
	block.alpha1 = alpha1 + (alpha0 - alpha1) / 34;
	uint besterror = computeAlphaIndices(src, &block);
	
	AlphaBlockDXT5 bestblock = block;

	for (int i = 0; i < iterationCount; i++)
	{
		optimizeAlpha8(src, &block);
		uint error = computeAlphaIndices(src, &block);
		
		if (error >= besterror)
		{
			// No improvement, stop.
			break;
		}
		if (sameIndices(block, bestblock))
		{
			bestblock = block;
			break;
		}
		
		besterror = error;
		bestblock = block;
	};
	
	// Copy best block to result;
	*dst = bestblock;
}

void QuickCompress::compressDXT5(const ColorBlock & rgba, BlockDXT5 * dxtBlock, int iterationCount/*=8*/)
{
	compressDXT1(rgba, &dxtBlock->color);
	compressDXT5A(rgba, &dxtBlock->alpha, iterationCount);
}



/*void QuickCompress::outputBlock4(const ColorSet & set, const Vector3 & start, const Vector3 & end, BlockDXT1 * block)
{
    Vector3 minColor = start * 255.0f;
    Vector3 maxColor = end * 255.0f;
    uint16 color0 = roundAndExpand(&maxColor);
    uint16 color1 = roundAndExpand(&minColor);

    if (color0 < color1)
    {
        swap(maxColor, minColor);
        swap(color0, color1);
    }

    block->col0 = Color16(color0);
    block->col1 = Color16(color1);
    block->indices = computeIndices4(set, maxColor / 255.0f, minColor / 255.0f);

    //optimizeEndPoints4(set, block);
}

void QuickCompress::outputBlock3(const ColorSet & set, const Vector3 & start, const Vector3 & end, BlockDXT1 * block)
{
    Vector3 minColor = start * 255.0f;
    Vector3 maxColor = end * 255.0f;
    uint16 color0 = roundAndExpand(&minColor);
    uint16 color1 = roundAndExpand(&maxColor);

    if (color0 > color1)
    {
        swap(maxColor, minColor);
        swap(color0, color1);
    }

    block->col0 = Color16(color0);
    block->col1 = Color16(color1);
    block->indices = computeIndices3(set, maxColor / 255.0f, minColor / 255.0f);

    //optimizeEndPoints3(set, block);
}
*/
