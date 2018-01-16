/**
 * Returns a pair of independent and normally distributed (zero mean, variance 1) random numbers.
 * Uses the Polar-Rejection method.
 */
function randomNormal2D()
{
	let s, a, b;
	do
	{
		a = 2 * Math.random() - 1;
		b = 2 * Math.random() - 1;
		s = a * a + b * b;
	} while (s>=1 || s==0);
	s = Math.sqrt(-2 * Math.log(s) / s);
	return [a * s, b * s];
}

/**
 * Return a random element of the source array.
 */
function pickRandom(source)
{
	return source.length ? source[Math.floor(source.length * Math.random())] : undefined;
}

/**
 * Return a random floating point number in the interval [min, max).
 */
function randFloat(min, max)
{
	return min + Math.random() * (max - min);
}

/**
 * Return a random integer of the interval [floor(min) .. ceil(max)] using Math.random library.
 *
 * If an argument is not integer, the uniform distribution is cut off at that endpoint.
 * For example randIntInclusive(1.5, 2.5) yields 50% chance to get 2 and 25% chance for 1 and 3.
 */
function randIntInclusive(min, max)
{
	return Math.floor(min + Math.random() * (max + 1 - min));
}

/**
 * Return a random integer of the interval [floor(min) .. ceil(max-1)].
 *
 * If an argument is not integer, the uniform distribution is cut off at that endpoint.
 * For example randIntExclusive(1.5, 3.5) yields 50% chance to get 2 and 25% chance for 1 and 3.
 */
function randIntExclusive(min, max)
{
	return Math.floor(min + Math.random() * (max - min));
}

/**
 * Returns a Bernoulli distributed boolean with p chance on true.
 */
function randBool(p = 0.5)
{
	return Math.random() < p;
}

/**
 * Returns a random radians between 0 and 360 degrees.
 */
function randomAngle()
{
	return randFloat(0, 2 * Math.PI);
}
