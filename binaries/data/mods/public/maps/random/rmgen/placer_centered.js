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
//	position: Tile coordinates of placer center (optional)
//
/////////////////////////////////////////////////////////////////////////////////////////

function ClumpPlacer(size, coherence, smoothness, failFraction = 0, position = undefined)
{
	this.size = size;
	this.coherence = coherence;
	this.smoothness = smoothness;
	this.failFraction = failFraction;
	this.x = position ? Math.round(position.x) : -1;
	this.z = position ? Math.round(position.y) : -1;
}

ClumpPlacer.prototype.place = function(constraint)
{
	let centerPosition = new Vector2D(this.x, this.z);

	// Preliminary bounds check
	if (!g_Map.inMapBounds(this.x, this.z) || !constraint.allows(centerPosition))
		return undefined;

	var retVec = [];

	var size = getMapSize();
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

	var c = 0;
	var looped = 0;
	for (var i=0; i < intPerim; i++)
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

	var failed = 0;
	for (var p=0; p < intPerim; p++)
	{
		var th = 2 * Math.PI * p / perim;
		var r = radius * (1 + (1-this.coherence)*noise[p]);
		var s = Math.sin(th);
		var c = Math.cos(th);
		var xx = this.x;
		var yy = this.z;

		for (var k = 0; k < Math.ceil(r); ++k)
		{
			var i = Math.floor(xx);
			var j = Math.floor(yy);
			let position = new Vector2D(i, j);

			if (g_Map.inMapBounds(i, j) && constraint.allows(position))
			{
				if (!gotRet[i][j])
				{
					// Only include each point once
					gotRet[i][j] = 1;
					retVec.push(position);
				}
			}
			else
				failed++;

			xx += s;
			yy += c;
		}
	}

	return failed > this.size * this.failFraction ? undefined : retVec;
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
//  fcc: Farthest circle center (optional)
//  q: a list containing numbers. each time if the list still contains values, pops one from the end and uses it as the radius (optional)
//
/////////////////////////////////////////////////////////////////////////////////////////

function ChainPlacer(minRadius, maxRadius, numCircles, failFraction, position, fcc, q)
{
	this.minRadius = minRadius;
	this.maxRadius = maxRadius;
	this.numCircles = numCircles;
	this.failFraction = failFraction !== undefined ? failFraction : 0;
	this.x = position ? Math.round(position.x) : -1;
	this.z = position ? Math.round(position.y) : -1;
	this.fcc = fcc !== undefined ? fcc : 0;
	this.q = q !== undefined ? q : [];
}

ChainPlacer.prototype.place = function(constraint)
{
	let centerPosition = new Vector2D(this.x, this.z);

	// Preliminary bounds check
	if (!g_Map.inMapBounds(this.x, this.z) || !constraint.allows(centerPosition))
		return undefined;

	var retVec = [];
	var size = getMapSize();
	var failed = 0, count = 0;
	var queueEmpty = !this.q.length;

	var gotRet = new Array(size).fill(0).map(p => new Array(size).fill(-1));
	--size;

	this.minRadius = Math.min(this.maxRadius, Math.max(this.minRadius, 1));

	var edges = [[this.x, this.z]];
	for (var i = 0; i < this.numCircles; ++i)
	{
		var [cx, cz] = pickRandom(edges);
		if (queueEmpty)
			var radius = randIntInclusive(this.minRadius, this.maxRadius);
		else
		{
			var radius = this.q.pop();
			queueEmpty = !this.q.length;
		}

		var sx = cx - radius, lx = cx + radius;
		var sz = cz - radius, lz = cz + radius;

		sx = Math.max(0, sx);
		sz = Math.max(0, sz);
		lx = Math.min(lx, size);
		lz = Math.min(lz, size);

		var radius2 = radius * radius;
		var dx, dz;

		for (var ix = sx; ix <= lx; ++ix)
			for (var iz = sz; iz <= lz; ++ iz)
			{
				let position = new Vector2D(ix, iz);
				dx = ix - cx;
				dz = iz - cz;
				if (dx * dx + dz * dz <= radius2)
				{
					if (g_Map.inMapBounds(ix, iz) && constraint.allows(position))
					{
						var state = gotRet[ix][iz];
						if (state == -1)
						{
							retVec.push(position);
							gotRet[ix][iz] = -2;
						}
						else if (state >= 0)
						{
							var s = edges.splice(state, 1);
							gotRet[ix][iz] = -2;

							var edgesLength = edges.length;
							for (var k = state; k < edges.length; ++k)
								--gotRet[edges[k][0]][edges[k][1]];
						}
					}
					else
						++failed;

					++count;
				}
			}

		for (var ix = sx; ix <= lx; ++ix)
			for (var iz = sz; iz <= lz; ++ iz)
			{
				if (this.fcc)
					if ((this.x - ix) > this.fcc || (ix - this.x) > this.fcc || (this.z - iz) > this.fcc || (iz - this.z) > this.fcc)
						continue;

				if (gotRet[ix][iz] == -2)
				{
					if (ix > 0)
					{
						if (gotRet[ix-1][iz] == -1)
						{
							edges.push([ix, iz]);
							gotRet[ix][iz] = edges.length - 1;
							continue;
						}
					}
					if (iz > 0)
					{
						if (gotRet[ix][iz-1] == -1)
						{
							edges.push([ix, iz]);
							gotRet[ix][iz] = edges.length - 1;
							continue;
						}
					}
					if (ix < size)
					{
						if (gotRet[ix+1][iz] == -1)
						{
							edges.push([ix, iz]);
							gotRet[ix][iz] = edges.length - 1;
							continue;
						}
					}
					if (iz < size)
					{
						if (gotRet[ix][iz+1] == -1)
						{
							edges.push([ix, iz]);
							gotRet[ix][iz] = edges.length - 1;
							continue;
						}
					}
				}
			}
	}

	return failed > count * this.failFraction ? undefined : retVec;
};
