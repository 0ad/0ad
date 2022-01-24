const REG_UNIT_TEMPLATE = "units/athen/infantry_spearman_b";
const FAST_UNIT_TEMPLATE = "units/athen/cavalry_swordsman_b";
const LARGE_UNIT_TEMPLATE = "units/brit/siege_ram";
const ELE_TEMPLATE = "units/cart/champion_elephant";

const ATTACKER = 2;

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

var WalkTo = function(x, z, queued, ent, owner=1)
{
	ProcessCommand(owner, {
		"type": "walk",
		"entities": Array.isArray(ent) ? ent : [ent],
		"x": x,
		"z": z,
		"queued": queued,
		"force": true,
	});
	return ent;
};

var FormationWalkTo = function(x, z, queued, ent, owner=1)
{
	ProcessCommand(owner, {
		"type": "walk",
		"entities": Array.isArray(ent) ? ent : [ent],
		"x": x,
		"z": z,
		"queued": queued,
		"force": true,
		"formation": "special/formations/box"
	});
	return ent;
};

var Attack = function(target, ent)
{
	let comm = {
		"type": "attack",
		"entities": Array.isArray(ent) ? ent : [ent],
		"target": target,
		"queued": true,
		"force": true,
	};
	ProcessCommand(ATTACKER, comm);
	return ent;
};

var Do = function(name, data, ent, owner = 1)
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


var experiments = {};

experiments.units_sparse_forest_of_units = {
	"spawn": (gx, gy) => {
		for (let i = -16; i <= 16; i += 8)
			for (let j = -16; j <= 16; j += 8)
				QuickSpawn(gx + i, gy + 50 + j, REG_UNIT_TEMPLATE);
		WalkTo(gx, gy + 100, true, QuickSpawn(gx, gy, REG_UNIT_TEMPLATE));
		WalkTo(gx, gy + 100, true, QuickSpawn(gx, gy-10, LARGE_UNIT_TEMPLATE));
	}
};

experiments.units_dense_forest_of_units = {
	"spawn": (gx, gy) => {
		for (let i = -16; i <= 16; i += 4)
			for (let j = -16; j <= 16; j += 4)
				QuickSpawn(gx + i, gy + 50 + j, REG_UNIT_TEMPLATE);
		WalkTo(gx, gy + 100, true, QuickSpawn(gx, gy, REG_UNIT_TEMPLATE));
		WalkTo(gx, gy + 100, true, QuickSpawn(gx, gy-10, LARGE_UNIT_TEMPLATE));
	}
};

experiments.units_superdense_forest_of_units = {
	"spawn": (gx, gy) => {
		for (let i = -6; i <= 6; i += 2)
			for (let j = -6; j <= 6; j += 2)
				QuickSpawn(gx + i, gy + 50 + j, REG_UNIT_TEMPLATE);
		WalkTo(gx, gy + 100, true, QuickSpawn(gx, gy, REG_UNIT_TEMPLATE));
		WalkTo(gx, gy + 100, true, QuickSpawn(gx, gy-10, LARGE_UNIT_TEMPLATE));
	}
};

experiments.units_superdense_forest_of_fast_units = {
	"spawn": (gx, gy) => {
		for (let i = -12; i <= 12; i += 2)
			for (let j = -12; j <= 12; j += 2)
				QuickSpawn(gx + i, gy + 50 + j, FAST_UNIT_TEMPLATE);
		WalkTo(gx, gy + 100, true, QuickSpawn(gx, gy, FAST_UNIT_TEMPLATE));
		WalkTo(gx, gy + 100, true, QuickSpawn(gx, gy-10, LARGE_UNIT_TEMPLATE));
	}
};

experiments.building = {
	"spawn": (gx, gy) => {
		let target = QuickSpawn(gx + 20, gy + 20, "foundation|structures/athen/storehouse");
		for (let i = 0; i < 8; ++i)
			Do("repair", { "target": target }, QuickSpawn(gx + i, gy, REG_UNIT_TEMPLATE));

		let cmpFoundation = Engine.QueryInterface(target, IID_Foundation);
		cmpFoundation.InitialiseConstruction("structures/athen/storehouse");
	}
};

experiments.collecting_tree = {
	"spawn": (gx, gy) => {
		let target = QuickSpawn(gx + 10, gy + 10, "gaia/tree/acacia");
		let storehouse = QuickSpawn(gx - 10, gy - 10, "structures/athen/storehouse");
		for (let i = 0; i < 8; ++i)
			Do("gather", { "target": target }, QuickSpawn(gx + i, gy, REG_UNIT_TEMPLATE));

		let cmpModifiersManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ModifiersManager);
		// Make that tree essentially infinite.
		cmpModifiersManager.AddModifiers("inf_tree", {
			"ResourceSupply/Max": [{ "replace": 50000 }],
		}, target);
		let cmpSupply = Engine.QueryInterface(target, IID_ResourceSupply);
		cmpSupply.SetAmount(50000);
		// Make the storehouse a territory root
		cmpModifiersManager.AddModifiers("root", {
			"TerritoryInfluence/Root": [{ "affects": ["Structure"], "replace": true }],
		}, storehouse);
		// Make units gather instantly
		cmpModifiersManager.AddModifiers("gatherrate", {
			"ResourceGatherer/BaseSpeed": [{ "affects": ["Unit"], "replace": 100 }],
		}, 3); // Player 1 is ent 3
	}
};

experiments.multicrossing = {
	"spawn": (gx, gy) => {
		for (let i = 0; i < 20; i += 2)
			for (let j = 0; j < 20; j += 2)
				WalkTo(gx+10, gy+70, false, QuickSpawn(gx + i, gy + j, REG_UNIT_TEMPLATE));
		for (let i = 0; i < 20; i += 2)
			for (let j = 0; j < 20; j += 2)
				WalkTo(gx+10, gy, false, QuickSpawn(gx + i, gy + j + 70, REG_UNIT_TEMPLATE));
	}
};

// Same as above but not as aligned.
experiments.multicrossing_spaced = {
	"spawn": (gx, gy) => {
		for (let i = 0; i < 20; i += 2)
			for (let j = 0; j < 20; j += 2)
				WalkTo(gx+10, gy+70, false, QuickSpawn(gx + i, gy + j, REG_UNIT_TEMPLATE));
		for (let i = 0; i < 20; i += 2)
			for (let j = 0; j < 20; j += 2)
				WalkTo(gx+10 + 5, gy, false, QuickSpawn(gx + i + 5, gy + j + 70, REG_UNIT_TEMPLATE));
	}
};

// Same as above but not as aligned.
experiments.multicrossing_spaced_2 = {
	"spawn": (gx, gy) => {
		for (let i = 0; i < 20; i += 2)
			for (let j = 0; j < 20; j += 2)
				WalkTo(gx+10, gy+70, false, QuickSpawn(gx + i, gy + j, REG_UNIT_TEMPLATE));
		for (let i = 0; i < 20; i += 2)
			for (let j = 0; j < 20; j += 2)
				WalkTo(gx+10 - 5, gy, false, QuickSpawn(gx + i - 5, gy + j + 70, REG_UNIT_TEMPLATE));
	}
};

experiments.crossing_perpendicular = {
	"spawn": (gx, gy) => {
		for (let i = 0; i < 20; i += 4)
			for (let j = 0; j < 20; j += 4)
				WalkTo(gx+10, gy+70, false, QuickSpawn(gx + i, gy + j, REG_UNIT_TEMPLATE));
		for (let i = 0; i < 20; i += 4)
			for (let j = 0; j < 20; j += 4)
				WalkTo(gx - 35, gy + 35, false, QuickSpawn(gx + i + 35, gy + j + 35, REG_UNIT_TEMPLATE));
	}
};

experiments.elephant_formation = {
	"spawn": (gx, gy) => {
		let ents = [];
		for (let i = 0; i < 20; i += 4)
			for (let j = 0; j < 20; j += 4)
				ents.push(QuickSpawn(gx + i, gy + j, ELE_TEMPLATE));
		FormationWalkTo(gx, gy+10, false, ents);
	}
};


experiments.sep1 = {
	"spawn": (gx, gy) => {}
};

experiments.battle = {
	"spawn": (gx, gy) => {
		for (let i = 0; i < 4; ++i)
			for (let j = 0; j < 8; ++j)
			{
				QuickSpawn(gx + i, gy + j, REG_UNIT_TEMPLATE);
				QuickSpawn(gx + i, gy + 50 + j, REG_UNIT_TEMPLATE, ATTACKER);
			}
	}
};

experiments.sep2 = {
	"spawn": (gx, gy) => {}
};


experiments.overlapping = {
	"spawn": (gx, gy) => {
		for (let i = 0; i < 20; ++i)
			QuickSpawn(gx, gy, REG_UNIT_TEMPLATE);
		for (let i = 0; i < 20; ++i)
			QuickSpawn(gx+15, gy+15, REG_UNIT_TEMPLATE);
	}
};

var perf_experiments = {};

// Perf check: put units everywhere, not moving.
perf_experiments.Idle = {
	"spawn": () => {
		const spacing = 12;
		for (let x = 0; x < 20*4*4 - 20; x += spacing)
			for (let z = 0; z < 20*4*4 - 20; z += spacing)
				QuickSpawn(x, z, REG_UNIT_TEMPLATE);
	}
};

// Perf check: put units everywhere, moving.
perf_experiments.MovingAround = {
	"spawn": () => {
		const spacing = 24;
		for (let x = 0; x < 20*16*4 - 20; x += spacing)
			for (let z = 0; z < 20*16*4 - 20; z += spacing)
			{
				let ent = QuickSpawn(x, z, REG_UNIT_TEMPLATE);
				for (let i = 0; i < 5; ++i)
				{
					WalkTo(x + 4, z, true, ent);
					WalkTo(x + 4, z + 4, true, ent);
					WalkTo(x, z + 4, true, ent);
					WalkTo(x, z, true, ent);
				}
			}
	}
};
// Perf check: fewer units moving more.
perf_experiments.LighterMovingAround = {
	"spawn": () => {
		const spacing = 48;
		for (let x = 0; x < 20*16*4 - 20; x += spacing)
			for (let z = 0; z < 20*16*4 - 20; z += spacing)
			{
				let ent = QuickSpawn(x, z, REG_UNIT_TEMPLATE);
				for (let i = 0; i < 5; ++i)
				{
					WalkTo(x + 20, z, true, ent);
					WalkTo(x + 20, z + 20, true, ent);
					WalkTo(x, z + 20, true, ent);
					WalkTo(x, z, true, ent);
				}
			}
	}
};

// Perf check: rows of units crossing each other.
perf_experiments.BunchaCollisions = {
	"spawn": () => {
		const spacing = 64;
		for (let x = 0; x < 20*16*4 - 20; x += spacing)
			for (let z = 0; z < 20*16*4 - 20; z += spacing)
			{
				for (let i = 0; i < 10; ++i)
				{
					// Add a little variation to the spawning, or all clusters end up identical.
					let ent = QuickSpawn(x + i + randFloat(-0.5, 0.5), z + 20 * (i%2) + randFloat(-0.5, 0.5), REG_UNIT_TEMPLATE);
					for (let ii = 0; ii < 5; ++ii)
					{
						WalkTo(x + i, z + 20, true, ent);
						WalkTo(x + i, z, true, ent);
					}
				}
			}
	}
};

// Massive moshpit of pushing.
perf_experiments.LotsaLocalCollisions = {
	"spawn": () => {
		const spacing = 3;
		for (let x = 100; x < 200; x += spacing)
			for (let z = 100; z < 200; z += spacing)
			{
				let ent = QuickSpawn(x, z, REG_UNIT_TEMPLATE);
				for (let ii = 0; ii < 20; ++ii)
					WalkTo(randFloat(100, 200), randFloat(100, 200), true, ent);
			}
	}
};


var woodcutting = (gx, gy) => {
	let dropsite = QuickSpawn(gx + 50, gy, "structures/athen/storehouse");
	let cmpModifiersManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ModifiersManager);
	cmpModifiersManager.AddModifiers("root", {
		"TerritoryInfluence/Root": [{ "affects": ["Structure"], "replace": true }],
	}, dropsite);
	// Make units gather faster
	cmpModifiersManager.AddModifiers("gatherrate", {
		"ResourceGatherer/BaseSpeed": [{ "affects": ["Unit"], "multiply": 3 }],
	}, 3); // Player 1 is ent 3
	for (let i = 20; i <= 80; i += 10)
		for (let j = 20; j <= 50; j += 10)
			QuickSpawn(gx + i, gy + j, "gaia/tree/acacia");
	for (let i = 10; i <= 90; i += 5)
		Do("gather-near-position", { "x": gx + i, "z": gy + 10, "resourceType": { "generic": "wood", "specific": "tree" }, "resourceTemplate": "gaia/tree/acacia" },
			QuickSpawn(gx + i, gy, REG_UNIT_TEMPLATE));
};

perf_experiments.WoodCutting = {
	"spawn": () => {
		for (let i = 0; i < 8; i++)
			for (let j = 0; j < 8; j++)
			{
				woodcutting(20 + i*100, 20 + j*100);
			}
	}
};

var cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);

Trigger.prototype.Setup = function()
{
	let start = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer).GetTime();

	// /*
	let gx = 100;
	let gy = 100;
	for (let key in experiments)
	{
		experiments[key].spawn(gx, gy);
		gx += 90;
		if (gx > 20*16*4-20)
		{
			gx = 100;
			gy += 150;
		}
	}
	/**/
	//perf_experiments.LotsaLocalCollisions.spawn();
	/*
	let time = 0;
	for (let key in perf_experiments)
	{
		cmpTrigger.DoAfterDelay(1000 + time * 10000, "RunExperiment", { "exp": key });
		time++;
	}
	/**/
};

Trigger.prototype.Cleanup = function()
{
	warn("cleanup");
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	let ents = cmpRangeManager.GetEntitiesByPlayer(1).concat(cmpRangeManager.GetEntitiesByPlayer(2));
	for (let ent of ents)
		Engine.DestroyEntity(ent);
};

Trigger.prototype.RunExperiment = function(data)
{
	warn("Start of " + data.exp);
	perf_experiments[data.exp].spawn();
	cmpTrigger.DoAfterDelay(9500, "Cleanup", {});
};

Trigger.prototype.EndGame = function()
{
	Engine.QueryInterface(4, IID_Player).SetState("defeated", "trigger");
	Engine.QueryInterface(3, IID_Player).SetState("won", "trigger");
};

/*
var cmpModifiersManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ModifiersManager);

// Reduce player 1 vision range (or patrolling units reacct)
cmpModifiersManager.AddModifiers("no_promotion", {
	"Vision/Range": [{ "affects": ["Unit"], "replace": 5 }],
}, 3); // player 1 is ent 3

// Prevent promotions, messes up things.
cmpModifiersManager.AddModifiers("no_promotion_A", {
	"Promotion/RequiredXp": [{ "affects": ["Unit"], "replace": 50000 }],
}, 3);
cmpModifiersManager.AddModifiers("no_promotion_B", {
	"Promotion/RequiredXp": [{ "affects": ["Unit"], "replace": 50000 }],
}, 4); // player 2 is ent 4
*/

cmpTrigger.DoAfterDelay(4000, "Setup", {});

cmpTrigger.DoAfterDelay(300000, "EndGame", {});
