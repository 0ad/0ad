/////////////////////////////////////////////////////////////////////////////////////////
//	passageMaker
//
//	Function for creating shallow water between two given points by changing the heiight of all tiles in
//	the path with height less than or equal to "maxheight" to "height"
//
//	x1,z1: 	Starting point of path
//	x2,z2: 	Ending point of path
//	width: 	Width of the shallow
//	maxheight:		Maximum height that it changes
//	height:		Height of the shallow
//	smooth:		smooth elevation in borders
//	tileclass:		(Optianal) - Adds those tiles to the class given
//	terrain:		(Optional) - Changes the texture of the elevated land
//
/////////////////////////////////////////////////////////////////////////////////////////

function passageMaker(x1, z1, x2, z2, width, maxheight, height, smooth, tileclass, terrain, riverheight)
{
	var tchm = TILE_CENTERED_HEIGHT_MAP;
	TILE_CENTERED_HEIGHT_MAP = true;
	var mapSize = g_Map.size;
	for (var ix = 0; ix < mapSize; ix++)
	{
		for (var iz = 0; iz < mapSize; iz++)
		{
			var a = z1-z2;
			var b = x2-x1;
			var c = (z1*(x1-x2))-(x1*(z1-z2));
			var dis = abs(a*ix + b*iz + c)/sqrt(a*a + b*b);
			var k = (a*ix + b*iz + c)/(a*a + b*b);
			var my = iz-(b*k);
			var inline = 0;
			if (b == 0)
			{
				dis = abs(ix-x1);
				if ((iz <= Math.max(z1,z2))&&(iz >= Math.min(z1,z2)))
				{
					inline = 1;
				}
			}
			else
			{
				if ((my <= Math.max(z1,z2))&&(my >= Math.min(z1,z2)))
				{
					inline = 1;
				}
			}
			if ((dis <= width)&&(inline))
			{
				if(g_Map.getHeight(ix, iz) <= maxheight)
				{
					if (dis > width - smooth)
					{
						g_Map.setHeight(ix, iz, ((width - dis)*(height)+(riverheight)*(smooth - width + dis))/(smooth));
					}
					else if (dis <= width - smooth)
					{
						g_Map.setHeight(ix, iz, height);
					}
					if (tileclass !== undefined)
					{
						addToClass(ix, iz, tileclass);
					}
					if (terrain !== undefined)
					{
						placeTerrain(ix, iz, terrain);
					}
				}
			}
		}
	}
	TILE_CENTERED_HEIGHT_MAP = tchm;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//rndRiver is a fuction that creates random values useful for making a jagged river.
//
//it works the same as sin or cos function. the only difference is that it's period is 1 instead of 2*pi
//it needs the "seed" parameter to use it to make random curves that don't get broken.
//seed must be created using randFloat(). or else it won't work
//
//	f:	Input: Same as angle in a sine function
//	seed:	Random Seed: Best to implement is to use randFloat()
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

function rndRiver(f, seed)
{
	var rndRq = seed;
	var rndRw = rndRq;
	var rndRe = 0;
	var rndRr = f-floor(f);
	var rndRa = 0;
	for (var rndRx=0; rndRx<=floor(f); rndRx++)
	{
		rndRw = 10*(rndRw-floor(rndRw));
	}
	if (rndRx%2==0)
	{
		var rndRs = -1;
	}
	else
	{
		var rndRs = 1;
	}
	rndRe = (floor(rndRw))%5;
	if (rndRe==0)
	{
		rndRa = (rndRs)*2.3*(rndRr)*(rndRr-1)*(rndRr-0.5)*(rndRr-0.5);
	}
	else if (rndRe==1)
	{
		rndRa = (rndRs)*2.6*(rndRr)*(rndRr-1)*(rndRr-0.3)*(rndRr-0.7);
	}
	else if (rndRe==2)
	{
		rndRa = (rndRs)*22*(rndRr)*(rndRr-1)*(rndRr-0.2)*(rndRr-0.3)*(rndRr-0.3)*(rndRr-0.8);
	}
	else if (rndRe==3)
	{
		rndRa = (rndRs)*180*(rndRr)*(rndRr-1)*(rndRr-0.2)*(rndRr-0.2)*(rndRr-0.4)*(rndRr-0.6)*(rndRr-0.6)*(rndRr-0.8);
	}
	else if (rndRe==4)
	{
		rndRa = (rndRs)*2.6*(rndRr)*(rndRr-1)*(rndRr-0.5)*(rndRr-0.7);
	}
	return rndRa;
}


/////////////////////////////////////////////////////////////////////////////////////////
// createStartingPlayerEntities
//
//	Creates the starting player entities
//	fx&fz: position of player base
//	playerid: id of player
//	civEntities: use getStartingEntities(id-1) fo this one
//	orientation: orientation of the main base building, default is BUILDING_ORIENTATION
//
///////////////////////////////////////////////////////////////////////////////////////////
function createStartingPlayerEntities(fx, fz, playerid, civEntities, orientation = BUILDING_ORIENTATION)
{
	var uDist = 6;
	var uSpace = 2;
	placeObject(fx, fz, civEntities[0].Template, playerid, orientation);
	for (var j = 1; j < civEntities.length; ++j)
	{
		var uAngle = orientation - PI * (2-j) / 2;
		var count = (civEntities[j].Count !== undefined ? civEntities[j].Count : 1);
		for (var numberofentities = 0; numberofentities < count; numberofentities++)
		{
			var ux = fx + uDist * cos(uAngle) + numberofentities * uSpace * cos(uAngle + PI/2) - (0.75 * uSpace * floor(count / 2) * cos(uAngle + PI/2));
			var uz = fz + uDist * sin(uAngle) + numberofentities * uSpace * sin(uAngle + PI/2) - (0.75 * uSpace * floor(count / 2) * sin(uAngle + PI/2));
			placeObject(ux, uz, civEntities[j].Template, playerid, uAngle);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// placeCivDefaultEntities
//
//	Creates the default starting player entities depending on the players civ
//	fx&fy: position of player base
//	playerid: id of player
//	kwargs: Takes some optional keyword arguments to tweek things
//		'iberWall': may be false, 'walls' (default) or 'towers'. Determines the defensive structures Iberians get as civ bonus
//		'orientation': angle of the main base building, default is BUILDING_ORIENTATION
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
function placeCivDefaultEntities(fx, fz, playerid, kwargs = {})
{
	// Unpack kwargs
	var iberWall = 'walls';
	if (getMapSize() <= 128)
		iberWall = false;
	if ('iberWall' in kwargs)
		iberWall = kwargs['iberWall'];
	var orientation = BUILDING_ORIENTATION;
	if ('orientation' in kwargs)
		orientation = kwargs['orientation'];
	// Place default civ starting entities
	var civ = getCivCode(playerid-1);
	var civEntities = getStartingEntities(playerid-1);
	var uDist = 6;
	var uSpace = 2;
	placeObject(fx, fz, civEntities[0].Template, playerid, orientation);
	for (var j = 1; j < civEntities.length; ++j)
	{
		var uAngle = orientation - PI * (2-j) / 2;
		var count = (civEntities[j].Count !== undefined ? civEntities[j].Count : 1);
		for (var numberofentities = 0; numberofentities < count; numberofentities++)
		{
			var ux = fx + uDist * cos(uAngle) + numberofentities * uSpace * cos(uAngle + PI/2) - (0.75 * uSpace * floor(count / 2) * cos(uAngle + PI/2));
			var uz = fz + uDist * sin(uAngle) + numberofentities * uSpace * sin(uAngle + PI/2) - (0.75 * uSpace * floor(count / 2) * sin(uAngle + PI/2));
			placeObject(ux, uz, civEntities[j].Template, playerid, uAngle);
		}
	}
	// Add defensive structiures for Iberians as their civ bonus
	if (civ == 'iber' && iberWall != false)
	{
		if (iberWall == 'towers')
			placePolygonalWall(fx, fz, 15, ['entry'], 'tower', civ, playerid, orientation, 7);
		else
			placeGenericFortress(fx, fz, 20/*radius*/, playerid);
	}
}

function placeDefaultChicken(playerX, playerZ, tileClass, constraint = undefined, template = "gaia/fauna_chicken")
{
	for (let j = 0; j < 2; ++j)
		for (var tries = 0; tries < 10; ++tries)
		{
			let aAngle = randFloat(0, TWO_PI);

			// Roman and ptolemian civic centers have a big footprint!
			let aDist = 9;

			let aX = round(playerX + aDist * cos(aAngle));
			let aZ = round(playerZ + aDist * sin(aAngle));

			let group = new SimpleGroup(
				[new SimpleObject(template, 5,5, 0,2)],
				true, tileClass, aX, aZ
			);

			if (createObjectGroup(group, 0, constraint))
				break;
		}
}

/**
 * Typically used for placing grass tufts around the civic centers.
 */
function placeDefaultDecoratives(playerX, playerZ, template, tileclass, radius, constraint = undefined)
{
	for (let i = 0; i < PI * radius * radius / 250; ++i)
	{
		let angle = randFloat(0, 2 * PI);
		let dist = radius - randIntInclusive(5, 11);

		createObjectGroup(
			new SimpleGroup(
				[new SimpleObject(template, 2, 5, 0, 1, -PI/8, PI/8)],
				false,
				tileclass,
				Math.round(playerX + dist * Math.cos(angle)),
				Math.round(playerZ + dist * Math.sin(angle))
			), 0, constraint);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// paintTerrainBasedOnHeight
//
//	paints the tiles which have a height between minheight and maxheight with the given terrain
//	minheight: minimum height of the tile
//	maxheight: maximum height of the tile
//	mode: accepts 4 values. 0 means the it will select tiles with height more than minheight and less than maxheight.
//  1 means it selects tiles with height more than or equal to minheight and less than max height. 2 means more than
//  minheight and less than or equal to maxheight. 3 means more than or equal to minheight and less than or equal to maxheight
//	terrain: intended terrain texture
//
///////////////////////////////////////////////////////////////////////////////////////////

function paintTerrainBasedOnHeight(minheight, maxheight, mode, terrain)
{
	var mSize = g_Map.size;
	for (var qx = 0; qx < mSize; qx++)
	{
		for (var qz = 0; qz < mSize; qz++)
		{
			if (mode == 0)
			{
				if ((g_Map.getHeight(qx, qz) > minheight)&&(g_Map.getHeight(qx, qz) < maxheight))
				{
					placeTerrain(qx, qz, terrain);
				}
			}
			else if (mode == 1)
			{
				if ((g_Map.getHeight(qx, qz) >= minheight)&&(g_Map.getHeight(qx, qz) < maxheight))
				{
					placeTerrain(qx, qz, terrain);
				}
			}
			else if (mode == 2)
			{
				if ((g_Map.getHeight(qx, qz) > minheight)&&(g_Map.getHeight(qx, qz) <= maxheight))
				{
					placeTerrain(qx, qz, terrain);
				}
			}
			else if (mode == 3)
			{
				if ((g_Map.getHeight(qx, qz) >= minheight)&&(g_Map.getHeight(qx, qz) <= maxheight))
				{
					placeTerrain(qx, qz, terrain);
				}
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// paintTileClassBasedOnHeight and unPaintTileClassBasedOnHeight
//
//	paints or unpaints the tiles which have a height between minheight and maxheight with the given tile class
//	minheight: minimum height of the tile
//	maxheight: maximum height of the tile
//	mode: accepts 4 values. 0 means the it will select tiles with height more than minheight and less than maxheight.
//  1 means it selects tiles with height more than or equal to minheight and less than max height. 2 means more than
//  minheight and less than or equal to maxheight. 3 means more than or equal to minheight and less than or equal to maxheight
//	tileclass: intended tile class
//
///////////////////////////////////////////////////////////////////////////////////////////

function paintTileClassBasedOnHeight(minheight, maxheight, mode, tileclass)
{
	var mSize = g_Map.size;
	for (var qx = 0; qx < mSize; qx++)
	{
		for (var qz = 0; qz < mSize; qz++)
		{
			if (mode == 0)
			{
				if ((g_Map.getHeight(qx, qz) > minheight)&&(g_Map.getHeight(qx, qz) < maxheight))
				{
					addToClass(qx, qz, tileclass);
				}
			}
			else if (mode == 1)
			{
				if ((g_Map.getHeight(qx, qz) >= minheight)&&(g_Map.getHeight(qx, qz) < maxheight))
				{
					addToClass(qx, qz, tileclass);
				}
			}
			else if (mode == 2)
			{
				if ((g_Map.getHeight(qx, qz) > minheight)&&(g_Map.getHeight(qx, qz) <= maxheight))
				{
					addToClass(qx, qz, tileclass);
				}
			}
			else if (mode == 3)
			{
				if ((g_Map.getHeight(qx, qz) >= minheight)&&(g_Map.getHeight(qx, qz) <= maxheight))
				{
					addToClass(qx, qz, tileclass);
				}
			}
		}
	}
}


function unPaintTileClassBasedOnHeight(minheight, maxheight, mode, tileclass)
{
	var mSize = g_Map.size;
	for (var qx = 0; qx < mSize; qx++)
	{
		for (var qz = 0; qz < mSize; qz++)
		{
			if (mode == 0)
			{
				if ((g_Map.getHeight(qx, qz) > minheight)&&(g_Map.getHeight(qx, qz) < maxheight))
				{
					removeFromClass(qx, qz, tileclass);
				}
			}
			else if (mode == 1)
			{
				if ((g_Map.getHeight(qx, qz) >= minheight)&&(g_Map.getHeight(qx, qz) < maxheight))
				{
					removeFromClass(qx, qz, tileclass);
				}
			}
			else if (mode == 2)
			{
				if ((g_Map.getHeight(qx, qz) > minheight)&&(g_Map.getHeight(qx, qz) <= maxheight))
				{
					removeFromClass(qx, qz, tileclass);
				}
			}
			else if (mode == 3)
			{
				if ((g_Map.getHeight(qx, qz) >= minheight)&&(g_Map.getHeight(qx, qz) <= maxheight))
				{
					removeFromClass(qx, qz, tileclass);
				}
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// getTIPIADBON
//
//	"get The Intended Point In A Direction Based On Height"
//	gets the N'th point with a specific height in a line and returns it as a [x, y] array
//	startPoint: [x, y] array defining the start point
//	endPoint: [x, y] array defining the ending point
//	heightRange: [min, max] array defining the range which the height of the intended point can be. includes both "min" and "max"
//  step: how much tile units per turn should the search go. more value means faster but less accurate
//  n: how many points to skip before ending the search. skips """n-1 points""".
//
///////////////////////////////////////////////////////////////////////////////////////////

function getTIPIADBON(startPoint, endPoint, heightRange, step, n)
{
	var stepX = step*(endPoint[0]-startPoint[0])/(sqrt((endPoint[0]-startPoint[0])*(endPoint[0]-startPoint[0]) + step*(endPoint[1]-startPoint[1])*(endPoint[1]-startPoint[1])));
	var stepY = step*(endPoint[1]-startPoint[1])/(sqrt((endPoint[0]-startPoint[0])*(endPoint[0]-startPoint[0]) + step*(endPoint[1]-startPoint[1])*(endPoint[1]-startPoint[1])));
	var y = startPoint[1];
	var checked = 0;
	for (var x = startPoint[0]; true; x += stepX)
	{
		if ((floor(x) < g_Map.size)||(floor(y) < g_Map.size))
		{
			if ((g_Map.getHeight(floor(x), floor(y)) <= heightRange[1])&&(g_Map.getHeight(floor(x), floor(y)) >= heightRange[0]))
			{
				++checked;
			}
			if (checked >= n)
			{
				return [x, y];
			}
		}
		y += stepY;
		if ((y > endPoint[1])&&(stepY>0))
			break;
		if ((y < endPoint[1])&&(stepY<0))
			break;
		if ((x > endPoint[1])&&(stepX>0))
			break;
		if ((x < endPoint[1])&&(stepX<0))
			break;
	}
	return undefined;
}

/////////////////////////////////////////////////////////////////////////////////////////
// doIntersect
//
//	determines if two lines with the width "width" intersect or collide with each other
//	x1, y1, x2, y2: determine the position of the first line
//	x3, y3, x4, y4: determine the position of the second line
//	width: determines the width of the lines
//
///////////////////////////////////////////////////////////////////////////////////////////

function checkIfIntersect (x1, y1, x2, y2, x3, y3, x4, y4, width)
{
	if (x1 == x2)
	{
		if (((x3 - x1) < width) || ((x4 - x2) < width))
			return true;
	}
	else
	{
		var m = (y1 - y2) / (x1 - x2);
		var b = y1 - m * x1;
		var m2 = sqrt(m * m + 1);
		if ((Math.abs((y3 - x3 * m - b)/m2) < width) || (Math.abs((y4 - x4 * m - b)/m2) < width))
			return true;
		//neccessary for some situations.
		if (x3 == x4)
		{
			if (((x1 - x3) < width) || ((x2 - x4) < width))
				return true;
		}
		else
		{
			var m = (y3 - y4) / (x3 - x4);
			var b = y3 - m * x3;
			var m2 = sqrt(m * m + 1);
			if ((Math.abs((y1 - x1 * m - b)/m2) < width) || (Math.abs((y2 - x2 * m - b)/m2) < width))
				return true;
		}
	}

	var s = ((x1 - x2) * (y3 - y1) - (y1 - y2) * (x3 - x1)), p = ((x1 - x2) * (y4 - y1) - (y1 - y2) * (x4 - x1));
	if ((s * p) <= 0)
	{
		s = ((x3 - x4) * (y1 - y3) - (y3 - y4) * (x1 - x3));
		p = ((x3 - x4) * (y2 - y3) - (y3 - y4) * (x2 - x3));
		if ((s * p) <= 0)
			return true;
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////
// distanceOfPointFromLine
//
//	returns the distance of a point from a line
//	x1, y1, x2, y2: determine the position of the line
//	x3, y3: determine the position of the point
//
///////////////////////////////////////////////////////////////////////////////////////////

function distanceOfPointFromLine (x1, y1, x2, y2, x3, y3)
{
	if (x1 == x2)
	{
		return Math.abs(x3 - x1);
	}
	else if (y1 == y2)
	{
		return Math.abs(y3 - y1);
	}
	else
	{
		var m = (y1 - y2) / (x1 - x2);
		var b = y1 - m * x1;
		var m2 = sqrt(m * m + 1);
		return Math.abs((y3 - x3 * m - b)/m2);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// createRamp
//
//	creates a ramp from point (x1, y1) to (x2, y2).
//	x1, y1, x2, y2: determine the position of the start and end of the ramp
//	minHeight, maxHeight: determine the height levels of the start and end point
//	width: determines the width of the ramp
//	smoothLevel: determines the smooth level around the edges of the ramp
//	mainTerrain: (Optional) determines the terrain texture for the ramp
//	edgeTerrain: (Optional) determines the terrain texture for the edges
//	tileclass: (Optional) adds the ramp to this tile class
//
///////////////////////////////////////////////////////////////////////////////////////////

function createRamp (x1, y1, x2, y2, minHeight, maxHeight, width, smoothLevel, mainTerrain, edgeTerrain, tileclass)
{
	var halfWidth = width / 2;
	var mapSize = g_Map.size;

	if (y1 == y2)
	{
		var x3 = x2;
		var y3 = y2 + halfWidth;
	}
	else
	{
		var m = (x1 - x2) / (y1 - y2);
		var b = y2 + m * x2;
		var x3 = x2 + halfWidth;
		var y3 = - m * x3 + b;
	}

	var minBoundX = (x1 <= x2 ? (x1 > halfWidth ? x1 - halfWidth : 0) : (x2 > halfWidth ? x2 - halfWidth : 0));
	var maxBoundX = (x1 >= x2 ? (x1 < mapSize - halfWidth ? x1 + halfWidth : mapSize) : (x2 < mapSize - halfWidth ? x2 + halfWidth : mapSize));
	var minBoundY = (y1 <= y2 ? (y1 > halfWidth ? y1 - halfWidth : 0) : (y2 > halfWidth ? y2 - halfWidth : 0));
	var maxBoundY = (y1 >= y2 ? (x1 < mapSize - halfWidth ? y1 + halfWidth : mapSize) : (y2 < mapSize - halfWidth ? y2 + halfWidth : mapSize));

	for (var x = minBoundX; x < maxBoundX; ++x)
	{
		for (var y = minBoundY; y < maxBoundY; ++y)
		{
			var lDist = distanceOfPointFromLine(x3, y3, x2, y2, x, y);
			var sDist = distanceOfPointFromLine(x1, y1, x2, y2, x, y);
			var rampLength = sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
			if (lDist <= rampLength && sDist <= halfWidth)
			{
				var h = ((rampLength - lDist) * maxHeight + lDist * minHeight) / rampLength;
				if (sDist >= halfWidth - smoothLevel)
				{
					h = (h - minHeight) * (halfWidth - sDist) / smoothLevel + minHeight;
					if (edgeTerrain !== undefined)
						placeTerrain(x, y, edgeTerrain);
				}
				else
				{
					if (mainTerrain !== undefined)
						placeTerrain(x, y, mainTerrain);
				}
				if (tileclass !== undefined)
					addToClass(x, y, tileclass);
				if((g_Map.getHeight(floor(x), floor(y)) < h) && (h <= maxHeight))
					g_Map.setHeight(x, y, h);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// createMountain
//
//	creates a mountain using a tecnique very similar to chain placer.
//
///////////////////////////////////////////////////////////////////////////////////////////

function createMountain(maxHeight, minRadius, maxRadius, numCircles, constraint, x, z, terrain, tileclass, fcc, q)
{
	fcc = (fcc !== undefined ? fcc : 0);
	q = (q !== undefined ? q : []);

	// checking for an array of constraints
	if (constraint instanceof Array)
	{
		var constraintArray = constraint;
		constraint = new AndConstraint(constraintArray);
	}

	// Preliminary bounds check
	if (!g_Map.inMapBounds(x, z) || !constraint.allows(x, z))
	{
		return;
	}

	var size = getMapSize();
	var queueEmpty = (q.length ? false : true);

	var gotRet = new Array(size);
	for (var i = 0; i < size; ++i)
	{
		gotRet[i] = new Array(size);
		for (var j = 0; j < size; ++j)
		{
			gotRet[i][j] = -1;
		}
	}

	--size;

	if (minRadius < 1) minRadius = 1;
	if (minRadius > maxRadius) minRadius = maxRadius;

	var edges = [[x, z]];
	var circles = [];

	for (var i = 0; i < numCircles; ++i)
	{
		var badPoint = false;
		var [cx, cz] = pickRandom(edges);

		if (queueEmpty)
			var radius = randIntInclusive(minRadius, maxRadius);
		else
		{
			var radius = q.pop();
			queueEmpty = (q.length ? false : true);
		}

		var sx = cx - radius, lx = cx + radius;
		var sz = cz - radius, lz = cz + radius;

		sx = (sx < 0 ? 0 : sx);
		sz = (sz < 0 ? 0 : sz);
		lx = (lx > size ? size : lx);
		lz = (lz > size ? size : lz);

		var radius2 = radius * radius;
		var dx, dz, distance2;

		//log (uneval([sx, sz, lx, lz]));

		for (var ix = sx; ix <= lx; ++ix)
		{
			for (var iz = sz; iz <= lz; ++ iz)
			{
				dx = ix - cx;
				dz = iz - cz;
				distance2 = dx * dx + dz * dz;
				if (dx * dx + dz * dz <= radius2)
				{
					if (g_Map.inMapBounds(ix, iz))
					{
						if (!constraint.allows(ix, iz))
						{
							badPoint = true;
							break;
						}

						var state = gotRet[ix][iz];

						if (state == -1)
						{
							gotRet[ix][iz] = -2;
						}
						else if (state >= 0)
						{

							var s = edges.splice(state, 1);

							gotRet[ix][iz] = -2;

							var edgesLength = edges.length;
							for (var k = state; k < edges.length; ++k)
							{
								--gotRet[edges[k][0]][edges[k][1]];
							}
						}
					}
				}
			}
			if (badPoint)
				break;
		}

		if (badPoint)
			continue;
		else
			circles.push([cx, cz, radius]);


		for (var ix = sx; ix <= lx; ++ix)
		{
			for (var iz = sz; iz <= lz; ++ iz)
			{
				if (fcc)
					if ((x - ix) > fcc || (ix - x) > fcc || (z - iz) > fcc || (iz - z) > fcc)
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
	}

	var numFinalCircles = circles.length;

	for (var i = 0; i < numFinalCircles; ++i)
	{
		var point = circles[i];
		var cx = point[0], cz = point[1], radius = point[2];

		var sx = cx - radius, lx = cx + radius;
		var sz = cz - radius, lz = cz + radius;

		sx = (sx < 0 ? 0 : sx);
		sz = (sz < 0 ? 0 : sz);
		lx = (lx > size ? size : lx);
		lz = (lz > size ? size : lz);

		var radius2 = radius * radius;
		var dx, dz, distance2;

		var clumpHeight = radius / maxRadius * maxHeight * randFloat(0.8, 1.2);

		for (var ix = sx; ix <= lx; ++ix)
		{
			for (var iz = sz; iz <= lz; ++ iz)
			{
				dx = ix - cx;
				dz = iz - cz;
				distance2 = dx * dx + dz * dz;

				var newHeight = Math.round((Math.sin(PI * (2 * ((radius - Math.sqrt(distance2)) / radius) / 3 - 1/6)) + 0.5) * 2/3 * clumpHeight) + randIntInclusive(0, 2);

				if (dx * dx + dz * dz <= radius2)
				{
					if (g_Map.getHeight(ix, iz) < newHeight)
						g_Map.setHeight(ix, iz, newHeight);
					else if (g_Map.getHeight(ix, iz) >= newHeight && g_Map.getHeight(ix, iz) < newHeight + 4)
						g_Map.setHeight(ix, iz, newHeight + 4);
					if (terrain !== undefined)
						placeTerrain(ix, iz, terrain);
					if (tileclass !== undefined)
						addToClass(ix, iz, tileclass);
				}
			}
		}
	}
}
