const UNIT_TEMPLATE = "units/athen_infantry_marine_archer_b";
const SHIP_TEMPLATE = "units/athen_ship_trireme";
const RAM_TEMPLATE = "units/brit_siege_ram";

const point_plaza_nw = 14; // Center plaza #1 (NW)
const point_plaza_ne = 15; // Center plaza #2 (NE)
const point_plaza_se = 16; // Center plaza #3 (SE)
const point_plaza_sw = 17; // Center plaza #4 (SW)
const point_road_1 = 18; // 'Road' from plaza to land end in the NW, #1
const point_road_2 = 19; // 'Road' from plaza to land end in the NW, #2
const point_road_3 = 20; // 'Road' from plaza to land end in the NW, #3
const point_road_4 = 21; // 'Road' from plaza to land end in the NW, #4
const point_road_5 = 22; // 'Road' from plaza to land end in the NW, #5 and last
const point_island_1 = 23; // Sand Island #1 (north)
const point_island_2 = 24; // Sand Island #2 (south)
const point_sea_1 = 25; // Sea #1 (west)
const point_sea_2 = 26; // Sea #2 (east)
const point_coast_1 = 27; // Coast point #1 (E)
const point_coast_2 = 28; // Coast point #2 (W)


var QuickSpawn = function(x, z, template, owner = 1)
{
	let ent = Engine.AddEntity(template);

	let cmpEntOwnership = Engine.QueryInterface(ent, IID_Ownership);
	if (cmpEntOwnership)
		cmpEntOwnership.SetOwner(owner);

	let cmpEntPosition = Engine.QueryInterface(ent, IID_Position);
	cmpEntPosition.JumpTo(x, z);
	return ent;
};

var Rotate = function(angle, ent)
{
	let cmpEntPosition = Engine.QueryInterface(ent, IID_Position);
	cmpEntPosition.SetYRotation(angle);
	return ent;
};

var WalkTo = function(x, z, ent, owner = 1)
{
	ProcessCommand(owner, {
		"type": "walk",
		"entities": Array.isArray(ent) ? ent : [ent],
		"x": x,
		"z": z,
		"queued": false
	});
	return ent;
};


var Do = function(name, data, ent, owner=1)
{
	let comm = {
		"type": name,
		"entities": Array.isArray(ent) ? ent : [ent],
		"queued": false
	};
	for (let k in data)
		comm[k] = data[k];
	ProcessCommand(owner, comm);
};

var cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);

Trigger.prototype.UnitsIntoRamClose = function()
{
	let pos = TriggerHelper.GetEntityPosition2D(point_plaza_nw);
	let ram = QuickSpawn(pos.x, pos.y, RAM_TEMPLATE, 1);
	for (let i = 0; i < 5; ++i)
	{
		pos = TriggerHelper.GetEntityPosition2D(point_plaza_ne);
		Do("garrison", { "target": ram }, QuickSpawn(pos.x+i, pos.y, UNIT_TEMPLATE));
	}
};


Trigger.prototype.UnitsIntoRamFar = function()
{
	let pos = TriggerHelper.GetEntityPosition2D(point_road_5);
	let ram = QuickSpawn(pos.x, pos.y, RAM_TEMPLATE, 1);
	for (let i = 0; i < 5; ++i)
	{
		pos = TriggerHelper.GetEntityPosition2D(point_road_1);
		Do("garrison", { "target": ram }, QuickSpawn(pos.x+i, pos.y, UNIT_TEMPLATE));
	}
};

Trigger.prototype.UnitsIntoRamOppositeDir = function()
{
	let pos = TriggerHelper.GetEntityPosition2D(point_plaza_sw);
	let ram = QuickSpawn(pos.x, pos.y, RAM_TEMPLATE, 1);
	let spawnpoints = [
		TriggerHelper.GetEntityPosition2D(point_plaza_se),
		TriggerHelper.GetEntityPosition2D(point_road_1)
	];
	for (let i = 0; i < 6; ++i)
	{
		pos = spawnpoints[i%2];
		Do("garrison", { "target": ram }, QuickSpawn(pos.x+i, pos.y, UNIT_TEMPLATE));
	}
};


Trigger.prototype.UnitsIntoRamImpossible = function()
{
	let pos = TriggerHelper.GetEntityPosition2D(point_island_1);
	let ram = QuickSpawn(pos.x, pos.y, RAM_TEMPLATE, 1);
	for (let i = 0; i < 3; ++i)
	{
		pos = TriggerHelper.GetEntityPosition2D(point_plaza_se);
		Do("garrison", { "target": ram }, QuickSpawn(pos.x+i, pos.y, UNIT_TEMPLATE));
	}
};


Trigger.prototype.UnitsIntoShipClose = function()
{
	let pos = TriggerHelper.GetEntityPosition2D(point_sea_1);
	let ship = QuickSpawn(pos.x, pos.y, SHIP_TEMPLATE, 1);
	for (let i = 0; i < 6; ++i)
	{
		pos = TriggerHelper.GetEntityPosition2D(point_plaza_se);
		Do("garrison", { "target": ship }, QuickSpawn(pos.x+i, pos.y, UNIT_TEMPLATE));
	}
};
Trigger.prototype.UnitsIntoShipFar = function()
{
	let pos = TriggerHelper.GetEntityPosition2D(point_sea_1);
	let ship = QuickSpawn(pos.x, pos.y, SHIP_TEMPLATE, 1);
	for (let i = 0; i < 50; ++i)
	{
		pos = TriggerHelper.GetEntityPosition2D(point_road_4);
		Do("garrison", { "target": ship }, QuickSpawn(pos.x+i, pos.y, UNIT_TEMPLATE));
	}
};

Trigger.prototype.UnitsIntoShipTwoIslands = function()
{
	let pos = TriggerHelper.GetEntityPosition2D(point_sea_2);
	let ship = QuickSpawn(pos.x, pos.y, SHIP_TEMPLATE, 1);
	for (let i = 0; i < 3; ++i)
	{
		pos = TriggerHelper.GetEntityPosition2D(point_road_2);
		Do("garrison", { "target": ship }, QuickSpawn(pos.x+i, pos.y, UNIT_TEMPLATE));
	}
	for (let i = 0; i < 3; ++i)
	{
		pos = TriggerHelper.GetEntityPosition2D(point_island_2);
		Do("garrison", { "target": ship }, QuickSpawn(pos.x+i, pos.y, UNIT_TEMPLATE));
	}
};


Trigger.prototype.UnitsIntoShipThenAnotherCloseBy = function()
{
	// This orders a unit to garrison in the ship, then orders another close by (by land)
	// but not actually close by (by sea) so that the ship behaves erratically.
	let pos = TriggerHelper.GetEntityPosition2D(point_sea_1);
	let ship = QuickSpawn(pos.x, pos.y, SHIP_TEMPLATE, 1);

	pos = TriggerHelper.GetEntityPosition2D(point_coast_1);
	Do("garrison", { "target": ship }, QuickSpawn(pos.x, pos.y, UNIT_TEMPLATE));

	// Don't do this at home
	let holder = Engine.QueryInterface(ship, IID_GarrisonHolder);
	let og = Object.getPrototypeOf(holder).PerformGarrison;
	holder.PerformGarrison = (...args) => {
		let res = og.apply(holder, args);
		delete holder.PerformGarrison;
		pos = TriggerHelper.GetEntityPosition2D(point_coast_2);
		Do("garrison", { "target": ship }, QuickSpawn(pos.x, pos.y, UNIT_TEMPLATE));
		return res;
	};
};


Trigger.prototype.UnitsIntoShipAlreadyInRange = function()
{
	// Yay ship on land.
	let pos = TriggerHelper.GetEntityPosition2D(point_road_3);
	let ship = QuickSpawn(pos.x, pos.y, SHIP_TEMPLATE, 1);
	for (let i = 0; i < 10; ++i)
	{
		pos = TriggerHelper.GetEntityPosition2D(point_road_3);
		Do("garrison", { "target": ship }, QuickSpawn(pos.x, pos.y, UNIT_TEMPLATE));
	}
};


Trigger.prototype.IslandUnitsPlayerGivesBadOrder = function()
{
	let pos = TriggerHelper.GetEntityPosition2D(point_sea_2);
	let ship = QuickSpawn(pos.x, pos.y, SHIP_TEMPLATE, 1);
	for (let i = 0; i < 3; ++i)
	{
		pos = TriggerHelper.GetEntityPosition2D(point_island_1);
		Do("garrison", { "target": ship }, QuickSpawn(pos.x+i, pos.y, UNIT_TEMPLATE));
	}
	// Order the ship to go somewhere it can't pick up the units from.
	WalkTo(567, 53, ship);
};

// Call everything here to easily disable/enable only some

cmpTrigger.DoAfterDelay(200, "UnitsIntoRamClose", {});
cmpTrigger.DoAfterDelay(200, "UnitsIntoRamFar", {});
cmpTrigger.DoAfterDelay(200, "UnitsIntoRamOppositeDir", {});
cmpTrigger.DoAfterDelay(200, "UnitsIntoRamImpossible", {});
cmpTrigger.DoAfterDelay(200, "UnitsIntoShipClose", {});
cmpTrigger.DoAfterDelay(200, "UnitsIntoShipFar", {});
cmpTrigger.DoAfterDelay(200, "UnitsIntoShipTwoIslands", {});
cmpTrigger.DoAfterDelay(200, "UnitsIntoShipThenAnotherCloseBy", {});
cmpTrigger.DoAfterDelay(200, "UnitsIntoShipAlreadyInRange", {});
cmpTrigger.DoAfterDelay(200, "IslandUnitsPlayerGivesBadOrder", {});
