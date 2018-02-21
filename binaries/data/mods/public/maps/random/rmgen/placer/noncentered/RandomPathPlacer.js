/**
 * Creates a winded path between the given two vectors.
 * Uses a random angle at each step, so it can be more random than the sin form of the PathPlacer.
 * Omits the given offset after the start and before the end.
 */
function RandomPathPlacer(pathStart, pathEnd, pathWidth, offset, blended)
{
	this.pathStart = Vector2D.add(pathStart, Vector2D.sub(pathEnd, pathStart).normalize().mult(offset)).round();
	this.pathEnd = pathEnd;
	this.offset = offset;
	this.blended = blended;
	this.diskPlacer = new DiskPlacer(pathWidth);
	this.maxPathLength = fractionToTiles(2);
}

RandomPathPlacer.prototype.place = function(constraint)
{
	let pathLength = 0;
	let points = [];
	let position = this.pathStart;

	while (position.distanceTo(this.pathEnd) >= this.offset && pathLength++ < this.maxPathLength)
	{
		position.add(
			new Vector2D(1, 0).rotate(
				-getAngle(this.pathStart.x, this.pathStart.y, this.pathEnd.x, this.pathEnd.y) +
				-Math.PI / 2 * (randFloat(-1, 1) + (this.blended ? 0.5 : 0)))).round();

		this.diskPlacer.setCenterPosition(position);

		for (let point of this.diskPlacer.place(constraint))
			if (points.every(p => !Vector2D.isEqualTo(p, point)))
				points.push(point);
	}

	return points;
};
