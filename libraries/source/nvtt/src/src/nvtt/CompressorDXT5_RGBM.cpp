#include "CompressorDXT5_RGBM.h"
#include "CompressorDXT1.h"

#include "OptimalCompressDXT.h"
#include "QuickCompressDXT.h"

#include "nvimage/ColorBlock.h"
#include "nvimage/BlockDXT.h"

#include "nvmath/Color.inl"
#include "nvmath/Vector.inl"
#include "nvmath/Fitting.h"
#include "nvmath/ftoi.h"

#include "nvthread/Atomic.h"
#include <stdio.h>

using namespace nv;

//static uint atomic_counter = 0;


float nv::compress_dxt5_rgbm(const Vector4 input_colors[16], const float input_weights[16], float min_m, BlockDXT5 * output) {

    // Convert to RGBM.
    Vector4 input_colors_rgbm[16]; // @@ Write over input_colors?
    float rgb_weights[16];

    float weight_sum = 0;

    for (uint i = 0; i < 16; i++) {
        const Vector4 & c = input_colors[i];

        float R = saturate(c.x);
        float G = saturate(c.y);
        float B = saturate(c.z);

        float M = max(max(R, G), max(B, min_m));
        float r = R / M;
        float g = G / M;
        float b = B / M;
        float a = (M - min_m) / (1 - min_m);

        input_colors_rgbm[i] = Vector4(r, g, b, a);
        rgb_weights[i] = input_weights[i] * M;
        weight_sum += input_weights[i];
    }

    if (weight_sum == 0) {
        for (uint i = 0; i < 16; i++) rgb_weights[i] = 1;
    }

    // Compress RGB.
    compress_dxt1(input_colors_rgbm, rgb_weights, Vector3(1), /*three_color_mode=*/false, &output->color);

    // Decompress RGB/M block.
    nv::ColorBlock RGB;
    output->color.decodeBlock(&RGB);

    // Compute M values to compensate for RGB's error.
    AlphaBlock4x4 M;
    for (int i = 0; i < 16; i++) {
        const Vector4 & c = input_colors[i];

        float R = saturate(c.x);
        float G = saturate(c.y);
        float B = saturate(c.z);

        float rm = RGB.color(i).r / 255.0f;
        float gm = RGB.color(i).g / 255.0f;
        float bm = RGB.color(i).b / 255.0f;

        // compute m such that m * (r/M, g/M, b/M) == RGB
    
        // Three equations, one unknown:
        //  m * r/M == R
        //  m * g/M == G
        //  m * b/M == B
        
        // Solve in the least squares sense!

        // m (rm gm bm) (rm gm bm)^T == (rm gm bm) (R G B)^T

        // m == dot(rgb, RGB) / dot(rgb, rgb)

        float m = dot(Vector3(rm, gm, bm), Vector3(R, G, B)) / dot(Vector3(rm, gm, bm), Vector3(rm, gm, bm));

        m = (m - min_m) / (1 - min_m);

#if 0
        // IC: This does indeed happen. What does that mean? The best choice of m is above the available range. If this happened too often it would make sense to scale m in
        // the pixel shader to allow for more accurate reconstruction. However, that scaling would reduce the precision over the [0-1] range. I haven't measured how much
        // error is introduced by the clamping vs. how much the error would change with the increased range.
        if (m > 1.0f) {
            uint counter = atomicIncrement(&atomic_counter);
            printf("It happens %u times!", counter);
        }
#endif

        M.alpha[i] = U8(ftoi_round(saturate(m) * 255.0f));
        M.weights[i] = input_weights[i];
    }

    // Compress M.
    //if (compressionOptions.quality == Quality_Fastest) {
    //    QuickCompress::compressDXT5A(M, &output->alpha);
    /*}
    else {*/
        OptimalCompress::compressDXT5A(M, &output->alpha);
    //}


#if 0   // Multiple iterations do not seem to help.
    // Decompress M.
    output->alpha.decodeBlock(&M);

    // Feed it back to the input RGB block.
    for (uint i = 0; i < 16; i++) {
        const Vector4 & c = input_colors[i];

        float R = saturate(c.x);
        float G = saturate(c.y);
        float B = saturate(c.z);

        float m = float(M.alpha[i]) / 255.0f * (1 - min_m) + min_m;

        float r = R / m;
        float g = G / m;
        float b = B / m;
        float a = float(M.alpha[i]) / 255.0f;

        input_colors_rgbm[i] = Vector4(r, g, b, a);
        rgb_weights[i] = input_weights[i] * m;
    }
#endif

    return 0; // @@ 
}




#if 0

    BlockDXT5 * block = new(output)BlockDXT5;

    // Decompress the color block and find the M values that reproduce the input most closely. This should compensate for some of the DXT errors.

    // Compress the resulting M values optimally.

    // Repeat this several times until compression error does not improve?

    //Vector3 rgb_block[16];
    //float m_block[16];


    // Init RGB/M block.
#if 0
    nvsquish::WeightedClusterFit fit;

    ColorBlock rgba;
    for (int i = 0; i < 16; i++) {
        const Vector4 & c = src.color(i);
        float R = saturate(c.x);
        float G = saturate(c.y);
        float B = saturate(c.z);

        float M = max(max(R, G), max(B, min_m));
        float r = R / M;
        float g = G / M;
        float b = B / M;
        float a = c.w;

        rgba.color(i) = toColor32(Vector4(r, g, b, a));
    }

    if (rgba.isSingleColor())
    {
        OptimalCompress::compressDXT1(rgba.color(0), &block->color);
    }
    else
    {
        nvsquish::WeightedClusterFit fit;
        fit.SetMetric(compressionOptions.colorWeight.x, compressionOptions.colorWeight.y, compressionOptions.colorWeight.z);

        int flags = 0;
        if (alphaMode == nvtt::AlphaMode_Transparency) flags |= nvsquish::kWeightColourByAlpha;

        nvsquish::ColourSet colours((uint8 *)rgba.colors(), flags);
        fit.SetColourSet(&colours, 0);
        fit.Compress(&block->color);
    }
#endif
#if 1
    ColorSet rgb;
    rgb.allocate(4, 4);

    for (uint i = 0; i < 16; i++) {
        const Vector4 & c = colors[i];

        float R = saturate(c.x);
        float G = saturate(c.y);
        float B = saturate(c.z);

        float M = max(max(R, G), max(B, min_m));
        float r = R / M;
        float g = G / M;
        float b = B / M;
        float a = c.w;

        rgb.colors[i] = Vector4(r, g, b, a);
        rgb.indices[i] = i;
        rgb.weights[i] = max(weights[i], 0.001f);// weights[i];   // IC: For some reason 0 weights are causing problems, even if we eliminate the corresponding colors from the set.
    }

    rgb.createMinimalSet(/*ignoreTransparent=*/true);

    if (rgb.isSingleColor(/*ignoreAlpha=*/true)) {
        OptimalCompress::compressDXT1(toColor32(rgb.color(0)), &block->color);
    }
    else {
        ClusterFit fit;
        fit.setColorWeights(compressionOptions.colorWeight);
        fit.setColorSet(&rgb);

        Vector3 start, end;
        fit.compress4(&start, &end);

        QuickCompress::outputBlock4(rgb, start, end, &block->color);
    }
#endif

    // Decompress RGB/M block.
    nv::ColorBlock RGB;
    block->color.decodeBlock(&RGB);
    
#if 1
    AlphaBlock4x4 M;
    for (int i = 0; i < 16; i++) {
        const Vector4 & c = colors[i];
        float R = saturate(c.x);
        float G = saturate(c.y);
        float B = saturate(c.z);

        float r = RGB.color(i).r / 255.0f;
        float g = RGB.color(i).g / 255.0f;
        float b = RGB.color(i).b / 255.0f;

        float m = (R / r + G / g + B / b) / 3.0f;
        //float m = max((R / r + G / g + B / b) / 3.0f, min_m);
        //float m = max(max(R / r, G / g), max(B / b, min_m));
        //float m = max(max(R, G), max(B, min_m));

        m = (m - min_m) / (1 - min_m);

        M.alpha[i] = U8(ftoi_round(saturate(m) * 255.0f));
        M.weights[i] = weights[i];
    }

    // Compress M.
    if (compressionOptions.quality == Quality_Fastest) {
        QuickCompress::compressDXT5A(M, &block->alpha);
    }
    else {
        OptimalCompress::compressDXT5A(M, &block->alpha);
    }
#else
    OptimalCompress::compressDXT5A_RGBM(src, RGB, &block->alpha);
#endif

#if 0
    // Decompress M.
    block->alpha.decodeBlock(&M);

    rgb.allocate(src.w, src.h);     // @@ Handle smaller blocks.

    for (uint i = 0; i < src.colorCount; i++) {
        const Vector4 & c = src.color(i);

        float R = saturate(c.x);
        float G = saturate(c.y);
        float B = saturate(c.z);

        //float m = max(max(R, G), max(B, min_m));
        float m = float(M.alpha[i]) / 255.0f * (1 - min_m) + min_m;
        float r = R / m;
        float g = G / m;
        float b = B / m;
        float a = c.w;

        rgb.colors[i] = Vector4(r, g, b, a);
        rgb.indices[i] = i;
        rgb.weights[i] = max(c.w, 0.001f);// src.weights[i];   // IC: For some reason 0 weights are causing problems, even if we eliminate the corresponding colors from the set.
    }

    rgb.createMinimalSet(/*ignoreTransparent=*/true);

    if (rgb.isSingleColor(/*ignoreAlpha=*/true)) {
        OptimalCompress::compressDXT1(toColor32(rgb.color(0)), &block->color);
    }
    else {
        ClusterFit fit;
        fit.setMetric(compressionOptions.colorWeight);
        fit.setColourSet(&rgb);

        Vector3 start, end;
        fit.compress4(&start, &end);

        QuickCompress::outputBlock4(rgb, start, end, &block->color);
    }
#endif

#if 0
    block->color.decodeBlock(&RGB);

    //AlphaBlock4x4 M;
    //M.initWeights(src);
    
    for (int i = 0; i < 16; i++) {
        const Vector4 & c = src.color(i);
        float R = saturate(c.x);
        float G = saturate(c.y);
        float B = saturate(c.z);

        float r = RGB.color(i).r / 255.0f;
        float g = RGB.color(i).g / 255.0f;
        float b = RGB.color(i).b / 255.0f;

        float m = (R / r + G / g + B / b) / 3.0f;
        //float m = max((R / r + G / g + B / b) / 3.0f, min_m);
        //float m = max(max(R / r, G / g), max(B / b, min_m));
        //float m = max(max(R, G), max(B, min_m));

        m = (m - min_m) / (1 - min_m);

        M.alpha[i] = U8(ftoi_round(saturate(m) * 255.0f));
        M.weights[i] = src.weights[i];
    }

    // Compress M.
    if (compressionOptions.quality == Quality_Fastest) {
        QuickCompress::compressDXT5A(M, &block->alpha);
    }
    else {
        OptimalCompress::compressDXT5A(M, &block->alpha);
    }
#endif



#if 0
    src.fromRGBM(M, min_m);

    src.createMinimalSet(/*ignoreTransparent=*/true);

    if (src.isSingleColor(/*ignoreAlpha=*/true)) {
        OptimalCompress::compressDXT1(src.color(0), &block->color);
    }
    else {
        // @@ Use our improved compressor.
        ClusterFit fit;
        fit.setMetric(compressionOptions.colorWeight);
        fit.setColourSet(&src);

        Vector3 start, end;
        fit.compress4(&start, &end);

        if (fit.compress3(&start, &end)) {
            QuickCompress::outputBlock3(src, start, end, block->color);
        }
        else {
            QuickCompress::outputBlock4(src, start, end, block->color);
        }
    }
#endif // 0

    // @@ Decompress color and compute M that best approximates src with these colors? Then compress M again?



    // RGBM encoding.
    // Maximize precision.
    // - Number of possible grey levels:
    //   - Naive:  2^3 = 8
    //   - Better: 2^3 + 2^2 = 12
    //   - How to choose min_m? 
    //     - Ideal = Adaptive per block, don't know where to store.
    //     - Adaptive per lightmap. How to compute optimal?
    //     - Fixed: 0.25 in our case. Lightmaps scaled to a fixed [0, 1] range.

    // - Optimal compressor: Interpolation artifacts.

    // - Color transform. 
    //    - Measure error in post-tone-mapping color space. 
    //    - Assume a simple tone mapping operator. We know minimum and maximum exposure, but don't know exact exposure in game.
    //    - Guess based on average lighmap color? Use fixed exposure, in scaled lightmap space.

    // - Enhanced DXT compressor.
    //    - Typical RGBM encoding as follows:
    //      rgb -> M = max(rgb), RGB=rgb/M -> RGBM
    //    - If we add a compression step (M' = M) and M' < M, then rgb may be greater than 1.
    //      - We could ensure that M' >= M during compression.
    //      - We could clamp RGB anyway.
    //      - We could add a fixed scale value to take into account compression errors and avoid clamping.


    


    // Compress color.
    /*if (rgba.isSingleColor())
    {
        OptimalCompress::compressDXT1(rgba.color(0), &block->color);
    }
    else
    {
        nvsquish::WeightedClusterFit fit;
        fit.SetMetric(compressionOptions.colorWeight.x, compressionOptions.colorWeight.y, compressionOptions.colorWeight.z);

        int flags = 0;
        if (alphaMode == nvtt::AlphaMode_Transparency) flags |= nvsquish::kWeightColourByAlpha;

        nvsquish::ColourSet colours((uint8 *)rgba.colors(), flags);
        fit.SetColourSet(&colours, 0);
        fit.Compress(&block->color);
    }*/

#endif // 0