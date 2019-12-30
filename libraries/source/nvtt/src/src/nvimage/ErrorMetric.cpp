
#include "ErrorMetric.h"
#include "FloatImage.h"
#include "Filter.h"

#include "nvmath/Matrix.h"
#include "nvmath/Vector.inl"

#include <float.h> // FLT_MAX

using namespace nv;

float nv::rmsColorError(const FloatImage * ref, const FloatImage * img, bool alphaWeight)
{
    if (!sameLayout(img, ref)) {
        return FLT_MAX;
    }
    nvDebugCheck(img->componentCount() == 4);
    nvDebugCheck(ref->componentCount() == 4);

    double mse = 0;

    const uint count = img->pixelCount();
    for (uint i = 0; i < count; i++)
    {
        float r0 = ref->pixel(i + count * 0);
        float g0 = ref->pixel(i + count * 1);
        float b0 = ref->pixel(i + count * 2);
        float a0 = ref->pixel(i + count * 3);
        float r1 = img->pixel(i + count * 0);
        float g1 = img->pixel(i + count * 1);
        float b1 = img->pixel(i + count * 2);
        //float a1 = img->pixel(i + count * 3);

        float r = r0 - r1;
        float g = g0 - g1;
        float b = b0 - b1;

        float a = 1;
        if (alphaWeight) a = a0 * a0; // @@ a0*a1 or a0*a0 ?

        mse += (r * r) * a;
        mse += (g * g) * a;
        mse += (b * b) * a;
    }

    return float(sqrt(mse / count));
}

float nv::rmsAlphaError(const FloatImage * ref, const FloatImage * img)
{
    if (!sameLayout(img, ref)) {
        return FLT_MAX;
    }
    nvDebugCheck(img->componentCount() == 4 && ref->componentCount() == 4);

    double mse = 0;

    const uint count = img->pixelCount();
    for (uint i = 0; i < count; i++)
    {
        float a0 = img->pixel(i + count * 3);
        float a1 = ref->pixel(i + count * 3);

        float a = a0 - a1;

        mse += a * a;
    }

    return float(sqrt(mse / count));
}


float nv::averageColorError(const FloatImage * ref, const FloatImage * img, bool alphaWeight)
{
    if (!sameLayout(img, ref)) {
        return FLT_MAX;
    }
    nvDebugCheck(img->componentCount() == 4);
    nvDebugCheck(ref->componentCount() == 4);

    double mae = 0;

    const uint count = img->pixelCount();
    for (uint i = 0; i < count; i++)
    {
        float r0 = img->pixel(i + count * 0);
        float g0 = img->pixel(i + count * 1);
        float b0 = img->pixel(i + count * 2);
        //float a0 = img->pixel(i + count * 3);
        float r1 = ref->pixel(i + count * 0);
        float g1 = ref->pixel(i + count * 1);
        float b1 = ref->pixel(i + count * 2);
        float a1 = ref->pixel(i + count * 3);

        float r = fabs(r0 - r1);
        float g = fabs(g0 - g1);
        float b = fabs(b0 - b1);

        float a = 1;
        if (alphaWeight) a = a1;

        mae += r * a;
        mae += g * a;
        mae += b * a;
    }

    return float(mae / count);
}

float nv::averageAlphaError(const FloatImage * ref, const FloatImage * img)
{
    if (img == NULL || ref == NULL || img->width() != ref->width() || img->height() != ref->height()) {
        return FLT_MAX;
    }
    nvDebugCheck(img->componentCount() == 4 && ref->componentCount() == 4);

    double mae = 0;

    const uint count = img->width() * img->height();
    for (uint i = 0; i < count; i++)
    {
        float a0 = img->pixel(i + count * 3);
        float a1 = ref->pixel(i + count * 3);

        float a = a0 - a1;

        mae += fabs(a);
    }

    return float(mae / count);
}


// Color space conversions based on:
// http://www.brucelindbloom.com/

// Assumes input is in *linear* sRGB color space.
static Vector3 rgbToXyz(Vector3::Arg c)
{
    Vector3 xyz;
    xyz.x = 0.412453f * c.x + 0.357580f * c.y + 0.180423f * c.z;
    xyz.y = 0.212671f * c.x + 0.715160f * c.y + 0.072169f * c.z;
    xyz.z = 0.019334f * c.x + 0.119193f * c.y + 0.950227f * c.z;
    return xyz;
}

static Vector3 xyzToRgb(Vector3::Arg c)
{
    Vector3 rgb;
    rgb.x =  3.2404542f * c.x - 1.5371385f * c.y - 0.4985314f * c.z;
    rgb.y = -0.9692660f * c.x + 1.8760108f * c.y + 0.0415560f * c.z;
    rgb.z =  0.0556434f * c.x - 0.2040259f * c.y + 1.0572252f * c.z;
    return rgb;
}

static float toLinear(float f)
{
    return powf(f, 2.2f);
}

static float toGamma(float f)
{
    // @@ Use sRGB space?
    return powf(f, 1.0f/2.2f);
}

static Vector3 toLinear(Vector3::Arg c)
{
    return Vector3(toLinear(c.x), toLinear(c.y), toLinear(c.z));
}

static Vector3 toGamma(Vector3::Arg c)
{
    return Vector3(toGamma(c.x), toGamma(c.y), toGamma(c.z));
}

static float f(float t)
{
    const float epsilon = powf(6.0f/29.0f, 3);

    if (t > epsilon) {
        return powf(t, 1.0f/3.0f);
    }
    else {
        return 1.0f/3.0f * powf(29.0f/6.0f, 2) * t + 4.0f / 29.0f;
    }
}

static float finv(float t)
{
    if (t > 6.0f / 29.0f) {
        return 3.0f * powf(6.0f / 29.0f, 2) * (t - 4.0f / 29.0f);
    }
    else {
        return powf(t, 3.0f);
    }
}

static Vector3 xyzToCieLab(Vector3::Arg c)
{
    // Normalized white point.
    const float Xn = 0.950456f;
    const float Yn = 1.0f;
    const float Zn = 1.088754f;

    float Xr = c.x / Xn;
    float Yr = c.y / Yn;
    float Zr = c.z / Zn;

    float fx = f(Xr);
    float fy = f(Yr);
    float fz = f(Zr);

    float L = 116 * fx - 16;
    float a = 500 * (fx - fy);
    float b = 200 * (fy - fz);

    return Vector3(L, a, b);
}

static Vector3 rgbToCieLab(Vector3::Arg c)
{
    return xyzToCieLab(rgbToXyz(toLinear(c)));
}

// h is hue-angle in radians
static Vector3 cieLabToLCh(Vector3::Arg c)
{
    return Vector3(c.x, sqrtf(c.y*c.y + c.z*c.z), atan2f(c.y, c.z));
}

static void rgbToCieLab(const FloatImage * rgbImage, FloatImage * LabImage)
{
    nvDebugCheck(rgbImage != NULL && LabImage != NULL);
    nvDebugCheck(rgbImage->width() == LabImage->width() && rgbImage->height() == LabImage->height());
    nvDebugCheck(rgbImage->componentCount() >= 3 && LabImage->componentCount() >= 3);

    const uint w = rgbImage->width();
    const uint h = LabImage->height();

    const float * R = rgbImage->channel(0);
    const float * G = rgbImage->channel(1);
    const float * B = rgbImage->channel(2);

    float * L = LabImage->channel(0);
    float * a = LabImage->channel(1);
    float * b = LabImage->channel(2);

    const uint count = w*h;
    for (uint i = 0; i < count; i++)
    {
        Vector3 Lab = rgbToCieLab(Vector3(R[i], G[i], B[i]));
        L[i] = Lab.x;
        a[i] = Lab.y;
        b[i] = Lab.z;
    }
}


// Assumes input images are in linear sRGB space.
float nv::cieLabError(const FloatImage * img0, const FloatImage * img1)
{
    if (!sameLayout(img0, img1)) return FLT_MAX;
    nvDebugCheck(img0->componentCount() == 4 && img1->componentCount() == 4);

    const float * r0 = img0->channel(0);
    const float * g0 = img0->channel(1);
    const float * b0 = img0->channel(2);

    const float * r1 = img1->channel(0);
    const float * g1 = img1->channel(1);
    const float * b1 = img1->channel(2);

    double error = 0.0f;

    const uint count = img0->pixelCount();
    for (uint i = 0; i < count; i++)
    {
        Vector3 lab0 = rgbToCieLab(Vector3(r0[i], g0[i], b0[i]));
        Vector3 lab1 = rgbToCieLab(Vector3(r1[i], g1[i], b1[i]));

        // @@ Measure Delta E.
        Vector3 delta = lab0 - lab1;
        
        error += length(delta);
    }

    return float(error / count);
}

// Assumes input images are in linear sRGB space.
float nv::cieLab94Error(const FloatImage * img0, const FloatImage * img1)
{
    if (!sameLayout(img0, img1)) return FLT_MAX;
    nvDebugCheck(img0->componentCount() == 4 && img1->componentCount() == 4);

    const float kL = 1;
    const float kC = 1;
    const float kH = 1;
    const float k1 = 0.045f;
    const float k2 = 0.015f;

    const float sL = 1;

    const float * r0 = img0->channel(0);
    const float * g0 = img0->channel(1);
    const float * b0 = img0->channel(2);

    const float * r1 = img1->channel(0);
    const float * g1 = img1->channel(1);
    const float * b1 = img1->channel(2);

    double error = 0.0f;

    const uint count = img0->pixelCount();
    for (uint i = 0; i < count; ++i)
    {
        Vector3 lab0 = rgbToCieLab(Vector3(r0[i], g0[i], b0[i]));
        Vector3 lch0 = cieLabToLCh(lab0);
        Vector3 lab1 = rgbToCieLab(Vector3(r1[i], g1[i], b1[i]));
        Vector3 lch1 = cieLabToLCh(lab1);

        const float sC = 1 + k1*lch0.x;
        const float sH = 1 + k2*lch0.x;

        // @@ Measure Delta E using the 1994 definition
        Vector3 labDelta = lab0 - lab1;
        Vector3 lchDelta = lch0 - lch1;

        double deltaLsq = powf(lchDelta.x / (kL*sL), 2);
        double deltaCsq = powf(lchDelta.y / (kC*sC), 2);

        // avoid possible sqrt of negative value by computing (deltaH/(kH*sH))^2
        double deltaHsq = powf(labDelta.y, 2) + powf(labDelta.z, 2) - powf(lchDelta.y, 2);
        deltaHsq /= powf(kH*sH, 2);

        error += sqrt(deltaLsq + deltaCsq + deltaHsq);
    }

    return float(error / count);
}

float nv::spatialCieLabError(const FloatImage * img0, const FloatImage * img1)
{
    if (img0 == NULL || img1 == NULL || img0->width() != img1->width() || img0->height() != img1->height()) {
        return FLT_MAX;
    }
    nvDebugCheck(img0->componentCount() == 4 && img1->componentCount() == 4);

    uint w = img0->width();
    uint h = img0->height();
    uint d = img0->depth();

    FloatImage lab0, lab1; // Original images in CIE-Lab space.
    lab0.allocate(3, w, h, d);
    lab1.allocate(3, w, h, d);

    // Convert input images to CIE-Lab.
    rgbToCieLab(img0, &lab0);
    rgbToCieLab(img1, &lab1);

    // @@ Convolve each channel by the corresponding filter.
    /*
    GaussianFilter LFilter(5);
    GaussianFilter aFilter(5);
    GaussianFilter bFilter(5);

    lab0.convolve(0, LFilter);
    lab0.convolve(1, aFilter);
    lab0.convolve(2, bFilter);

    lab1.convolve(0, LFilter);
    lab1.convolve(1, aFilter);
    lab1.convolve(2, bFilter);
    */
    // @@ Measure Delta E between lab0 and lab1.

    return 0.0f;
}


// Assumes input images are normal maps.
float nv::averageAngularError(const FloatImage * img0, const FloatImage * img1)
{
    if (img0 == NULL || img1 == NULL || img0->width() != img1->width() || img0->height() != img1->height()) {
        return FLT_MAX;
    }
    nvDebugCheck(img0->componentCount() == 4 && img1->componentCount() == 4);

    uint w = img0->width();
    uint h = img0->height();

    const float * x0 = img0->channel(0);
    const float * y0 = img0->channel(1);
    const float * z0 = img0->channel(2);

    const float * x1 = img1->channel(0);
    const float * y1 = img1->channel(1);
    const float * z1 = img1->channel(2);

    double error = 0.0f;

    const uint count = w*h;
    for (uint i = 0; i < count; i++)
    {
        Vector3 n0 = Vector3(x0[i], y0[i], z0[i]);
        Vector3 n1 = Vector3(x1[i], y1[i], z1[i]);

        n0 = 2.0f * n0 - Vector3(1);
        n1 = 2.0f * n1 - Vector3(1);

        n0 = normalizeSafe(n0, Vector3(0), 0.0f);
        n1 = normalizeSafe(n1, Vector3(0), 0.0f);

        error += acos(clamp(dot(n0, n1), -1.0f, 1.0f));
    }

    return float(error / count);
}

float nv::rmsAngularError(const FloatImage * img0, const FloatImage * img1)
{
    if (img0 == NULL || img1 == NULL || img0->width() != img1->width() || img0->height() != img1->height()) {
        return FLT_MAX;
    }
    nvDebugCheck(img0->componentCount() == 4 && img1->componentCount() == 4);

    uint w = img0->width();
    uint h = img0->height();

    const float * x0 = img0->channel(0);
    const float * y0 = img0->channel(1);
    const float * z0 = img0->channel(2);

    const float * x1 = img1->channel(0);
    const float * y1 = img1->channel(1);
    const float * z1 = img1->channel(2);

    double error = 0.0f;

    const uint count = w*h;
    for (uint i = 0; i < count; i++)
    {
        Vector3 n0 = Vector3(x0[i], y0[i], z0[i]);
        Vector3 n1 = Vector3(x1[i], y1[i], z1[i]);

        n0 = 2.0f * n0 - Vector3(1);
        n1 = 2.0f * n1 - Vector3(1);

        n0 = normalizeSafe(n0, Vector3(0), 0.0f);
        n1 = normalizeSafe(n1, Vector3(0), 0.0f);

        float angle = acosf(clamp(dot(n0, n1), -1.0f, 1.0f));
        error += angle * angle;
    }

    return float(sqrt(error / count));
}

