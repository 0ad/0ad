
/////////////////////////////////////////////////////////////////////////////////////////
//	PathPlacer
//
//	Class for creating a winding path between two points
//
//	x1,z1: 	Starting point of path
//	x2,z2: 	Ending point of path
//	width: 	Width of the path
//	a:		Waviness - how wavy the path will be (higher is wavier, 0.0 is straight line)
//	b:		Smoothness - how smooth the path will be (higher is smoother)
//	c:		Offset - max amplitude of waves along the path (0.0 is straight line)
// 	taper:	Tapering - how much the width of the path changes from start to end
//				if positive, the width will decrease by that factor, if negative the width
//				will increase by that factor
//
/////////////////////////////////////////////////////////////////////////////////////////

function PathPlacer(x1, z1, x2, z2, width, a, b, c, taper, failfraction)
{
	this.x1 = x1;
	this.z1 = z1;
	this.x2 = x2;
	this.z2 = z2;
	this.width = width;
	this.a = a;
	this.b = b;
	this.c = c;
	this.taper = taper;
	this.failfraction = (failfraction !== undefined ? failfraction : 5);
}

PathPlacer.prototype.place = function(constraint)
{
	/*/ Preliminary bounds check
	if (!g_Map.validT(this.x1, this.z1) || !constraint.allows(this.x1, this.z1) ||
		!g_Map.validT(this.x2, this.z2) || !constraint.allows(this.x2, this.z2))
	{
		return undefined;
	}*/

	var failed = 0;
	var dx = (this.x2 - this.x1);
	var dz = (this.z2 - this.z1);
	var dist = Math.sqrt(dx*dx + dz*dz);
	dx /= dist;
	dz /= dist;

	var numSteps = 1 + Math.floor(dist/4 * this.a);
	var numISteps = 1 + Math.floor(dist/4 * this.b);
	var totalSteps = numSteps*numISteps;
	var offset = 1 + Math.floor(dist/4 * this.c);

	var size = getMapSize();
	var gotRet = [];
	for (var i = 0; i < size; ++i)
		gotRet[i] = new Uint8Array(size);			// bool / uint8

	// Generate random offsets
	var ctrlVals = new Float32Array(numSteps);		//float32
	for (var j = 1; j < (numSteps-1); ++j)
	{
		ctrlVals[j] = randFloat(-offset, offset);
	}

	// Interpolate for smoothed 1D noise
	var noise = new Float32Array(totalSteps+1);		//float32
	for (var j = 0; j < numSteps; ++j)
	{
		// Cubic interpolation
		var v0 = ctrlVals[(j+numSteps-1)%numSteps];
		var v1 = ctrlVals[j];
		var v2 = ctrlVals[(j+1)%numSteps];
		var v3 = ctrlVals[(j+2)%numSteps];
		var P = (v3 - v2) - (v0 - v1);
		var Q = (v0 - v1) - P;
		var R = v2 - v0;
		var S = v1;
		for (var k = 0; k < numISteps; ++k)
		{
			var t = k/numISteps;
			noise[j*numISteps + k] = P*t*t*t + Q*t*t + R*t + S;
		}
	}

	var halfWidth = 0.5 * this.width;

	// Add smoothed noise to straight path
	var segments1 = [];
	var segments2 = [];

	for (var j = 0; j < totalSteps; ++j)
	{
		// Interpolated points along straight path
		var t = j/totalSteps;
		var tx = this.x1 * (1.0 - t) + this.x2 * t;
		var tz = this.z1 * (1.0 - t) + this.z2 * t;
		var t2 = (j+1)/totalSteps;
		var tx2 = this.x1 * (1.0 - t2) + this.x2 * t2;
		var tz2 = this.z1 * (1.0 - t2) + this.z2 * t2;

		// Find noise offset points
		var nx = (tx - dz * noise[j]);
		var nz = (tz + dx * noise[j]);
		var nx2 = (tx2 - dz * noise[j+1]);
		var nz2 = (tz2 + dx * noise[j+1]);

		// Find slope of offset points
		var ndx = (nx2 - nx);
		var ndz = (nz2 - nz);
		var dist = Math.sqrt(ndx*ndx + ndz*ndz);
		ndx /= dist;
		ndz /= dist;

		var taperedWidth = (1.0 - t*this.taper) * halfWidth;

		// Find slope of offset path
		var px = Math.round(nx - ndz * -taperedWidth);
		var pz = Math.round(nz + ndx * -taperedWidth);
		segments1.push(new PointXZ(px, pz));
		var px2 = Math.round(nx2 - ndz * taperedWidth);
		var pz2 = Math.round(nz2 + ndx * taperedWidth);
		segments2.push(new PointXZ(px2, pz2));

	}

	var retVec = [];
	// Draw path segments
	var num = segments1.length - 1;
	for (var j = 0; j < num; ++j)
	{
		// Fill quad formed by these 4 points
		// Note the code assumes these points have been rounded to integer values
		var pt11 = segments1[j];
		var pt12 = segments1[j+1];
		var pt21 = segments2[j];
		var pt22 = segments2[j+1];

		var tris = [[pt12, pt11, pt21], [pt12, pt21, pt22]];

		for (var t = 0; t < 2; ++t)
		{
			// Sort vertices by min z
			var tri = tris[t].sort(
				function(a, b)
				{
					return a.z - b.z;
				}
			);

			// Fills in a line from (z, x1) to (z,x2)
			var fillLine = function(z, x1, x2)
			{
				var left = Math.round(Math.min(x1, x2));
				var right = Math.round(Math.max(x1, x2));
				for (var x = left; x <= right; x++)
				{
					if (constraint.allows(x, z))
					{
						if (g_Map.inMapBounds(x, z) && !gotRet[x][z])
						{
							retVec.push(new PointXZ(x, z));
							gotRet[x][z] = 1;
						}
					}
					else
					{
						failed++;
					}
				}
			};

			var A = tri[0];
			var B = tri[1];
			var C = tri[2];

			var dx1 = (B.z != A.z) ? ((B.x - A.x) / (B.z - A.z)) : 0;
			var dx2 = (C.z != A.z) ? ((C.x - A.x) / (C.z - A.z)) : 0;
			var dx3 = (C.z != B.z) ? ((C.x - B.x) / (C.z - B.z)) : 0;

			if (A.z == B.z)
			{
				fillLine(A.z, A.x, B.x);
			}
			else
			{
				for (var z = A.z; z <= B.z; z++)
				{
					fillLine(z, A.x + dx1*(z - A.z), A.x + dx2*(z - A.z));
				}
			}
			if (B.z == C.z)
			{
				fillLine(B.z, B.x, C.x);
			}
			else
			{
				for (var z = B.z + 1; z < C.z; z++)
				{
					fillLine(z, B.x + dx3*(z - B.z), A.x + dx2*(z - A.z));
				}
			}
		}
	}

	return ((failed > this.width*this.failfraction*dist) ? undefined : retVec);
};

