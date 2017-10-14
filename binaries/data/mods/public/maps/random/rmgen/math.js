function rotateCoordinates(x, z, angle, centerX = 0.5, centerZ = 0.5)
{
	let sin = Math.sin(angle);
	let cos = Math.cos(angle);

	return [
		cos * (x - centerX) - sin * (z - centerZ) + centerX,
		sin * (x - centerX) + cos * (z - centerZ) + centerZ
	];
}
