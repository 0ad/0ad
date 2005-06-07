// Object type constants

const 
TYPE_RECTPLACER = 1,
TYPE_TERRAINPAINTER = 2,
TYPE_NULLCONSTRAINT = 3,
TYPE_RANDOMTERRAIN = 4,
TYPE_LAYEREDPAINTER = 5,
TYPE_AVOIDAREACONSTRAINT = 6,
TYPE_CLUMPPLACER = 7,
TYPE_AVOIDTERRAINCONSTRAINT = 8,
TYPE_ANDCONSTRAINT = 9;

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
	this.x1 = x1;
	this.y1 = y1;
	this.x2 = x2;
	this.y2 = y2;
	this.raw = function() {
		return [TYPE_RECTPLACER, this.x1, this.y1, this.x2, this.y2];
	}
}

function TerrainPainter(terrain) {
	this.terrain = terrain;
	this.raw = function() {
		return [TYPE_TERRAINPAINTER, this.terrain];
	}
}

function NullConstraint() {
	this.raw = function() {
		return [TYPE_NULLCONSTRAINT];
	}
}

function RandomTerrain() {
	this.terrains = argsToArray(arguments);
	this.raw = function() {
		return [TYPE_RANDOMTERRAIN, this.terrains];
	}
}

function LayeredPainter(widths, terrains) {
	this.widths = widths;
	this.terrains = terrains;
	this.raw = function() {
		return [TYPE_LAYEREDPAINTER, this.widths, this.terrains];
	}
}

function AvoidAreaConstraint(area) {
	this.area = area;
	this.raw = function() {
		return [TYPE_AVOIDAREACONSTRAINT, this.area];
	}
}

function ClumpPlacer(size, coherence, smoothness, x, y) {
	this.size = size;
	this.coherence = coherence;
	this.smoothness = smoothness;
	this.x = x ? x : -1;
	this.y = y ? y : -1;
	this.raw = function() {
		return [TYPE_CLUMPPLACER, this.size, this.coherence, this.smoothness, this.x, this.y];
	}
}

function AvoidTerrainConstraint(texture) {
	this.texture = texture;
	this.raw = function() {
		return [TYPE_AVOIDTERRAINCONSTRAINT, this.texture];
	}
}

function AndConstraint(a, b) {
	this.a = a;
	this.b = b;
	this.raw = function() {
		return [TYPE_ANDCONSTRAINT, this.a, this.b];
	}
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
	return (good==num) ? ret : null;
}
