// Object type constants

const  
TYPE_RECTPLACER = 1,
TYPE_TERRAINPAINTER = 2,
TYPE_NULLCONSTRAINT = 3;

// Utility functions

function println(x) {
	print(x);
	print("\n");
}

function chooseRand() {
	if(arguments.length==0) {
		error("chooseRand: requires at least 1 argument");
	}
	var ar = (arguments.length==1 ? arguments[0] : arguments);
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