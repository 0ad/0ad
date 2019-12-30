
#include "CompressorDXT1.h"
#include "SingleColorLookup.h"
#include "ClusterFit.h"

#include "nvimage/ColorBlock.h"
#include "nvimage/BlockDXT.h"

#include "nvmath/Color.inl"
#include "nvmath/Vector.inl"
#include "nvmath/Fitting.h"
#include "nvmath/ftoi.h"

#include "nvcore/Utils.h" // swap

#include <string.h> // memset
#include <float.h> // FLT_MAX


using namespace nv;


///////////////////////////////////////////////////////////////////////////////////////////////////
// Color conversion functions.

static const float midpoints5[32] = {
    0.015686f, 0.047059f, 0.078431f, 0.111765f, 0.145098f, 0.176471f, 0.207843f, 0.241176f, 0.274510f, 0.305882f, 0.337255f, 0.370588f, 0.403922f, 0.435294f, 0.466667f, 0.5f,
    0.533333f, 0.564706f, 0.596078f, 0.629412f, 0.662745f, 0.694118f, 0.725490f, 0.758824f, 0.792157f, 0.823529f, 0.854902f, 0.888235f, 0.921569f, 0.952941f, 0.984314f, 1.0f
};

static const float midpoints6[64] = {
    0.007843f, 0.023529f, 0.039216f, 0.054902f, 0.070588f, 0.086275f, 0.101961f, 0.117647f, 0.133333f, 0.149020f, 0.164706f, 0.180392f, 0.196078f, 0.211765f, 0.227451f, 0.245098f, 
    0.262745f, 0.278431f, 0.294118f, 0.309804f, 0.325490f, 0.341176f, 0.356863f, 0.372549f, 0.388235f, 0.403922f, 0.419608f, 0.435294f, 0.450980f, 0.466667f, 0.482353f, 0.500000f, 
    0.517647f, 0.533333f, 0.549020f, 0.564706f, 0.580392f, 0.596078f, 0.611765f, 0.627451f, 0.643137f, 0.658824f, 0.674510f, 0.690196f, 0.705882f, 0.721569f, 0.737255f, 0.754902f, 
    0.772549f, 0.788235f, 0.803922f, 0.819608f, 0.835294f, 0.850980f, 0.866667f, 0.882353f, 0.898039f, 0.913725f, 0.929412f, 0.945098f, 0.960784f, 0.976471f, 0.992157f, 1.0f
};

/*void init_tables() {
    for (int i = 0; i < 31; i++) {
        float f0 = float(((i+0) << 3) | ((i+0) >> 2)) / 255.0f;
        float f1 = float(((i+1) << 3) | ((i+1) >> 2)) / 255.0f;
        midpoints5[i] = (f0 + f1) * 0.5;
    }
    midpoints5[31] = 1.0f;

    for (int i = 0; i < 63; i++) {
        float f0 = float(((i+0) << 2) | ((i+0) >> 4)) / 255.0f;
        float f1 = float(((i+1) << 2) | ((i+1) >> 4)) / 255.0f;
        midpoints6[i] = (f0 + f1) * 0.5;
    }
    midpoints6[63] = 1.0f;
}*/

static Color16 vector3_to_color16(const Vector3 & v) {
    // Truncate.
    uint r = ftoi_trunc(clamp(v.x * 31.0f, 0.0f, 31.0f));
	uint g = ftoi_trunc(clamp(v.y * 63.0f, 0.0f, 63.0f));
	uint b = ftoi_trunc(clamp(v.z * 31.0f, 0.0f, 31.0f));

    // Round exactly according to 565 bit-expansion.
    r += (v.x > midpoints5[r]);
    g += (v.y > midpoints6[g]);
    b += (v.z > midpoints5[b]);

    return Color16((r << 11) | (g << 5) | b);
}


static Color32 bitexpand_color16_to_color32(Color16 c16) {
    Color32 c32;
    //c32.b = (c16.b << 3) | (c16.b >> 2);
    //c32.g = (c16.g << 2) | (c16.g >> 4);
    //c32.r = (c16.r << 3) | (c16.r >> 2);
    //c32.a = 0xFF;

    c32.u = ((c16.u << 3) & 0xf8) | ((c16.u << 5) & 0xfc00) | ((c16.u << 8) & 0xf80000);
    c32.u |= (c32.u >> 5) & 0x070007;
    c32.u |= (c32.u >> 6) & 0x000300;

    return c32;
}

/*static Color32 bitexpand_color16_to_color32(int r, int g, int b) {
    Color32 c32;
    c32.b = (b << 3) | (b >> 2);
    c32.g = (g << 2) | (g >> 4);
    c32.r = (r << 3) | (r >> 2);
    c32.a = 0xFF;
    return c32;
}*/

static Color16 truncate_color32_to_color16(Color32 c32) {
    Color16 c16;
    c16.b = (c32.b >> 3);
    c16.g = (c32.g >> 2);
    c16.r = (c32.r >> 3);
    return c16;
}

/*inline Vector3 r5g6b5_to_vector3(int r, int g, int b)
{
    Vector3 c;
    c.x = float((r << 3) | (r >> 2));
    c.y = float((g << 2) | (g >> 4));
    c.z = float((b << 3) | (b >> 2));
    return c;
}*/

inline Vector3 color_to_vector3(Color32 c)
{
    const float scale = 1.0f / 255.0f;
    return Vector3(c.r * scale, c.g * scale, c.b * scale);
}

inline Color32 vector3_to_color(Vector3 v)
{
    Color32 color;
    color.r = U8(ftoi_round(saturate(v.x) * 255));
    color.g = U8(ftoi_round(saturate(v.y) * 255));
    color.b = U8(ftoi_round(saturate(v.z) * 255));
    color.a = 255;
    return color;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Input block processing.

inline static void color_block_to_vector_block(const ColorBlock & rgba, Vector3 block[16])
{
	for (int i = 0; i < 16; i++)
	{
		const Color32 c = rgba.color(i);
		block[i] = Vector3(c.r, c.g, c.b);
	}
}

// Find first valid color.
static bool find_valid_color_rgb(const Vector3 * colors, const float * weights, int count, Vector3 * valid_color)
{
    for (int i = 0; i < count; i++) {
        if (weights[i] > 0.0f) {
            *valid_color = colors[i];
            return true;
        }
    }

    // No valid colors.
    return false;
}

static bool is_single_color_rgb(const Vector3 * colors, const float * weights, int count, Vector3 color)
{
    for (int i = 0; i < count; i++) {
        if (weights[i] > 0.0f) {
            if (colors[i] != color) return false;
        }
    }

    return true;
}

// Find similar colors and combine them together.
static int reduce_colors(const Vector4 * input_colors, const float * input_weights, Vector3 * colors, float * weights)
{
    int n = 0;
    for (int i = 0; i < 16; i++)
    {
        Vector3 ci = input_colors[i].xyz();
        float wi = input_weights[i];

        if (wi > 0) {
            // Find matching color.
            int j;
            for (j = 0; j < n; j++) {
                if (equal(colors[j].x, ci.x) && equal(colors[j].y, ci.y) && equal(colors[j].z, ci.z)) {
                    weights[j] += wi;
                    break;
                }
            }

            // No match found. Add new color.
            if (j == n) {
                colors[n] = ci;
                weights[n] = wi;
                n++;
            }
        }
    }

    nvDebugCheck(n <= 16);

    return n;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Error evaluation.

// Different ways of estimating the error.
/*static float evaluate_mse(const Vector3 & p, const Vector3 & c) {
    //return (square(p.x-c.x) * w2.x + square(p.y-c.y) * w2.y + square(p.z-c.z) * w2.z);
    Vector3 d = (p - c);
    return dot(d, d);
}*/

static float evaluate_mse(const Vector3 & p, const Vector3 & c, const Vector3 & w) {
    //return (square(p.x-c.x) * w2.x + square(p.y-c.y) * w2.y + square(p.z-c.z) * w2.z);
    Vector3 d = (p - c) * w;
    return dot(d, d);
}

/*static float evaluate_mse(const Vector3 & p, const Vector3 & c, const Vector3 & w) {
    return ww.x * square(p.x-c.x) + ww.y * square(p.y-c.y) + ww.z * square(p.z-c.z);
}*/

static int evaluate_mse(const Color32 & p, const Color32 & c) {
    return (square(int(p.r)-c.r) + square(int(p.g)-c.g) + square(int(p.b)-c.b));
}

static float evaluate_mse(const Vector3 palette[4], const Vector3 & c, const Vector3 & w) {
    float e0 = evaluate_mse(palette[0], c, w);
    float e1 = evaluate_mse(palette[1], c, w);
    float e2 = evaluate_mse(palette[2], c, w);
    float e3 = evaluate_mse(palette[3], c, w);
    return min(min(e0, e1), min(e2, e3));
}

static int evaluate_mse(const Color32 palette[4], const Color32 & c) {
    int e0 = evaluate_mse(palette[0], c);
    int e1 = evaluate_mse(palette[1], c);
    int e2 = evaluate_mse(palette[2], c);
    int e3 = evaluate_mse(palette[3], c);
    return min(min(e0, e1), min(e2, e3));
}

// Returns MSE error in [0-255] range.
static int evaluate_mse(const BlockDXT1 * output, Color32 color, int index) {
    Color32 palette[4];
    output->evaluatePalette(palette, /*d3d9=*/false);

    return evaluate_mse(palette[index], color);
}

// Returns weighted MSE error in [0-255] range.
static float evaluate_palette_error(Color32 palette[4], const Color32 * colors, const float * weights, int count) {
    
	float total = 0.0f;
	for (int i = 0; i < count; i++) {
        total += weights[i] * evaluate_mse(palette, colors[i]);
	}

	return total;
}

#if 0
static float evaluate_mse(const BlockDXT1 * output, const Vector3 colors[16]) {
    Color32 palette[4];
    output->evaluatePalette(palette, /*d3d9=*/false);

    // convert palette to float.
    Vector3 vector_palette[4];
    for (int i = 0; i < 4; i++) {
        vector_palette[i] = color_to_vector3(palette[i]);
    }

    // evaluate error for each index.
    float error = 0.0f;
    for (int i = 0; i < 16; i++) {
        int index = (output->indices >> (2*i)) & 3; // @@ Is this the right order?
        error += evaluate_mse(vector_palette[index], colors[i]);
    }

    return error;
}
#endif

static float evaluate_mse(const Vector4 input_colors[16], const float input_weights[16], const Vector3 & color_weights, const BlockDXT1 * output) {
    Color32 palette[4];
    output->evaluatePalette(palette, /*d3d9=*/false);

    // convert palette to float.
    Vector3 vector_palette[4];
    for (int i = 0; i < 4; i++) {
        vector_palette[i] = color_to_vector3(palette[i]);
    }

    // evaluate error for each index.
    float error = 0.0f;
    for (int i = 0; i < 16; i++) {
        int index = (output->indices >> (2 * i)) & 3;
        error += input_weights[i] * evaluate_mse(vector_palette[index], input_colors[i].xyz(), color_weights);
    }
    return error;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
// Palette evaluation.

static void evaluate_palette4(Color32 palette[4]) {
    palette[2].r = (2 * palette[0].r + palette[1].r) / 3;
    palette[2].g = (2 * palette[0].g + palette[1].g) / 3;
    palette[2].b = (2 * palette[0].b + palette[1].b) / 3;
    palette[3].r = (2 * palette[1].r + palette[0].r) / 3;
    palette[3].g = (2 * palette[1].g + palette[0].g) / 3;
    palette[3].b = (2 * palette[1].b + palette[0].b) / 3;
}

static void evaluate_palette3(Color32 palette[4]) {
    palette[2].r = (palette[0].r + palette[1].r) / 2;
    palette[2].g = (palette[0].g + palette[1].g) / 2;
    palette[2].b = (palette[0].b + palette[1].b) / 2;
    palette[3].r = 0;
    palette[3].g = 0;
    palette[3].b = 0;
}

static void evaluate_palette(Color16 c0, Color16 c1, Color32 palette[4]) {
    palette[0] = bitexpand_color16_to_color32(c0);
    palette[1] = bitexpand_color16_to_color32(c1);
    if (c0.u > c1.u) {
        evaluate_palette4(palette);
    }
    else {
        evaluate_palette3(palette);
    }
}

static void evaluate_palette(Color16 c0, Color16 c1, Vector3 palette[4]) {
    Color32 palette32[4];
    evaluate_palette(c0, c1, palette32);

    for (int i = 0; i < 4; i++) {
        palette[i] = color_to_vector3(palette32[i]);
    }
}

static void evaluate_palette3(Color16 c0, Color16 c1, Vector3 palette[4]) {
    nvDebugCheck(c0.u > c1.u);

    Color32 palette32[4];
    evaluate_palette(c0, c1, palette32);

    for (int i = 0; i < 4; i++) {
        palette[i] = color_to_vector3(palette32[i]);
    }
}





static uint compute_indices4(const Vector4 input_colors[16], const Vector3 & color_weights, const Vector3 palette[4]) {
    
    uint indices = 0;
	for (int i = 0; i < 16; i++) {
		float d0 = evaluate_mse(palette[0], input_colors[i].xyz(), color_weights);
		float d1 = evaluate_mse(palette[1], input_colors[i].xyz(), color_weights);
		float d2 = evaluate_mse(palette[2], input_colors[i].xyz(), color_weights);
		float d3 = evaluate_mse(palette[3], input_colors[i].xyz(), color_weights);
		
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


static uint compute_indices(const Vector4 input_colors[16], const Vector3 & color_weights, const Vector3 palette[4]) {
    
    uint indices = 0;
	for (int i = 0; i < 16; i++) {
		float d0 = evaluate_mse(palette[0], input_colors[i].xyz(), color_weights);
		float d1 = evaluate_mse(palette[1], input_colors[i].xyz(), color_weights);
		float d2 = evaluate_mse(palette[2], input_colors[i].xyz(), color_weights);
		float d3 = evaluate_mse(palette[3], input_colors[i].xyz(), color_weights);
		
        uint index;
        if (d0 < d1 && d0 < d2 && d0 < d3) index = 0;
        else if (d1 < d2 && d1 < d3) index = 1;
        else if (d2 < d3) index = 2;
        else index = 3;

		indices |= index << (2 * i);
	}

	return indices;
}


static void output_block3(const Vector4 input_colors[16], const Vector3 & color_weights, const Vector3 & v0, const Vector3 & v1, BlockDXT1 * block)
{
    Color16 color0 = vector3_to_color16(v0);
    Color16 color1 = vector3_to_color16(v1);

    if (color0.u > color1.u) {
        swap(color0, color1);
    }

    Vector3 palette[4];
    evaluate_palette(color0, color1, palette);

    block->col0 = color0;
    block->col1 = color1;
    block->indices = compute_indices(input_colors, color_weights, palette);
}

static void output_block4(const Vector4 input_colors[16], const Vector3 & color_weights, const Vector3 & v0, const Vector3 & v1, BlockDXT1 * block)
{
    Color16 color0 = vector3_to_color16(v0);
    Color16 color1 = vector3_to_color16(v1);

    if (color0.u < color1.u) {
        swap(color0, color1);
    }

    Vector3 palette[4];
    evaluate_palette(color0, color1, palette);

    block->col0 = color0;
    block->col1 = color1;
    block->indices = compute_indices4(input_colors, color_weights, palette);
}





// Single color compressor, based on:
// https://mollyrocket.com/forums/viewtopic.php?t=392
static void compress_dxt1_single_color_optimal(Color32 c, BlockDXT1 * output)
{
    output->col0.r = OMatch5[c.r][0];
    output->col0.g = OMatch6[c.g][0];
    output->col0.b = OMatch5[c.b][0];
    output->col1.r = OMatch5[c.r][1];
    output->col1.g = OMatch6[c.g][1];
    output->col1.b = OMatch5[c.b][1];
    output->indices = 0xaaaaaaaa;
    
    if (output->col0.u < output->col1.u)
    {
        swap(output->col0.u, output->col1.u);
        output->indices ^= 0x55555555;
    }
}


float nv::compress_dxt1_single_color_optimal(Color32 c, BlockDXT1 * output)
{
    ::compress_dxt1_single_color_optimal(c, output);

    // Multiply by 16^2, the weight associated to a single color.
    // Divide by 255*255 to covert error to [0-1] range.
    return (256.0f / (255*255)) * evaluate_mse(output, c, output->indices & 3);
}


float nv::compress_dxt1_single_color_optimal(const Vector3 & color, BlockDXT1 * output)
{
    return compress_dxt1_single_color_optimal(vector3_to_color(color), output);
}


// Compress block using the average color.
float nv::compress_dxt1_single_color(const Vector3 * colors, const float * weights, int count, const Vector3 & color_weights, BlockDXT1 * output)
{
    // Compute block average.
    Vector3 color_sum(0);
    float weight_sum = 0;

    for (int i = 0; i < count; i++) {
        color_sum += colors[i] * weights[i];
        weight_sum += weights[i];
    }

    // Compress optimally.
    ::compress_dxt1_single_color_optimal(vector3_to_color(color_sum / weight_sum), output);

    // Decompress block color.
    Color32 palette[4];
    output->evaluatePalette(palette, /*d3d9=*/false);

    Vector3 block_color = color_to_vector3(palette[output->indices & 0x3]);

    // Evaluate error.
    float error = 0;
    for (int i = 0; i < count; i++) {
        error += weights[i] * evaluate_mse(block_color, colors[i], color_weights);
    }
    return error;
}


/* @@ Not implemented yet.
// Low quality baseline compressor.
float nv::compress_dxt1_least_squares_fit(const Vector3 * input_colors, const Vector3 * colors, const float * weights, int count, BlockDXT1 * output)
{
    // @@ Iterative best end point fit.

    return FLT_MAX;
}*/


float nv::compress_dxt1_bounding_box_exhaustive(const Vector4 input_colors[16], const Vector3 * colors, const float * weights, int count, const Vector3 & color_weights, bool three_color_mode, int max_volume, BlockDXT1 * output)
{
    // Compute bounding box.
    Vector3 min_color(1.0f);
    Vector3 max_color(0.0f);

    for (int i = 0; i < count; i++) {
        min_color = min(min_color, colors[i]);
        max_color = max(max_color, colors[i]);
    }

    // Convert to 5:6:5
    int min_r = ftoi_floor(31 * min_color.x);
    int min_g = ftoi_floor(63 * min_color.y);
    int min_b = ftoi_floor(31 * min_color.z);
    int max_r = ftoi_ceil(31 * max_color.x);
    int max_g = ftoi_ceil(63 * max_color.y);
    int max_b = ftoi_ceil(31 * max_color.z);

    // Expand the box.
    int range_r = max_r - min_r;
    int range_g = max_g - min_g;
    int range_b = max_b - min_b;

    min_r = max(0, min_r - range_r / 2 - 2);
    min_g = max(0, min_g - range_g / 2 - 2);
    min_b = max(0, min_b - range_b / 2 - 2);

    max_r = min(31, max_r + range_r / 2 + 2);
    max_g = min(63, max_g + range_g / 2 + 2);
    max_b = min(31, max_b + range_b / 2 + 2);

    // Estimate size of search space.
    int volume = (max_r-min_r+1) * (max_g-min_g+1) * (max_b-min_b+1);

    // if size under search_limit, then proceed. Note that search_volume is sqrt of number of evaluations.
    if (volume > max_volume) {
        return FLT_MAX;
    }

    // @@ Convert to fixed point before building box?
    Color32 colors32[16];
    for (int i = 0; i < count; i++) {
        colors32[i] = toColor32(Vector4(colors[i], 1));
    }

    float best_error = FLT_MAX;
    Color16 best0, best1;           // @@ Record endpoints as Color16?

    Color16 c0, c1;
    Color32 palette[4];

    for(int r0 = min_r; r0 <= max_r; r0++)
    for(int g0 = min_g; g0 <= max_g; g0++)
    for(int b0 = min_b; b0 <= max_b; b0++)
    {
        c0.r = r0; c0.g = g0; c0.b = b0;
        palette[0] = bitexpand_color16_to_color32(c0);

        for(int r1 = min_r; r1 <= max_r; r1++)
        for(int g1 = min_g; g1 <= max_g; g1++)
        for(int b1 = min_b; b1 <= max_b; b1++)
        {
            c1.r = r1; c1.g = g1; c1.b = b1;
            palette[1] = bitexpand_color16_to_color32(c1);

            if (c0.u > c1.u) {
                // Evaluate error in 4 color mode.
                evaluate_palette4(palette);
            }
            else {
                if (three_color_mode) {
                    // Evaluate error in 3 color mode.
                    evaluate_palette3(palette);
                }
                else {
                    // Skip 3 color mode.
                    continue;
                }
            }

            float error = evaluate_palette_error(palette, colors32, weights, count);

            if (error < best_error) {
                best_error = error;
                best0 = c0;
                best1 = c1;
            }
        }
    }

    output->col0 = best0;
    output->col1 = best1;

    Vector3 vector_palette[4];
    evaluate_palette(output->col0, output->col1, vector_palette);

    output->indices = compute_indices(input_colors, color_weights, vector_palette);

    return best_error / (255 * 255);
}


void nv::compress_dxt1_cluster_fit(const Vector4 input_colors[16], const Vector3 * colors, const float * weights, int count, const Vector3 & color_weights, bool three_color_mode, BlockDXT1 * output)
{
    ClusterFit fit;
    fit.setColorWeights(Vector4(color_weights, 1));
    fit.setColorSet(colors, weights, count);

    // start & end are in [0, 1] range.
    Vector3 start, end;
    fit.compress4(&start, &end);

    if (three_color_mode && fit.compress3(&start, &end)) {
        output_block3(input_colors, color_weights, start, end, output);
    }
    else {
        output_block4(input_colors, color_weights, start, end, output);
    }
}




float nv::compress_dxt1(const Vector4 input_colors[16], const float input_weights[16], const Vector3 & color_weights, bool three_color_mode, BlockDXT1 * output)
{
    Vector3 colors[16];
    float weights[16];
    int count = reduce_colors(input_colors, input_weights, colors, weights);

    if (count == 0) {
        // Output trivial block.
        output->col0.u = 0;
        output->col1.u = 0;
        output->indices = 0;
        return 0;
    }


    float error = FLT_MAX;

    // Sometimes the single color compressor produces better results than the exhaustive. This introduces discontinuities between blocks that
    // use different compressors. For this reason, this is not enabled by default.
    if (1) {
        error = compress_dxt1_single_color(colors, weights, count, color_weights, output);

        if (error == 0.0f || count == 1) {
            // Early out.
            return error;
        }
    }

    // This is too expensive, even with a low threshold.
    // If high quality:
    if (0) {
        BlockDXT1 exhaustive_output;
        float exhaustive_error = compress_dxt1_bounding_box_exhaustive(input_colors, colors, weights, count, color_weights, three_color_mode, 1400, &exhaustive_output);

        if (exhaustive_error != FLT_MAX) {
            float exhaustive_error2 = evaluate_mse(input_colors, input_weights, color_weights, &exhaustive_output);

            // The exhaustive compressor does not use color_weights, so the results may be different.
            //nvCheck(equal(exhaustive_error, exhaustive_error2));

            if (exhaustive_error2 < error) {
                *output = exhaustive_output;
                error = exhaustive_error;
            }
        }
    }

    // @@ TODO.
    // This is pretty fast and in some cases can produces better quality than cluster fit.
    //error = compress_dxt1_least_squares_fit(colors, weigths, error, output);

    // Cluster fit cannot handle single color blocks, so encode them optimally if we haven't encoded them already.
    if (error == FLT_MAX && count == 1) {
        error = compress_dxt1_single_color_optimal(colors[0], output);
    }

    if (count > 1) {
        BlockDXT1 cluster_fit_output;
        compress_dxt1_cluster_fit(input_colors, colors, weights, count, color_weights, three_color_mode, &cluster_fit_output);

        float cluster_fit_error = evaluate_mse(input_colors, input_weights, color_weights, &cluster_fit_output);
        
        if (cluster_fit_error < error) {
            *output = cluster_fit_output;
            error = cluster_fit_error;
        }
    }

    return error;
}


// Once we have an index assignment we have colors grouped in 1-4 clusters.
// If 1 clusters -> Use optimal compressor.
// If 2 clusters -> Try: (0, 1), (1, 2), (0, 2), (0, 3) - [0, 1]
// If 3 clusters -> Try: (0, 1, 2), (0, 1, 3), (0, 2, 3) - [0, 1, 2]
// If 4 clusters -> Try: (0, 1, 2, 3)

// @@ How do we do the initial index/cluster assignment? Use standard cluster fit.


// Least squares fitting of color end points for the given indices. @@ Take weights into account.
static bool optimize_end_points4(uint indices, const Vector3 * colors, const Vector3 * weights, int count, Vector3 * a, Vector3 * b)
{
    float alpha2_sum = 0.0f;
    float beta2_sum = 0.0f;
    float alphabeta_sum = 0.0f;
    Vector3 alphax_sum(0.0f);
    Vector3 betax_sum(0.0f);

    for (int i = 0; i < count; i++)
    {
        const uint bits = indices >> (2 * i);

        float beta = float(bits & 1);
        if (bits & 2) beta = (1 + beta) / 3.0f;
        float alpha = 1.0f - beta;

        alpha2_sum += alpha * alpha;
        beta2_sum += beta * beta;
        alphabeta_sum += alpha * beta;
        alphax_sum += alpha * colors[i];
        betax_sum += beta * colors[i];
    }

    float denom = alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum;
    if (equal(denom, 0.0f)) return false;

    float factor = 1.0f / denom;

    *a = saturate((alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor);
    *b = saturate((betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor);

    return true;
}


// Least squares fitting of color end points for the given indices. @@ This does not support black/transparent index. @@ Take weights into account.
static bool optimize_end_points3(uint indices, const Vector3 * colors, const Vector3 * weights, int count, Vector3 * a, Vector3 * b)
{
    float alpha2_sum = 0.0f;
    float beta2_sum = 0.0f;
    float alphabeta_sum = 0.0f;
    Vector3 alphax_sum(0.0f);
    Vector3 betax_sum(0.0f);

    for (int i = 0; i < count; i++)
    {
        const uint bits = indices >> (2 * i);

        float beta = float(bits & 1);
        if (bits & 2) beta = 0.5f;
        float alpha = 1.0f - beta;

        alpha2_sum += alpha * alpha;
        beta2_sum += beta * beta;
        alphabeta_sum += alpha * beta;
        alphax_sum += alpha * colors[i];
        betax_sum += beta * colors[i];
    }

    float denom = alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum;
    if (equal(denom, 0.0f)) return false;

    float factor = 1.0f / denom;

    *a = saturate((alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor);
    *b = saturate((betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor);

    return true;
}

// @@ After optimization we need to round end points. Round in all possible directions, and pick best.






