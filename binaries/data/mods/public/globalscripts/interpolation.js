/**
 * One dimensional interpolation within four uniformly spaced points using polynomials of 3rd degree.
 * A uniform Catmull–Rom spline implementation.
 * @param {float} xf - Float coordinate relative to p1 to interpolate the property for
 * @param {float} p0 - Value of the property to calculate at the origin of four point scale
 * @param {float} px - Value at the property of the corresponding point on the scale
 * @return {float} - Value at the given float position in the 4-point scale interpolated from it's property values
 */
function cubicInterpolation(xf, p0, p1, p2, p3)
{
	 return p1 +
		(-0.5 * p0 + 0.5 * p2) * xf +
		(p0 - 2.5 * p1 + 2.0 * p2 - 0.5 * p3) * xf * xf +
		(-0.5 * p0 + 1.5 * p1 - 1.5 * p2 + 0.5 * p3) * xf * xf * xf;
}

/**
 * Two dimensional interpolation within a square grid using polynomials of 3rd degree.
 * @param {float} xf - X coordinate relative to p1y of the point to interpolate the value for
 * @param {float} yf - Y coordinate relative to px1 of the point to interpolate the value for
 * @param {float} p00 - Value of the property to calculate at the origin of the neighbor grid
 * @param {float} pxy - Value at neighbor grid coordinates x/y, x/y in {0, 1, 2, 3}
 * @return {float} - Value at the given float coordinates in the small grid interpolated from it's values
 */
function bicubicInterpolation
(
	xf, yf,
	p00, p01, p02, p03,
	p10, p11, p12, p13,
	p20, p21, p22, p23,
	p30, p31, p32, p33
)
{
	return cubicInterpolation(
		xf,
		cubicInterpolation(yf, p00, p01, p02, p03),
		cubicInterpolation(yf, p10, p11, p12, p13),
		cubicInterpolation(yf, p20, p21, p22, p23),
		cubicInterpolation(yf, p30, p31, p32, p33)
	);
}
