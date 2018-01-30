/**
 * @file A Centered Placer generates a shape (array of points) around a variable center location satisfying a Constraint.
 * The center is determined by the x and z property which can be modified externally, typically by createAreas.
 */

/////////////////////////////////////////////////////////////////////////////////////////
//	ClumpPlacer
//
//	Class for generating a roughly circular clump of points
//
//	size: The average number of points in the clump
//	coherence: How much the radius of the clump varies (1.0 = circle, 0.0 = very random)
//	smoothness: How smooth the border of the clump is (1.0 = few "peaks", 0.0 = very jagged)
//	failfraction: Percentage of place attempts allowed to fail (optional)
//	centerPosition: Tile coordinates of placer center (optional)
//
/////////////////////////////////////////////////////////////////////////////////////////

function ClumpPlacer(size, coherence, smoothness, failFraction = 0, centerPosition = undefined)
{
	this.size = size;
	this.coherence = coherence;
	this.smoothness = smoothness;
	this.failFraction = failFraction;
	this.centerPosition = undefined;

	if (centerPosition)
		this.setCenterPosition(centerPosition);
}

ClumpPlacer.prototype.setCenterPosition = function(position)
{
	this.centerPosition = deepfreeze(position.clone().round());
};

ClumpPlacer.prototype.place = function(constraint)
{
	// Preliminary bounds check
	if (!g_Map.inMapBounds(this.centerPosition) || !constraint.allows(this.centerPosition))
		return undefined;

	var points = [];

	var size = g_Map.getSize();
	var gotRet = new Array(size).fill(0).map(p => new Uint8Array(size)); // booleans
	var radius = Math.sqrt(this.size / Math.PI);
	var perim = 4 * radius * 2 * Math.PI;
	var intPerim = Math.ceil(perim);

	var ctrlPts = 1 + Math.floor(1.0/Math.max(this.smoothness,1.0/intPerim));

	if (ctrlPts > radius * 2 * Math.PI)
		ctrlPts = Math.floor(radius * 2 * Math.PI) + 1;

	var noise = new Float32Array(intPerim);			//float32
	var ctrlCoords = new Float32Array(ctrlPts+1);	//float32
	var ctrlVals = new Float32Array(ctrlPts+1);		//float32

	// Generate some interpolated noise
	for (var i=0; i < ctrlPts; i++)
	{
		ctrlCoords[i] = i * perim / ctrlPts;
		ctrlVals[i] = randFloat(0, 2);
	}

	let c = 0;
	let looped = 0;
	for (let i = 0; i < intPerim; ++i)
	{
		if (ctrlCoords[(c+1) % ctrlPts] < i && !looped)
		{
			c = (c+1) % ctrlPts;
			if (c == ctrlPts-1)
				looped = 1;
		}

		noise[i] = cubicInterpolation(
			1,
			(i - ctrlCoords[c]) / ((looped ? perim : ctrlCoords[(c + 1) % ctrlPts]) - ctrlCoords[c]),
			ctrlVals[(c + ctrlPts - 1) % ctrlPts],
			ctrlVals[c],
			ctrlVals[(c + 1) % ctrlPts],
			ctrlVals[(c + 2) % ctrlPts]);
	}

	let failed = 0;
	for (let stepAngle = 0; stepAngle < intPerim; ++stepAngle)
	{
		let position = this.centerPosition.clone();
		let radiusUnitVector = new Vector2D(0, 1).rotate(-2 * Math.PI * stepAngle / perim);
		let maxRadiusSteps = Math.ceil(radius * (1 + (1 - this.coherence) * noise[stepAngle]));

		for (let stepRadius = 0; stepRadius < maxRadiusSteps; ++stepRadius)
		{
			let tilePos = position.clone().floor();

			if (g_Map.inMapBounds(tilePos) && constraint.allows(tilePos))
			{
				if (!gotRet[tilePos.x][tilePos.y])
				{
					gotRet[tilePos.x][tilePos.y] = 1;
					points.push(tilePos);
				}
			}
			else
				++failed;

			position.add(radiusUnitVector);
		}
	}

	return failed > this.size * this.failFraction ? undefined : points;
};

/////////////////////////////////////////////////////////////////////////////////////////
//	Chain Placer
//
//	Class for generating a more random clump of points it randomly creates circles around the edges of the current clump
//
//	minRadius: minimum radius of the circles
//	maxRadius: maximum radius of the circles
//	numCircles: the number of the circles
//	failfraction: Percentage of place attempts allowed to fail (optional)
//	position: Tile coordinates of placer center (optional)
//  maxDistance: Farthest circle center (optional)
//  queue: a list containing numbers. each time if the list still contains values, pops one from the end and uses it as the radius (optional)
//
/////////////////////////////////////////////////////////////////////////////////////////

function ChainPlacer(minRadius, maxRadius, numCircles, failFraction = 0, centerPosition = undefined, maxDistance = 0, queue = [])
{
	this.minRadius = minRadius;
	this.maxRadius = maxRadius;
	this.numCircles = numCircles;
	this.failFraction = failFraction;
	this.maxDistance = maxDistance;
	this.queue = queue;
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

		// TODO: get all points in this square to abstract and make the loop onedimensional
		let sx = Math.max(0, chainPos.x - radius);
		let sz = Math.max(0, chainPos.y - radius);
		let lx = Math.min(chainPos.x + radius, size);
		let lz = Math.min(chainPos.y + radius, size);

		for (let ix = sx; ix <= lx; ++ix)
			for (let iz = sz; iz <= lz; ++iz)
			{
				let position = new Vector2D(ix, iz);

				if (position.distanceToSquared(chainPos) >= radius2)
					continue;

				++count;

				if (!g_Map.inMapBounds(position) || !constraint.allows(position))
				{
					++failed;
					continue;
				}

				let state = gotRet[ix][iz];
				if (state == -1)
				{
					points.push(position);
					gotRet[ix][iz] = -2;
				}
				else if (state >= 0)
				{
					let s = edges.splice(state, 1);
					gotRet[ix][iz] = -2;

					let edgesLength = edges.length;
					for (let k = state; k < edges.length; ++k)
						--gotRet[edges[k].x][edges[k].y];
				}
			}

		for (let ix = sx; ix <= lx; ++ix)
			for (let iz = sz; iz <= lz; ++ iz)
			{
				let pos = new Vector2D(ix, iz);

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
