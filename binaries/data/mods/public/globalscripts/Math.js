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

Math.sin = function(a)
{
	return Math.cos(a - Math.PI/2);
};

// Returns angle from -pi/2 to pi/2
Math.atan = function(a)
{
	var tanPiBy6 = 0.5773502691896257;
	var tanPiBy12 = 0.2679491924311227;
	var sign = 1;
	var inverted = false;
	var tanPiBy6Shift = 0;
	
	if (a < 0){
		// tan(x) = -tan(-x) so remove sign now and put it back at the end
		sign = -1;
		a *= -1;
	}
	if (a > 1){
		// tan(pi/2 - x) = 1/tan(x)
		inverted = true;
		a = 1/a;
	}
	if (a > tanPiBy12){
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

Math.atan2 = function(y,x)
{
	// get unsigned x,y for ease of calculation, this means all angles are in the range [0, pi/2]
	var ux = Math.abs(x);
	var uy = Math.abs(y);
	// holds the result in the upper right quadrant
	var r;
	
	// Handle all edges cases to match the spec
	if (uy === 0)
	{
		r = 0;
	}
	else
	{
		if (ux === 0)
		{
			r = Math.PI / 2;
		}
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
