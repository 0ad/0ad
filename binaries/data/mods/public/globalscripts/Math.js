/**
 * Safe, platform consistent implementations of some Math functions
 *
 * These functions are implemented in JS to avoid observed differences
 * between results of different floating point libraries, see
 * https://bugzilla.mozilla.org/show_bug.cgi?id=531915
 *
 * They mostly meet the ECMAScript Edition 5 spec, see
 * https://www.ecma-international.org/publications/files/ECMA-ST/Ecma-262.pdf
 *
 * See simulation/components/tests/test_Math.js for tests.
 */

/**
 * Approximation of cosine of a (radians)
 */
Math.cos = function(a)
{
	// Bring a into the 0 to +pi range without expensive branching.
	// Uses the symmetry that cos is even.
	a = (a + Math.PI) % (2*Math.PI);
	a = Math.abs((2*Math.PI + a) % (2*Math.PI) - Math.PI);

	// make b = 0 if a < pi/2 and b=1 if a > pi/2
	var b = (a-Math.PI/2) + Math.abs(a-Math.PI/2);
	b = b/(b+1e-30); // normalize b to one while avoiding divide by zero errors.

	// if a > pi/2 send a to pi-a, otherwise just send a to -a which has no effect
	// Using the symmetry cos(x) = -cos(pi-x) to bring a to the 0 to pi/2 range.
	a = b*Math.PI - a;

	var c = 1 - 2*b; // sign of the output

	// Taylor expansion about 0 with a correction term in the quadratic to make cos(pi/2)=0
	return c * (1 - a*a*(0.5000000025619951 - a*a*(1/24 - a*a*(1/720 - a*a*(1/40320 - a*a*(1/3628800 - a*a/479001600))))));
};

/**
 * Approximation of sine of a (radians)
 */
Math.sin = function(a)
{
	return Math.cos(a - Math.PI/2);
};

/**
 * Approximation of arctangent of a, returns angle from -pi/2 to pi/2
 */
Math.atan = function(a)
{
	var tanPiBy6 = 0.5773502691896257;
	var tanPiBy12 = 0.2679491924311227;
	var sign = 1;
	var inverted = false;
	var tanPiBy6Shift = 0;

	if (a < 0 || 1/a === -Infinity)
	{
		// tan(x) = -tan(-x) so remove sign now and put it back at the end
		sign = -1;
		a *= -1;
	}

	if (a > 1)
	{
		// tan(pi/2 - x) = 1/tan(x)
		inverted = true;
		a = 1/a;
	}

	if (a > tanPiBy12)
	{
		// tan(x-pi/6) = (tan(x) - tan(pi/6)) / (1 + tan(pi/6)tan(x))
		tanPiBy6Shift = Math.PI/6;
		a = (a - tanPiBy6) / (1 + tanPiBy6*a);
	}
	// Now a will be in the range [-tan(pi/12), tan(pi/12)]

	// Use the taylor expansion around 0 with a correction to the linear term to match the pi/12 boundary
	// atan(x) = x - x^3/3 + x^5/5 - ...
	var r = a*(1.0000000000390272 - a*a*(1/3 - a*a*(1/5 - a*a*(1/7 - a*a*(1/9 - a*a*(1/11 - a*a*(1/13 - a*a/15)))))));

	// shift the result back where necessary
	r += tanPiBy6Shift;
	if (inverted)
		r = Math.PI/2 - r;
	return sign * r;
};

/**
 * Approximation of arctangent of y/x, returns angle from -pi to pi
 */
Math.atan2 = function(y,x)
{
	// get unsigned x,y for ease of calculation, this means all angles are in the range [0, pi/2]
	var ux = Math.abs(x);
	var uy = Math.abs(y);
	// holds the result in the upper right quadrant
	var r;

	// Handle all edges cases to match the spec
	if (uy === 0)
		r = 0;
	else
	{
		if (ux === 0)
			r = Math.PI / 2;

		if (uy === Infinity)
		{
			if (ux === Infinity)
				r = Math.PI / 4;
			else
				r = Math.PI / 2;
		}
		else
		{
			if (ux === Infinity)
				r = 0;
			else
				r = Math.atan(uy/ux);
		}
	}

	// puts the result into the correct quadrant
	// 1/(-0) is the only way to determine the sign for a 0 value
	if (x < 0 || 1/x === -Infinity)
	{
		if (y < 0 || 1/y === -Infinity)
			return -Math.PI + r;
		else
			return Math.PI - r;
	}
	else
	{
		if (y < 0 || 1/y === -Infinity)
			return -r;
		else
			return r;
	}
};

Math.acos = function()
{
	error("Math.acos() does not yet have a synchronization safe implementation");
};

Math.asin = function()
{
	error("Math.asin() does not yet have a synchronization safe implementation");
};

Math.tan = function()
{
	error("Math.tan() does not yet have a synchronization safe implementation");
};

/**
 * Approximation of raising x to the power y
 */
Math.pow = function(x, y)
{
	if (Math.round(y) === y)
	{
		if (y >= 0)
			return Math.intPow(x, y);

		return 1 / Math.intPow(x, -y);
	}
	// log has the biggest error when x ~=~ 1
	// exp has the biggest error when y*log(x) ~<~ 0
	// so the biggest error happens around numbers like pow(0.9999,0.0001),
	// that has an error of 10^-17. So I think we're safe
	return Math.exp(y*Math.log(x));
};

/**
 * Get the square of a number without repeating the value and without calling the slower Math.pow.
 */
Math.square = function(x)
{
	return x * x;
};

/**
 * Approximation of the exponential function, e raised to the power x
 */
Math.exp = function(x)
{
	if (x < 0)
		var iPart = 1/Math.intPow(Math.E, -Math.floor(x));
	else
		var iPart = Math.intPow(Math.E, Math.floor(x));

	if (x === Math.floor(x))
		// no need to loop if we know the answer
		return iPart;

	// the integer part is known, work further with the decimal part of x
	x = x - Math.floor(x); // x \in [0,1)

	// taylor series around 0
	// max error ~=~ 10^(-16)
	var dPart = 1;

	for (var i = 22; i > 0; i--)
		dPart = 1+x*dPart/i;

	// total precision ~=~ 17 decimal digits
	return iPart*dPart;
};

/**
 * Approximation of the natural logarithm of x
 *
 * For values very close to 1, the error of 10^-16 could become bigger than the actual value
 * But this also happens with the native log function
 */
Math.log = function(x)
{
	if (!(x >= 0))
		return NaN;

	if (x === 0)
		return -Infinity;

	if (x === Infinity)
		return x;

	// start with calculating the binary logarithm
	// based on https://en.wikipedia.org/wiki/Binary_logarithm#Real_number

	// calculate to 50 fractional bits -> error ~=~ 10^-16
	var precisionBits = 50;

	// calculate integer log, rounded down
	// when implemented in C, just count the number of bits before the fraction
	// without leading zeros. This may be negative.
	var log = 0;
	if (x >= 1)
	{
		for (var i = 1; i <= x; i *= 2)
			log++;

		log--;
		i /= 2;
	}
	else
	{
		for (var i = 1; i > x; i /= 2)
			log--;
	}
	// now lb(x) = log + lb(y) with y = x/i. So y \in [1,2)

	var y = x/i;

	// if we're done, or there's a minimal rounding error and we should be done
	// convert to natural logarithm
	if (y <= 1)
		return log / Math.LOG2E;

	var m = 0;
	var add = 1;
	while (true)
	{
		while (m <= precisionBits && y < 4)
		{
			m++;
			y *= y;
			add /= 2;
		}
		if (m > precisionBits)
			break;
		log += add;
		y /= 2;
	}

	// convert binary logarithm to natural logarithm;
	return log / Math.LOG2E;

};

/**
 * Calculate the power for positive integer exponents
 */
Math.intPow = function(x, y)
{
	if (Math.abs(y) === Infinity)
	{
		if (Math.abs(x) === 1)
			return NaN;

		if (Math.abs(x) < 1 && y > 0 || Math.abs(x) > 1 && y < 0)
			return 0;

		return Infinity;
	}
	var powers = [x];
	var binary = [1];
	var i = 0;
	for (var e = 2; e <= y; e *= 2)
	{
		// calculate x^i, using x^(i/2)
		powers.push(powers[i]*powers[i]);
		binary.push(e);
		i++;
	}

	var result = 1;

	var i = binary.length;

	while (y > 0)
	{
		if (binary[--i] <= y)
		{
			result *= powers[i];
			y -= binary[i];
		}
	}
	// error margin = 0 (default JS error)
	return result;

};

Math.euclidDistance2DSquared = function(x1, y1, x2, y2)
{
	return Math.square(x2 - x1) + Math.square(y2 - y1);
};

/**
 * Can be faster than Math.hypot.
 */
Math.euclidDistance2D = function(x1, y1, x2, y2)
{
	return Math.sqrt(Math.euclidDistance2DSquared(x1, y1, x2, y2));
};

Math.euclidDistance3DSquared = function(x1, y1, z1, x2, y2, z2)
{
	return Math.square(x2 - x1) + Math.square(y2 - y1) + Math.square(z2 - z1);
};

Math.euclidDistance3D = function(x1, y1, z1, x2, y2, z2)
{
	return Math.sqrt(Math.euclidDistance3DSquared(x1, y1, z1, x2, y2, z2));
};
