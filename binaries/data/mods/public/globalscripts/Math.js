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
