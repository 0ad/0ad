#ifndef INCLUDED_MATHUTIL
#define INCLUDED_MATHUTIL

#ifndef PI
#define PI							3.14159265358979323846f
#endif

#define DEGTORAD(a)					((a) * (PI/180.0f))
#define RADTODEG(a)					((a) * (180.0f/PI))
#define SQR(x)						((x) * (x))

template <typename T>
T Interpolate(const T& a, const T& b, float l)
{
	return a + (b - a) * l;
}

template <typename T>
inline T clamp(T value, T min, T max)
{
	if (value <= min) return min;
	else if (value >= max) return max;
	else return value;
}

inline float sgn(float a)
{
    if (a > 0.0f) return 1.0f;
    if (a < 0.0f) return -1.0f;
    return 0.0f;
}

#endif
