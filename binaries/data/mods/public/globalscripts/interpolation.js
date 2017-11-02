/**
 * Interpolates the value of a point that is located between four known equidistant points with given values by
 * constructing a polynomial of degree three that goes through all points.
 * Computes a cardinal or Catmull-Rom spline.
 *
 * @param {Number} tension - determines how sharply the curve bends at the given points.
 * @param {Number} x - Location of the point to interpolate, relative to p1
 */
function cubicInterpolation(tension, x, p0, p1, p2, p3)
{
	let P = -tension * p0 + (2 - tension) * p1 + (tension - 2) * p2 + tension * p3;
	let Q = 2 * tension * p0 + (tension - 3) * p1 + (3 - 2 * tension) * p2 - tension * p3;
	let R = -tension * p0 + tension * p2;
	let S = p1;

	return ((P * x + Q) * x + R) * x + S;
}

/**
 * Two dimensional interpolation within a square grid using a polynomial of degree three.
 *
 * @param {Number} x, y - Location of the point to interpolate, relative to p11
 */
function bicubicInterpolation
(
	x, y,
	p00, p01, p02, p03,
	p10, p11, p12, p13,
	p20, p21, p22, p23,
	p30, p31, p32, p33
)
{
	let tension = 0.5;
	return cubicInterpolation(
		tension,
		x,
		cubicInterpolation(tension, y, p00, p01, p02, p03),
		cubicInterpolation(tension, y, p10, p11, p12, p13),
		cubicInterpolation(tension, y, p20, p21, p22, p23),
		cubicInterpolation(tension, y, p30, p31, p32, p33));
}
