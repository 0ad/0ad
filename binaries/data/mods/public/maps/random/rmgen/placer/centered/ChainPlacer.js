/**
 * Generates a more random clump of points. It randomly creates circles around the edges of the current clump.s
 *
 * @param {number} minRadius - minimum radius of the circles.
 * @param {number} maxRadius - maximum radius of the circles.
 * @param {number} numCircles - number of circles.
 * @param {number} [failFraction] - Percentage of place attempts allowed to fail.
 * @param {Vector2D} [centerPosition]
 * @param {number} [maxDistance] - Farthest distance from the center.
 * @param {number[]} [queue] - When given, uses these radiuses for the first circles.
 */
function ChainPlacer(minRadius, maxRadius, numCircles, failFraction = 0, centerPosition = undefined, maxDistance = 0, queue = [])
{
	this.minRadius = minRadius;
	this.maxRadius = maxRadius;
	this.numCircles = numCircles;
	this.failFraction = failFraction;
	this.maxDistance = maxDistance;
	this.queue = queue.map(radius => Math.floor(radius));
	this.centerPosition = undefined;

	if (centerPosition)
		this.setCenterPosition(centerPosition);
}

ChainPlacer.prototype.setCenterPosition = function(position)
{
	this.centerPosition = deepfreeze(position.clone().round());
};

ChainPlacer.prototype.place = function(constraint)
{
	// Preliminary bounds check
	if (!g_Map.inMapBounds(this.centerPosition) || !constraint.allows(this.centerPosition))
		return undefined;

	let points = [];
	let size = g_Map.getSize();
	let failed = 0;
	let count = 0;

	let gotRet = new Array(size).fill(0).map(p => new Array(size).fill(-1));
	--size;

	this.minRadius = Math.min(this.maxRadius, Math.max(this.minRadius, 1));

	let edges = [this.centerPosition];

	for (let i = 0; i < this.numCircles; ++i)
	{
		let chainPos = pickRandom(edges);
		let radius = this.queue.length ? this.queue.pop() : randIntInclusive(this.minRadius, this.maxRadius);
		let radius2 = Math.square(radius);

		let bbox = getPointsInBoundingBox(getBoundingBox([
			new Vector2D(Math.max(0, chainPos.x - radius), Math.max(0, chainPos.y - radius)),
			new Vector2D(Math.min(chainPos.x + radius, size), Math.min(chainPos.y + radius, size))
		]));

		for (let position of bbox)
		{
			if (position.distanceToSquared(chainPos) >= radius2)
				continue;

			++count;

			if (!g_Map.inMapBounds(position) || !constraint.allows(position))
			{
				++failed;
				continue;
			}

			let state = gotRet[position.x][position.y];
			if (state == -1)
			{
				points.push(position);
				gotRet[position.x][position.y] = -2;
			}
			else if (state >= 0)
			{
				let s = edges.splice(state, 1);
				gotRet[position.x][position.y] = -2;

				let edgesLength = edges.length;
				for (let k = state; k < edges.length; ++k)
					--gotRet[edges[k].x][edges[k].y];
			}
		}

		for (let pos of bbox)
		{
			if (this.maxDistance &&
			    (Math.abs(this.centerPosition.x - pos.x) > this.maxDistance ||
			     Math.abs(this.centerPosition.y - pos.y) > this.maxDistance))
				continue;

			if (gotRet[pos.x][pos.y] != -2)
				continue;

			if (pos.x > 0 && gotRet[pos.x - 1][pos.y] == -1 ||
			    pos.y > 0 && gotRet[pos.x][pos.y - 1] == -1 ||
			    pos.x < size && gotRet[pos.x + 1][pos.y] == -1 ||
			    pos.y < size && gotRet[pos.x][pos.y + 1] == -1)
			{
				edges.push(pos);
				gotRet[pos.x][pos.y] = edges.length - 1;
			}
		}
	}

	return failed > count * this.failFraction ? undefined : points;
};
