function VectorDistance(a, b)
{
	var dx = a[0] - b[0];
	var dz = a[1] - b[1];
	return Math.sqrt(dx*dx + dz*dz);
}
