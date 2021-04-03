const ARCHER_TEMPLATE = "units/maur/infantry_archer_b";
const JAV_TEMPLATE = "units/mace/infantry_javelineer_b";

const REG_UNIT_TEMPLATE = "units/athen/infantry_spearman_b";
const FAST_UNIT_TEMPLATE = "units/athen/cavalry_swordsman_b";
const FAST_UNIT_TEMPLATE_2 = "units/athen/cavalry_javelineer_b";

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

var WalkTo = function(x, z, queued, ent, owner=1)
{
	ProcessCommand(owner, {
		"type": "walk",
		"entities": Array.isArray(ent) ? ent : [ent],
		"x": x,
		"z": z,
		"queued": queued,
		"force": false,
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

var Garrison = function(target, ent)
{
	let comm = {
		"type": "garrison",
		"entities": Array.isArray(ent) ? ent : [ent],
		"target": target,
		"queued": true,
		"force": true,
	};
	ProcessCommand(1, comm);
	return ent;
};

var gx;
var gy;

var straight_line = function(attacker_first, attacker, target, walk = true)
{
	return () => {
		let chaser;
		let chasee;
		if (attacker_first)
		{
			chaser = QuickSpawn(gx, 80, attacker, ATTACKER);
			chasee = QuickSpawn(gx, 80+40, target);
		}
		else
		{
			chasee = QuickSpawn(gx, 80+40, target);
			chaser = QuickSpawn(gx, 80, attacker, ATTACKER);
		}
		if (walk)
			WalkTo(gx, 900, true, chasee);
		Attack(chasee, chaser, ATTACKER);
		return [chaser, chasee];
	};
};

var straight_line_garrison = function(garrison_first, attacker, target)
{
	return () => {
		let chaser;
		let chasee;
		if (garrison_first)
		{
			chaser = QuickSpawn(gx, gy, attacker);
			chasee = QuickSpawn(gx, gy+50, target);
		}
		else
		{
			chasee = QuickSpawn(gx, gy+50, target);
			chaser = QuickSpawn(gx, gy, attacker);
		}
		WalkTo(gx, 900, true, chasee);
		Garrison(chasee, chaser);
		return [chaser, chasee];
	};
};

var experiments = {};

experiments.fast_on_fast = {
	"spawn": straight_line(false, FAST_UNIT_TEMPLATE, FAST_UNIT_TEMPLATE_2)
};

experiments.fast_on_fast_2 = {
	"spawn": straight_line(true, FAST_UNIT_TEMPLATE, FAST_UNIT_TEMPLATE_2)
};

experiments.fast_on_slow = {
	"spawn": straight_line(false, FAST_UNIT_TEMPLATE, ARCHER_TEMPLATE)
};

experiments.fast_on_slow_2 = {
	"spawn": straight_line(true, FAST_UNIT_TEMPLATE, ARCHER_TEMPLATE)
};

experiments.slow_on_slow = {
	"spawn": straight_line(false, "units/athen/infantry_spearman_b", "units/mace/infantry_pikeman_b")
};

experiments.slow_on_slow_2 = {
	"spawn": straight_line(true, "units/athen/infantry_spearman_b", "units/mace/infantry_pikeman_b")
};

experiments.fast_on_trader = {
	"spawn": straight_line(false, FAST_UNIT_TEMPLATE, "units/mace/support_trader")
};

experiments.fast_on_trader_2 = {
	"spawn": straight_line(true, FAST_UNIT_TEMPLATE, "units/mace/support_trader")
};

// Traders are passive, let them flee
experiments.fast_on_trader_flee = {
	"spawn": straight_line(false, JAV_TEMPLATE, "units/mace/support_trader", false)
};

experiments.fast_on_trader_flee_2 = {
	"spawn": straight_line(true, JAV_TEMPLATE, "units/mace/support_trader", false)
};

experiments.slow_on_trader = {
	"spawn": straight_line(false, "units/athen/infantry_spearman_b", "units/mace/support_trader", false)
};

experiments.slow_on_trader_2 = {
	"spawn": straight_line(true, "units/athen/infantry_spearman_b", "units/mace/support_trader", false)
};

experiments.fast_on_muskox = {
	"spawn": straight_line(false, FAST_UNIT_TEMPLATE, "gaia/fauna_muskox")
};

experiments.fast_on_muskox_2 = {
	"spawn": straight_line(true, FAST_UNIT_TEMPLATE, "gaia/fauna_muskox")
};

// Women flee
experiments.fast_on_women_flee = {
	"spawn": straight_line(false, FAST_UNIT_TEMPLATE, "units/mace/support_female_citizen", false)
};

experiments.fast_on_women_flee_2 = {
	"spawn": straight_line(true, FAST_UNIT_TEMPLATE, "units/mace/support_female_citizen", false)
};

// Women flee
experiments.slow_on_women_flee = {
	"spawn": straight_line(false, "units/athen/infantry_spearman_b", "units/mace/support_female_citizen", false)
};

experiments.slow_on_women_flee_2 = {
	"spawn": straight_line(true, "units/athen/infantry_spearman_b", "units/mace/support_female_citizen", false)
};

experiments.straight_line_garrison = {
	"spawn": straight_line_garrison(false, "units/athen/infantry_spearman_b", "units/mace/siege_ram")
};

experiments.straight_line_garrison_2 = {
	"spawn": straight_line_garrison(true, "units/athen/infantry_spearman_b", "units/mace/siege_ram")
};

experiments.archer_on_spearman = {
	"spawn": straight_line(false, ARCHER_TEMPLATE, REG_UNIT_TEMPLATE, true)
};

experiments.archer_on_spearman_2 = {
	"spawn": straight_line(true, ARCHER_TEMPLATE, REG_UNIT_TEMPLATE, true)
};

experiments.jav_on_spearman = {
	"spawn": straight_line(false, JAV_TEMPLATE, REG_UNIT_TEMPLATE, true)
};

experiments.jav_on_spearman_2 = {
	"spawn": straight_line(true, JAV_TEMPLATE, REG_UNIT_TEMPLATE, true)
};

experiments.fast_archer_on_spearman = {
	"spawn": straight_line(false, FAST_UNIT_TEMPLATE_2, REG_UNIT_TEMPLATE, true)
};

experiments.fast_archer_on_spearman_2 = {
	"spawn": straight_line(true, FAST_UNIT_TEMPLATE_2, REG_UNIT_TEMPLATE, true)
};

experiments.fast_on_semi = {
	"spawn": straight_line(false, FAST_UNIT_TEMPLATE_2, ARCHER_TEMPLATE, true)
};

experiments.fast_on_semi_2 = {
	"spawn": straight_line(true, FAST_UNIT_TEMPLATE_2, ARCHER_TEMPLATE, true)
};

experiments.fast_on_flee = {
	"spawn": straight_line(false, FAST_UNIT_TEMPLATE_2, "units/mace/support_female_citizen", false)
};

experiments.fast_on_flee_2 = {
	"spawn": straight_line(true, FAST_UNIT_TEMPLATE_2, "units/mace/support_female_citizen", false)
};


var cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);

var onDelete = {};

Trigger.prototype.SetupUnits = function()
{
	gx = 130;
	gy = 150;
	for (let key in experiments)
	{
		let ents = experiments[key].spawn();
		onDelete[ents[0]] = ents;
		onDelete[ents[1]] = ents;
		gx += 15;
		if (gx >= 620)
		{
			gx = 120;
			gy += 70;
		}
	}
};

Trigger.prototype.OnOwnershipChanged = function(msg)
{
	if (msg.to === -1 && msg.entity in onDelete)
	{
		Engine.DestroyEntity(onDelete[msg.entity][0]);
		Engine.DestroyEntity(onDelete[msg.entity][1]);
	}
};

cmpTrigger.RegisterTrigger("OnOwnershipChanged", "OnOwnershipChanged", { "enabled": true });

var cmpModifiersManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ModifiersManager);

// Prevent promotions, messes up things.
cmpModifiersManager.AddModifiers("no_promotion_A", {
	"Promotion/RequiredXp": [{ "affects": ["Unit"], "replace": 50000 }],
}, 3);
cmpModifiersManager.AddModifiers("no_promotion_B", {
	"Promotion/RequiredXp": [{ "affects": ["Unit"], "replace": 50000 }],
}, 4); // player 2 is ent 4

cmpTrigger.DoAfterDelay(3000, "SetupUnits", {});
