/**
 * From https://github.com/hughsk/bicubic licensed under MIT 2013 Hugh Kennedy - see /license_mit.txt
 * Two dimensional interpolation within a square grid using polynomials of 3rd degree.
 * @param {float} xf - X coordinate within the subgrid (see below) of the point to interpolate the value for
 * @param {float} yf - y coordinate within the subgrid (see below) of the point to interpolate the value for
 * @param {float} p00 - Value of the property to calculate at the origin of the naighbor grid
 * @param {float} pxy - Value at naighbor grid coordinates x/y, x/y in {0, 1, 2, 3}
 * @return {float} - Value at the given float coordinates in the small grid interpolated from it's values
  */
function bicubicInterpolation(
	xf, yf,
	p00, p01, p02, p03,
	p10, p11, p12, p13,
	p20, p21, p22, p23,
	p30, p31, p32, p33
)
{
	var yf2 = yf * yf;
	var xf2 = xf * xf;
	var xf3 = xf * xf2;

	var x00 = p03 - p02 - p00 + p01;
	var x01 = p00 - p01 - x00;
	var x02 = p02 - p00;
	var x0 = x00*xf3 + x01*xf2 + x02*xf + p01;

	var x10 = p13 - p12 - p10 + p11;
	var x11 = p10 - p11 - x10;
	var x12 = p12 - p10;
	var x1 = x10*xf3 + x11*xf2 + x12*xf + p11;

	var x20 = p23 - p22 - p20 + p21;
	var x21 = p20 - p21 - x20;
	var x22 = p22 - p20;
	var x2 = x20*xf3 + x21*xf2 + x22*xf + p21;

	var x30 = p33 - p32 - p30 + p31;
	var x31 = p30 - p31 - x30;
	var x32 = p32 - p30;
	var x3 = x30*xf3 + x31*xf2 + x32*xf + p31;

	var y0 = x3 - x2 - x0 + x1;
	var y1 = x0 - x1 - y0;
	var y2 = x2 - x0;

	return y0*yf*yf2 + y1*yf2 + y2*yf + x1;
}
