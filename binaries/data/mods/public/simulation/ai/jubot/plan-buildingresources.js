// THIS CLASS IS FOR BUILDING FARMS AND ANIMAL PENS ONLY
var BuildingConstructionPlanResources = Class({

	_init: function(gameState, type, indno)
	{
		this.type = gameState.applyCiv(type);

		var template = gameState.getTemplate(this.type);
		if (!template)
		{
			this.invalidTemplate = true;
			return;
		}

		this.cost = new Resources(template.cost());
	},

	canExecute: function(gameState)
	{
		if (this.invalidTemplate)
			return false;

		// TODO: verify numeric limits etc

		var builders = gameState.findBuilders(this.type);

		return (builders.length != 0);
	},

	execute: function(gameState)
	{
//		warn("Executing BuildingConstructionPlan "+uneval(this));

		var builders = gameState.findBuilders(this.type).toEntityArray();

		// We don't care which builder we assign, since they won't actually
		// do the building themselves - all we care about is that there is
		// some unit that can start the foundation

		var pos = this.findGoodPosition(gameState);

		builders[0].construct(this.type, pos.x, pos.z, pos.angle);
	},

	getCost: function()
	{
		return this.cost;
	},

	/**
	 * Make each cell's 16-bit value at least one greater than each of its
	 * neighbours' values. (If the grid is initialised with 0s and 65535s,
	 * the result of each cell is its Manhattan distance to the nearest 0.)
	 *
	 * TODO: maybe this should be 8-bit (and clamp at 255)?
	 */
	expandInfluences: function(grid, w, h)
	{
		for (var y = 0; y < h; ++y)
		{
			var min = 65535;
			for (var x = 0; x < w; ++x)
			{
				var g = grid[x + y*w];
				if (g > min) grid[x + y*w] = min;
				else if (g < min) min = g;
				++min;
			}

			for (var x = w-2; x >= 0; --x)
			{
				var g = grid[x + y*w];
				if (g > min) grid[x + y*w] = min;
				else if (g < min) min = g;
				++min;
			}
		}

		for (var x = 0; x < w; ++x)
		{
			var min = 65535;
			for (var y = 0; y < h; ++y)
			{
				var g = grid[x + y*w];
				if (g > min) grid[x + y*w] = min;
				else if (g < min) min = g;
				++min;
			}

			for (var y = h-2; y >= 0; --y)
			{
				var g = grid[x + y*w];
				if (g > min) grid[x + y*w] = min;
				else if (g < min) min = g;
				++min;
			}
		}
	},

	/**
	 * Add a circular linear-falloff shape to a grid.
	 */
	addInfluence: function(grid, w, h, cx, cy, maxDist)
	{
		var x0 = Math.max(0, cx - maxDist);
		var y0 = Math.max(0, cy - maxDist);
		var x1 = Math.min(w, cx + maxDist);
		var y1 = Math.min(h, cy + maxDist);
		for (var y = y0; y < y1; ++y)
		{
			for (var x = x0; x < x1; ++x)
			{
				var dx = x - cx;
				var dy = y - cy;
				var r = Math.sqrt(dx*dx + dy*dy);
				if (r < maxDist)
					grid[x + y*w] += maxDist - r;
			}
		}
	},

	/**
	 * Add a circular linear-falloff shape to a grid.
	 */
	subtractInfluence: function(grid, w, h, cx, cy, maxDist)
	{
		var x0 = Math.max(0, cx - maxDist);
		var y0 = Math.max(0, cy - maxDist);
		var x1 = Math.min(w, cx + maxDist);
		var y1 = Math.min(h, cy + maxDist);
		for (var y = y0; y < y1; ++y)
		{
			for (var x = x0; x < x1; ++x)
			{
				var dx = x - cx;
				var dy = y - cy;
				var r = Math.sqrt(dx*dx + dy*dy);
				if (r < maxDist)
					grid[x + y*w] -= maxDist - r;
			}
		}
	},
	
	findGoodPosition: function(gameState)
	{
		var self = this;

		var cellSize = 4; // size of each tile

		var template = gameState.getTemplate(this.type);

		// Find all tiles in valid territory that are far enough away from obstructions:
		var passabilityMap = gameState.getPassabilityMap();
		var territoryMap = gameState.getTerritoryMap();
		const TERRITORY_PLAYER_MASK = 0x7F;
		var obstructionMask = gameState.getPassabilityClassMask("foundationObstruction");

		if (passabilityMap.data.length != territoryMap.data.length)
			error("passability and territory data are not matched!");

		// See BuildRestrictions.js
		switch(template.buildPlacementType())
		{
		case "shore":
			obstructionMask |= gameState.getPassabilityClassMask("building-shore");
			break;
		case "land":
		default:
			obstructionMask |= gameState.getPassabilityClassMask("building-land");
		}

		var playerID = gameState.getPlayerID();
		var buildOwn = template.hasBuildTerritory("own");
		var buildAlly = template.hasBuildTerritory("ally");
		var buildNeutral = template.hasBuildTerritory("neutral");
		var buildEnemy = template.hasBuildTerritory("enemy");

		var obstructionTiles = new Uint16Array(passabilityMap.data.length);
		for (var i = 0; i < passabilityMap.data.length; ++i)
		{
			var tilePlayer = (territoryMap.data[i] & TERRITORY_PLAYER_MASK);
			var invalidTerritory = (
				(!buildOwn && tilePlayer == playerID) ||
				(!buildAlly && gameState.isPlayerAlly(tilePlayer) && tilePlayer != playerID) ||
				(!buildNeutral && tilePlayer == 0) ||
				(!buildEnemy && gameState.isPlayerEnemy(tilePlayer) && tilePlayer !=0)
			);
			obstructionTiles[i] = (invalidTerritory || (passabilityMap.data[i] & obstructionMask)) ? 0 : 65535;
		}

//		Engine.DumpImage("tiles0.png", obstructionTiles, passabilityMap.width, passabilityMap.height, 64);

		this.expandInfluences(obstructionTiles, passabilityMap.width, passabilityMap.height);

		// TODO: handle distance restrictions for e.g. CivCentres
		
		// Compute each tile's closeness to friendly structures:

		var friendlyTiles = new Uint16Array(passabilityMap.data.length);

		gameState.getOwnEntities().forEach(function(ent) {
			if (ent.hasClass("Structure"))
			{
				var infl = 0;
				if (ent.hasClass("CivCentre"))
				{
					infl = 5;
				}
				else if (ent.hasClass("DropsiteFood"))
				{
					infl = 40;
				}

				var pos = ent.position();
				var x = Math.round(pos[0] / cellSize);
				var z = Math.round(pos[1] / cellSize);
				self.addInfluence(friendlyTiles, passabilityMap.width, passabilityMap.height, x, z, infl);
			}
		});

//			var foetargets = gameState.entities.filter(function(ent) {
//				return (ent.isEnemy());
//			});
//			foetargets.forEach(function(ent) {
//			if (ent.hasClass("CivCentre"))
//			{
//				var infl = 100;
//				var pos = ent.position();
//				var x = Math.round(pos[0] / cellSize);
//				var z = Math.round(pos[1] / cellSize);
//				self.subtractInfluence(friendlyTiles, passabilityMap.width, passabilityMap.height, x, z, infl);
//			}
//		});

		
		// Find target building's approximate obstruction radius,
		// and expand by a bit to make sure we're not too close
		var radius = Math.ceil(template.obstructionRadius() / cellSize) + 1;

		// Find the best non-obstructed tile
		var bestIdx = 0;
		var bestVal = -1;
		for (var i = 0; i < passabilityMap.data.length; ++i)
		{
			if (obstructionTiles[i] > radius)
			{
				var v = friendlyTiles[i];
				//var foe = enemyTiles[i];
			//JuBotAI.prototype.chat(v);
			//JuBotAI.prototype.chat(i);
			//JuBotAI.prototype.chat(foe);
				if (v >= bestVal)
				{
					bestVal = v;
					bestIdx = i;
			//JuBotAI.prototype.chat("BestVal is " + bestVal + ", and bestIdx is " + bestIdx + ".");
				}
			}
		}
		var x = ((bestIdx % passabilityMap.width) + 0.5) * cellSize;
		var z = (Math.floor(bestIdx / passabilityMap.width) + 0.5) * cellSize;

//		Engine.DumpImage("tiles1.png", obstructionTiles, passabilityMap.width, passabilityMap.height, 32);
//		Engine.DumpImage("tiles2.png", friendlyTiles, passabilityMap.width, passabilityMap.height, 256);

		// TODO: special dock placement requirements

		// Fixed angle to match fixed starting cam
		var angle = 0.75*Math.PI;

		return {
			"x": x,
			"z": z,
			"angle": angle
		};
	},
});
