// Utility function used in both noises as an ease curve
function easeCurve(t)
{
	return t*t*t*(t*(t*6-15)+10);
}

// Find mod of number but only positive values
function modPos(num, m)
{
	var p = num % m;
	if (p < 0)
		p += m;

	return p;
}

/////////////////////////////////////////////////////////////////////
//	Noise2D
//
//	Class representing 2D noise with a given base frequency
//
/////////////////////////////////////////////////////////////////////

function Noise2D(freq)
{
	freq = Math.floor(freq);
	this.freq = freq;
	this.grads = [];

	for (var i=0; i < freq; ++i)
	{
		this.grads[i] = [];
		for (var j=0; j < freq; ++j)
		{
			var a = randFloat(0, 2 * PI);
			this.grads[i][j] = new Vector2D(Math.cos(a), Math.sin(a));
		}
	}
}

Noise2D.prototype.get = function(x, y)
{
	x *= this.freq;
	y *= this.freq;

	var ix = modPos(Math.floor(x), this.freq);
	var iy = modPos(Math.floor(y), this.freq);

	var fx = x - ix;
	var fy = y - iy;

	var ix1 = (ix+1) % this.freq;
	var iy1 = (iy+1) % this.freq;

	var s = this.grads[ix][iy].dot(new Vector2D(fx, fy));
	var t = this.grads[ix1][iy].dot(new Vector2D(fx-1, fy));
	var u = this.grads[ix][iy1].dot(new Vector2D(fx, fy-1));
	var v = this.grads[ix1][iy1].dot(new Vector2D(fx-1, fy-1));

	var ex = easeCurve(fx);
	var ey = easeCurve(fy);
	var a = s + ex*(t-s);
	var b = u + ex*(v-u);
	return (a + ey*(b-a)) * 0.5 + 0.5;
};

/////////////////////////////////////////////////////////////////////
//	Noise3D
//
//	Class representing 3D noise with given base frequencies
//
/////////////////////////////////////////////////////////////////////

function Noise3D(freq, vfreq)
{
	freq = Math.floor(freq);
	vfreq = Math.floor(vfreq);
	this.freq = freq;
	this.vfreq = vfreq;
	this.grads = [];

	for (var i=0; i < freq; ++i)
	{
		this.grads[i] = [];
		for (var j=0; j < freq; ++j)
		{
			this.grads[i][j] = [];
			for(var k=0; k < vfreq; ++k)
			{
				var v = new Vector3D();
				do
				{
					v.set(randFloat(-1, 1), randFloat(-1, 1), randFloat(-1, 1));
				}
				while(v.lengthSquared() > 1 || v.lengthSquared() < 0.1);

				v.normalize();

				this.grads[i][j][k] = v;
			}
		}
	}
}

Noise3D.prototype.get = function(x, y, z)
{
	x *= this.freq;
	y *= this.freq;
	z *= this.vfreq;

	var ix =modPos(Math.floor(x), this.freq);
	var iy = modPos(Math.floor(y), this.freq);
	var iz = modPos(Math.floor(z), this.vfreq);

	var fx = x - ix;
	var fy = y - iy;
	var fz = z - iz;

	var ix1 = (ix+1) % this.freq;
	var iy1 = (iy+1) % this.freq;
	var iz1 = (iz+1) % this.vfreq;

	var s0 = this.grads[ix][iy][iz].dot(new Vector3D(fx, fy, fz));
	var t0 = this.grads[ix1][iy][iz].dot(new Vector3D(fx-1, fy, fz));
	var u0 = this.grads[ix][iy1][iz].dot(new Vector3D(fx, fy-1, fz));
	var v0 = this.grads[ix1][iy1][iz].dot(new Vector3D(fx-1, fy-1, fz));

	var s1 = this.grads[ix][iy][iz1].dot(new Vector3D(fx, fy, fz-1));
	var t1 = this.grads[ix1][iy][iz1].dot(new Vector3D(fx-1, fy, fz-1));
	var u1 = this.grads[ix][iy1][iz1].dot(new Vector3D(fx, fy-1, fz-1));
	var v1 = this.grads[ix1][iy1][iz1].dot(new Vector3D(fx-1, fy-1, fz-1));

	var ex = easeCurve(fx);
	var ey = easeCurve(fy);
	var ez = easeCurve(fz);

	var a0 = s0 + ex*(t0-s0);
	var b0 = u0 + ex*(v0-u0);
	var c0 = a0 + ey*(b0-a0);

	var a1 = s1 + ex*(t1-s1);
	var b1 = u1 + ex*(v1-u1);
	var c1 = a1 + ey*(b1-a1);

	return (c0 + ez*(c1-c0)) * 0.5 + 0.5;
};
