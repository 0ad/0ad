// Object type constants

const 
TYPE_RECTPLACER = 1,
TYPE_TERRAINPAINTER = 2,
TYPE_NULLCONSTRAINT = 3,
TYPE_LAYEREDPAINTER = 4,
TYPE_AVOIDAREACONSTRAINT = 5,
TYPE_CLUMPPLACER = 6,
TYPE_AVOIDTEXTURECONSTRAINT = 7,
TYPE_ELEVATIONPAINTER = 8;

// Utility functions

function println(x) {
	print(x);
	print("\n");
}

function argsToArray(x) {
	if(x.length!=1) {
		var ret = new Array();
		for(var i=0; i<x.length; i++) {
			ret[i] = x[i];
		}
		return ret;
	}
	else {
		return x[0];
	}
}

function chooseRand() {
	if(arguments.length==0) {
		error("chooseRand: requires at least 1 argument");
	}
	var ar = argsToArray(arguments);
	return ar[randInt(ar.length)];
}

// Area placers

function RectPlacer(x1, y1, x2, y2) {
	this.TYPE = TYPE_RECTPLACER;
	this.x1 = x1;
	this.y1 = y1;
	this.x2 = x2;
	this.y2 = y2;
}

function TerrainPainter(terrain) {
	this.TYPE = TYPE_TERRAINPAINTER;
	this.terrain = terrain;
}

function NullConstraint() {
	this.TYPE = TYPE_NULLCONSTRAINT;
}

function LayeredPainter(widths, terrains) {
	this.TYPE = TYPE_LAYEREDPAINTER;
	this.widths = widths;
	this.terrains = terrains;
}

function AvoidAreaConstraint(area) {
	this.TYPE = TYPE_AVOIDAREACONSTRAINT;
	this.area = area;
}

function ClumpPlacer(size, coherence, smoothness, x, y) {
	this.TYPE = TYPE_CLUMPPLACER;
	this.size = size;
	this.coherence = coherence;
	this.smoothness = smoothness;
	this.x = x ? x : -1;
	this.y = y ? y : -1;
}

function AvoidTextureConstraint(texture) {
	this.TYPE = TYPE_AVOIDTEXTURECONSTRAINT;
	this.texture = texture;
}

function ElevationPainter(elevation) {
	this.TYPE = TYPE_ELEVATIONPAINTER;
	this.elevation = elevation;
}

function createMulti(centeredPlacer, painter, constraint, num, maxFail) {
	var good = 0;
	var bad = 0;
	var ret = new Array();
	while(good < num && bad <= maxFail) {
		centeredPlacer.x = randInt(SIZE);
		centeredPlacer.y = randInt(SIZE);
		var r = createArea(centeredPlacer, painter, constraint);
		if(r) {
			good++;
			ret[ret.length] = r;
		}
		else {
			bad++;
		}
	}
	return ret;
}
