const ELEVATION_SET = 0;
const ELEVATION_MODIFY = 1;

/////////////////////////////////////////////////////////////////////////////
//	ElevationPainter
/////////////////////////////////////////////////////////////////////////////
	
function ElevationPainter(elevation)
{
	this.elevation = elevation;
	this.DX = [0, 1, 1, 0];
	this.DY = [0, 0, 1, 1];
}

ElevationPainter.prototype.paint = function(area)
{
	var length = area.points.length;
	var elevation = this.elevation;
	
	for (var i=0; i < length; i++)
	{
		var pt = area.points[i];
		
		for (var j=0; j < 4; j++)
		{
			g_Map.height[pt.x+this.DX[j]][pt.y+this.DY[j]] = elevation;
		}
	}
};

/////////////////////////////////////////////////////////////////////////////
//	LayeredPainter
/////////////////////////////////////////////////////////////////////////////

function LayeredPainter(terrainArray, widths)
{
	if (!(terrainArray instanceof Array))
		error("terrains must be an array!");
	
	this.terrains = [];
	for (var i = 0; i < terrainArray.length; ++i)
		this.terrains.push(createTerrain(terrainArray[i]));
	
	this.widths = widths;
}

LayeredPainter.prototype.paint = function(area)
{
	var size = getMapSize();
	var saw = new Array(size);
	var dist = new Array(size);
	
	// init typed arrays
	for (var i = 0; i < size; ++i)
	{
		saw[i] = new Uint8Array(size);		// bool / uint8
		dist[i] = new Uint16Array(size);		// uint16
	}

	// Point queue (implemented with array)
	var pointQ = [];

	// push edge points
	var pts = area.points;
	var length = pts.length;
	
	for (var i=0; i < length; i++)
	{
		var x = pts[i].x;
		var y = pts[i].y;
		
		for (var dx=-1; dx <= 1; dx++)
		{
			var nx = x+dx;
			for (var dy=-1; dy <= 1; dy++)
			{
				var ny = y+dy;
				
				if (g_Map.validT(nx, ny) && g_Map.area[nx][ny] != area && !saw[nx][ny])
				{
					saw[nx][ny] = 1;
					dist[nx][ny] = 0;
					pointQ.push(new Point(nx, ny));
				}
			}
		}
	}
	
	// do BFS inwards to find distances to edge
	while (pointQ.length)
	{
		var pt = pointQ.shift();	// Pop queue
		var px = pt.x;
		var py = pt.y;
		var d = dist[px][py];

		// paint if in area
		if (g_Map.area[px][py] == area)
		{
			var w=0;
			var i=0;
			
			for (; i < this.widths.length; i++)
			{
				w += this.widths[i];
				if (w >= d)
				{
					break;
				}
			}
			this.terrains[i].place(px, py);
		}

		// enqueue neighbours
		for (var dx=-1; dx<=1; dx++)
		{
			var nx = px+dx;
			for (var dy=-1; dy<=1; dy++)
			{
				var ny = py+dy;
				
				if (g_Map.validT(nx, ny) && g_Map.area[nx][ny] == area && !saw[nx][ny])
				{
					saw[nx][ny] = 1;
					dist[nx][ny] = d+1;
					pointQ.push(new Point(nx, ny));
				}
			}
		}
	}
};

/////////////////////////////////////////////////////////////////////////////
//	MultiPainter
/////////////////////////////////////////////////////////////////////////////
	
function MultiPainter(painters)
{
	this.painters = painters;
}

MultiPainter.prototype.paint = function(area)
{
	for (var i=0; i < this.painters.length; i++)
	{
		this.painters[i].paint(area);
	}
};

/////////////////////////////////////////////////////////////////////////////
//	SmoothElevationPainter
/////////////////////////////////////////////////////////////////////////////

function SmoothElevationPainter(type, elevation, blendRadius)
{
	this.type = type;
	this.elevation = elevation;
	this.blendRadius = blendRadius;
	
	if (type != ELEVATION_SET && type != ELEVATION_MODIFY)
		error("SmoothElevationPainter: invalid type '"+type+"'");
}

SmoothElevationPainter.prototype.checkInArea = function(area, x, y)
{
	if (g_Map.validT(x, y))
	{
		return (g_Map.area[x][y] == area);
	}
	else
	{
		return false;
	}
};

SmoothElevationPainter.prototype.paint = function(area)
{
	var pointQ = [];
	var pts = area.points;
	var heightPts = [];
	
	var mapSize = getMapSize()+1;
	
	var saw = new  Array(mapSize);
	var dist = new Array(mapSize);
	var gotHeightPt = new Array(mapSize);
	var newHeight = new Array(mapSize);
	
	// init typed arrays
	for (var i = 0; i < mapSize; ++i)
	{
		saw[i] = new Uint8Array(mapSize);		// bool / uint8
		dist[i] = new Uint16Array(mapSize);		// uint16
		gotHeightPt[i] = new Uint8Array(mapSize);	// bool / uint8
		newHeight[i] = new Float32Array(mapSize);	// float32
	}
	
	var length = pts.length;
	
	// get a list of all points
	for (var i=0; i < length; i++)
	{
		var x = pts[i].x;
		var y = pts[i].y;
		
		for (var dx=-1; dx <= 2; dx++)
		{
			var nx = x+dx;
			for (var dy=-1; dy <= 2; dy++)
			{
				var ny = y+dy;
				
				if (g_Map.validH(nx, ny) && !gotHeightPt[nx][ny])
				{
					gotHeightPt[nx][ny] = 1;
					heightPts.push(new Point(nx, ny));
					newHeight[nx][ny] = g_Map.height[nx][ny];
				}
			}
		}
	}

	// push edge points
	for (var i=0; i < length; i++)
	{
		var x = pts[i].x, y = pts[i].y;
		for (var dx=-1; dx <= 2; dx++)
		{
			var nx = x+dx;
			for (var dy=-1; dy <= 2; dy++)
			{
				var ny = y+dy;
				
				if (g_Map.validH(nx, ny) 
					&& !this.checkInArea(area, nx, ny)
					&& !this.checkInArea(area, nx-1, ny)
					&& !this.checkInArea(area, nx, ny-1)
					&& !this.checkInArea(area, nx-1, ny-1)
					&& !saw[nx][ny])
				{
					saw[nx][ny]= 1;
					dist[nx][ny] = 0;
					pointQ.push(new Point(nx, ny));
				}
			}
		}
	}

	// do BFS inwards to find distances to edge
	while(pointQ.length)
	{
		var pt = pointQ.shift();
		var px = pt.x;
		var py = pt.y;
		var d = dist[px][py];

		// paint if in area
		if (g_Map.validH(px, py)
			&& (this.checkInArea(area, px, py) || this.checkInArea(area, px-1, py) 
			|| this.checkInArea(area, px, py-1) || this.checkInArea(area, px-1, py-1)))
		{
			if (d <= this.blendRadius)
			{
				var a = (d-1) / this.blendRadius;
				if (this.type == ELEVATION_SET)
				{
					newHeight[px][py] = a*this.elevation + (1-a)*g_Map.height[px][py];
				}
				else
				{	// type == MODIFY
					newHeight[px][py] += a*this.elevation;
				}
			}
			else
			{	// also happens when blendRadius == 0
				if (this.type == ELEVATION_SET)
				{
					newHeight[px][py] = this.elevation;
				}
				else
				{	// type == MODIFY
					newHeight[px][py] += this.elevation;
				}
			}
		}

		// enqueue neighbours
		for (var dx=-1; dx <= 1; dx++)
		{
			var nx = px+dx;
			for (var dy=-1; dy <= 1; dy++)
			{
				var ny = py+dy;
				
				if (g_Map.validH(nx, ny) 
					&& (this.checkInArea(area, nx, ny) || this.checkInArea(area, nx-1, ny) 
						|| this.checkInArea(area, nx, ny-1) || this.checkInArea(area, nx-1, ny-1))
					&& !saw[nx][ny])
				{
					saw[nx][ny] = 1;
					dist[nx][ny] = d+1;
					pointQ.push(new Point(nx, ny));
				}
			}
		}
	}

	length = heightPts.length;
	
	// smooth everything out
	for (var i = 0; i < length; ++i)
	{
		var pt = heightPts[i];
		var px = pt.x;
		var py = pt.y;
		
		if ((this.checkInArea(area, px, py) || this.checkInArea(area, px-1, py) 
			|| this.checkInArea(area, px, py-1) || this.checkInArea(area, px-1, py-1)))
		{
			var sum = 8 * newHeight[px][py];
			var count = 8;
			
			for (var dx=-1; dx <= 1; dx++)
			{
				var nx = px+dx;
				for (var dy=-1; dy <= 1; dy++)
				{
					var ny = py+dy;
					
					if (g_Map.validH(nx, ny))
					{
						sum += newHeight[nx][ny];
						count++;
					}
				}
			}
			
			g_Map.height[px][py] = sum/count;
		}
	}
};

/////////////////////////////////////////////////////////////////////////////
//	TerrainPainter
/////////////////////////////////////////////////////////////////////////////

function TerrainPainter(terrain)
{
	this.terrain = createTerrain(terrain);
}

TerrainPainter.prototype.paint = function(area)
{
	var length = area.points.length;
	for (var i=0; i < length; i++)
	{
		var pt = area.points[i];
		this.terrain.place(pt.x, pt.y);
	}
};

/////////////////////////////////////////////////////////////////////////////
//	TileClassPainter
/////////////////////////////////////////////////////////////////////////////

function TileClassPainter(tileClass)
{
	this.tileClass = tileClass;
}

TileClassPainter.prototype.paint = function(area)
{
	var length = area.points.length;
	for (var i=0; i < length; i++)
	{
		var pt = area.points[i];
		this.tileClass.add(pt.x, pt.y);
	}
};
