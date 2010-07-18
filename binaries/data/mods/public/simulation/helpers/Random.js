/**
 * Returns a random integer from min (inclusive) to max (exclusive)
 */
function RandomInt(min, max)
{
	return Math.floor(min + Math.random() * (max-min))
}

Engine.RegisterGlobal("RandomInt", RandomInt);
