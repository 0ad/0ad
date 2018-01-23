// Ships respawn every few minutes, attack the closest warships, then patrol the sea.
// To prevent unlimited spawning of ships, no more than the amount of ships intended at a given time are spawned.

// Ships are filled or refilled with new units.
// The number of ships, number of units per ship, as well as ratio of siege engines, champion and heroes
// increases with time, while keeping an individual and randomized composition for each ship.
// Each hero exists at most once per map.

// Every few minutes, equal amount of ships unload units at the sides of the river unless
// one side of the river was wiped from players.
// Siege engines attack defensive structures, units attack units then patrol that side of the river.

const showDebugLog = false;

var shipTemplate = "gaul_ship_trireme";
var siegeTemplate = "gaul_mechanical_siege_ram";

var heroTemplates = [
	"gaul_hero_viridomarus",
	"gaul_hero_vercingetorix",
	"gaul_hero_brennus"
];

var femaleTemplate = "gaul_support_female_citizen";
var healerTemplate = "gaul_support_healer_b";

var citizenInfantryTemplates = [
	"gaul_infantry_javelinist_b",
	"gaul_infantry_spearman_b",
	"gaul_infantry_slinger_b"
];

var citizenCavalryTemplates = [
	"gaul_cavalry_javelinist_b",
	"gaul_cavalry_swordsman_b"
];

var citizenTemplates = [...citizenInfantryTemplates, ...citizenCavalryTemplates];

var championInfantryTemplates = [
	"gaul_champion_fanatic",
	"gaul_champion_infantry"
];

var championCavalryTemplates = [
	"gaul_champion_cavalry"
];

var championTemplates = [...championInfantryTemplates, ...championCavalryTemplates];

var ccDefenders = [
	{ "count": 8, "template": "units/" + pickRandom(citizenInfantryTemplates) },
	{ "count": 8, "template": "units/" + pickRandom(championInfantryTemplates) },
	{ "count": 4, "template": "units/" + pickRandom(championCavalryTemplates) },
	{ "count": 4, "template": "units/" + healerTemplate },
	{ "count": 5, "template": "units/" + femaleTemplate },
	{ "count": 10, "template": "gaia/fauna_sheep" }
];

var gallicBuildingGarrison = [
	{
		"buildings": ["House"],
		"units": [femaleTemplate, healerTemplate]
	},
	{
		"buildings": ["CivCentre", "Temple"],
		"units": championTemplates
	},
	{
		"buildings": ["DefenseTower", "Outpost"],
		"units": championInfantryTemplates
	}
];

/**
 * Notice if gaia becomes too strong, players will just turtle and try to outlast the players on the other side.
 * However we want interaction and fights between the teams.
 * This can be accomplished by not wiping out players buildings entirely.
 */

/**
 * Time in minutes between two consecutive waves spawned from the gaia civic centers, if they still exist.
 */
var ccAttackerInterval = t => randFloat(6, 8);

/**
 * Number of attackers spawned at a civic center at t minutes ingame time.
 */
var ccAttackerCount = t => Math.min(20, Math.max(0, Math.round(t * 1.5)));

/**
 * Time between two consecutive waves.
 */
var shipRespawnTime = () => randFloat(8, 10);

/**
 * Limit of ships on the map when spawning them.
 * Have at least two ships, so that both sides will be visited.
 */
var shipCount = (t, numPlayers) => Math.max(2, Math.round(Math.min(1.5, t / 10) * numPlayers));

/**
 * Order all ships to ungarrison at the shoreline.
 */
var shipUngarrisonInterval = () => randFloat(5, 7);

/**
 * Time between refillings of all ships with new soldiers.
 */
var shipFillInterval = () => randFloat(4, 5);

/**
 * Total count of gaia attackers per shipload.
 */
var attackersPerShip = t => Math.min(30, Math.round(t * 2));

/**
 * Likelihood of adding a non-existing hero at t minutes.
 */
var heroProbability = t => Math.max(0, Math.min(1, (t - 25) / 60));

/**
 * Percent of healers to add per shipload after potentially adding a hero and siege engines.
 */
var healerRatio = t => randFloat(0, 0.1);

/**
 * Percent of siege engines to add per shipload.
 */
var siegeRatio = t => t < 8 ? 0 : randFloat(0.03, 0.06);

/**
 * Percent of champions to be added after spawning heroes, healers and siege engines.
 * Rest will be citizen soldiers.
 */
var championRatio = t => Math.min(1, Math.max(0, (t - 25) / 75));

/**
 * Ships and land units will queue attack orders for this amount of closest units.
 */
var targetCount = 3;

/**
 * Number of trigger points to patrol when not having enemies to attack.
 */
var patrolCount = 5;

/**
 * Which units ships should focus when attacking and patrolling.
 */
var shipTargetClass = "Warship";

/**
 * Which entities siege engines should focus when attacking and patrolling.
 */
var siegeTargetClass = "Defensive";

/**
 * Which entities units should focus when attacking and patrolling.
 */
var unitTargetClass = "Unit+!Ship";

/**
 * Ungarrison ships when being in this range of the target.
 */
var shipUngarrisonDistance = 50;

/**
 * Currently formations are not working properly and enemies in vision range are often ignored.
 * So only have a small chance of using formations.
 */
var formationProbability = 0.2;

var unitFormations = [
	"box",
	"battle_line",
	"line_closed",
	"column_closed"
];

/**
 * Chance for the units at the meeting place to participate in the ritual.
 */
var ritualProbability = 0.75;

/**
 * Units celebrating at the meeting place will perform one of these animations
 * if idle and switch back when becoming idle again.
 */
var ritualAnimations = {
	"female": ["attack_slaughter"],
	"male": ["attack_capture", "promotion", "attack_slaughter"],
	"healer": ["attack_capture", "promotion", "heal"]
};

var triggerPointShipSpawn = "A";
var triggerPointShipPatrol = "B";
var triggerPointUngarrisonLeft = "C";
var triggerPointUngarrisonRight = "D";
var triggerPointLandPatrolLeft = "E";
var triggerPointLandPatrolRight = "F";
var triggerPointCCAttackerPatrolLeft = "G";
var triggerPointCCAttackerPatrolRight = "H";
var triggerPointRiverDirection = "I";

/**
 * Which playerID to use for the opposing gallic reinforcements.
 */
var gaulPlayer = 0;

Trigger.prototype.debugLog = function(txt)
{
	if (showDebugLog)
		print(
			"DEBUG [" +
			Math.round(Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer).GetTime() / 60 / 1000) + "] " + txt + "\n");
};

/**
 * Return a random amount of these templates whose sum is count.
 */
Trigger.prototype.RandomAttackerTemplates = function(templates, count)
{
	let ratios = new Array(templates.length).fill(1).map(i => randFloat(0, 1));
	let ratioSum = ratios.reduce((current, sum) => current + sum, 0);

	let remainder = count;
	let templateCounts = {};

	for (let i in templates)
	{
		let currentCount = +i == templates.length - 1 ? remainder : Math.round(ratios[i] / ratioSum * count);
		if (!currentCount)
			continue;

		templateCounts[templates[i]] = currentCount;
		remainder -= currentCount;
	}

	if (remainder != 0)
		warn("Not as many templates as expected: " + count + " vs " + uneval(templateCounts));

	return templateCounts;
};

Trigger.prototype.GarrisonAllGallicBuildings = function(gaiaEnts)
{
	this.debugLog("Garrisoning all gallic buildings");

	for (let buildingGarrison of gallicBuildingGarrison)
		for (let building of buildingGarrison.buildings)
			this.SpawnAndGarrisonBuilding(gaiaEnts, building, buildingGarrison.units);
};

/**
 * Garrisons all targetEnts that match the targetClass with newly spawned entities of the given template.
 */
Trigger.prototype.SpawnAndGarrisonBuilding = function(gaiaEnts, targetClass, templates)
{
	for (let gaiaEnt of gaiaEnts)
	{
		let cmpIdentity = Engine.QueryInterface(gaiaEnt, IID_Identity);
		if (!cmpIdentity || !cmpIdentity.HasClass(targetClass))
			continue;

		let cmpGarrisonHolder = Engine.QueryInterface(gaiaEnt, IID_GarrisonHolder);
		if (!cmpGarrisonHolder)
			continue;

		let unitCounts = this.RandomAttackerTemplates(templates, cmpGarrisonHolder.GetCapacity());
		this.debugLog("Garrisoning " + uneval(unitCounts) + " at " + targetClass);

		for (let template in unitCounts)
			for (let newEnt of TriggerHelper.SpawnUnits(gaiaEnt, "units/" + template, unitCounts[template], gaulPlayer))
				Engine.QueryInterface(gaiaEnt, IID_GarrisonHolder).Garrison(newEnt);
	}
};

/**
 * Spawn units of the template at each gaia Civic Center and set them to defensive.
 */
Trigger.prototype.SpawnInitialCCDefenders = function(gaiaEnts)
{
	this.debugLog("To defend CCs, spawning " + uneval(ccDefenders));

	for (let gaiaEnt of gaiaEnts)
	{
		let cmpIdentity = Engine.QueryInterface(gaiaEnt, IID_Identity);
		if (!cmpIdentity || !cmpIdentity.HasClass("CivCentre"))
			continue;

		this.civicCenters.push(gaiaEnt);

		for (let ccDefender of ccDefenders)
			for (let ent of TriggerHelper.SpawnUnits(gaiaEnt, ccDefender.template, ccDefender.count, gaulPlayer))
				Engine.QueryInterface(ent, IID_UnitAI).SwitchToStance("defensive");
	}
};

Trigger.prototype.SpawnCCAttackers = function()
{
	let time = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer).GetTime() / 60 / 1000;

	let [spawnLeft, spawnRight] = this.GetActiveRiversides();

	for (let gaiaCC of this.civicCenters)
	{
		let isLeft = this.IsLeftRiverside(gaiaCC)
		if (isLeft && !spawnLeft || !isLeft && !spawnRight)
			continue;

		let toSpawn = this.GetAttackerComposition(ccAttackerCount(time), false);
		this.debugLog("Spawning civic center attackers at " + gaiaCC + ": " + uneval(toSpawn));

		let ccAttackers = [];

		for (let spawn of toSpawn)
		{
			let ents = TriggerHelper.SpawnUnits(gaiaCC, "units/" + spawn.template, spawn.count, gaulPlayer);

			if (spawn.hero && ents[0])
				this.heroes.push({ "template": spawn.template, "ent": ents[0] });

			ccAttackers = ccAttackers.concat(ents);
		}

		let patrolPointRef = isLeft ?
			triggerPointCCAttackerPatrolLeft :
			triggerPointCCAttackerPatrolRight;

		this.AttackAndPatrol(ccAttackers, unitTargetClass, patrolPointRef, "CCAttackers", false);
	}

	if (this.civicCenters.length)
		this.DoAfterDelay(ccAttackerInterval() * 60 * 1000, "SpawnCCAttackers", {});
};

/**
 * Remember most Humans present at the beginning of the match (before spawning any unit) and
 * make them defensive.
 */
Trigger.prototype.StartCelticRitual = function(gaiaEnts)
{
	for (let ent of gaiaEnts)
	{
		let cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
		if (!cmpIdentity || !cmpIdentity.HasClass("Human"))
			continue;

		if (randBool(ritualProbability))
			this.ritualEnts.push(ent);

		let cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		if (cmpUnitAI)
			cmpUnitAI.SwitchToStance("defensive");
	}

	this.DoRepeatedly(5 * 1000, "UpdateCelticRitual", {});
};

/**
 * Play one of the given animations for most participants if and only if they are idle.
 */
Trigger.prototype.UpdateCelticRitual = function()
{
	for (let ent of this.ritualEnts)
	{
		let cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		if (!cmpUnitAI || cmpUnitAI.GetCurrentState() != "INDIVIDUAL.IDLE")
			continue;

		let cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
		if (!cmpIdentity)
			continue;

		let animations = ritualAnimations[
			cmpIdentity.HasClass("Healer") ? "healer" :
			cmpIdentity.HasClass("Female") ? "female" : "male"];

		let cmpVisual = Engine.QueryInterface(ent, IID_Visual);
		if (!cmpVisual)
			continue;

		if (animations.indexOf(cmpVisual.GetAnimationName()) == -1)
			cmpVisual.SelectAnimation(pickRandom(animations), false, 1, "");
	}
};

/**
 * Spawn ships with a unique attacker composition each until
 * the number of ships is reached that is supposed to exist at the given time.
 */
Trigger.prototype.SpawnShips = function()
{
	let time = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer).GetTime() / 60 / 1000;
	let numPlayers = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetNumPlayers();

	let shipSpawnCount = shipCount(time, numPlayers) - this.ships.length;
	this.debugLog("Spawning " + shipSpawnCount + " ships");

	while (this.ships.length < shipSpawnCount)
		this.ships.push(TriggerHelper.SpawnUnits(pickRandom(this.GetTriggerPoints(triggerPointShipSpawn)), "units/" + shipTemplate, 1, gaulPlayer)[0]);

	for (let ship of this.ships)
		this.AttackAndPatrol([ship], shipTargetClass, triggerPointShipPatrol, "Ships");

	this.DoAfterDelay(shipRespawnTime(time) * 60 * 1000, "SpawnShips", {});

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	cmpTimer.CancelTimer(this.fillShipsTimer);

	this.FillShips();
};

Trigger.prototype.GetAttackerComposition = function(attackerCount, addSiege)
{
	let time = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer).GetTime() / 60 / 1000;
	let toSpawn = [];
	let remainder = attackerCount;

	let siegeCount = addSiege ? Math.round(siegeRatio(time) * remainder) : 0;
	if (siegeCount)
		toSpawn.push({ "template": siegeTemplate, "count": siegeCount });
	remainder -= siegeCount;

	let heroTemplate = pickRandom(heroTemplates.filter(hTemp => this.heroes.every(hero => hTemp != hero.template)));
	if (heroTemplate && remainder && randBool(heroProbability(time)))
	{
		toSpawn.push({ "template": heroTemplate, "count": 1, "hero": true });
		--remainder;
	}

	let healerCount = Math.round(healerRatio(time) * remainder);
	if (healerCount)
		toSpawn.push({ "template": healerTemplate, "count": healerCount });
	remainder -= healerCount;

	let championCount = Math.round(championRatio(time) * remainder);
	let championTemplateCounts = this.RandomAttackerTemplates(championTemplates, championCount);
	for (let template in championTemplateCounts)
	{
		let count = championTemplateCounts[template];
		toSpawn.push({ "template": template, "count": count });
		championCount -= count;
		remainder -= count;
	}

	let citizenTemplateCounts = this.RandomAttackerTemplates(citizenTemplates, remainder);
	for (let template in citizenTemplateCounts)
	{
		let count = citizenTemplateCounts[template];
		toSpawn.push({ "template": template, "count": count });
		remainder -= count;
	}

	if (remainder != 0)
		warn("Didn't spawn as many attackers as were intended (" + remainder + " remaining)");

	return toSpawn;
};

Trigger.prototype.FillShips = function()
{
	let time = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer).GetTime() / 60 / 1000;
	for (let ship of this.ships)
	{
		let cmpGarrisonHolder = Engine.QueryInterface(ship, IID_GarrisonHolder);
		if (!cmpGarrisonHolder)
			continue;

		let toSpawn = this.GetAttackerComposition(Math.max(0, attackersPerShip(time) - cmpGarrisonHolder.GetEntities().length), true);
		this.debugLog("Filling ship " + ship + " with " + uneval(toSpawn));

		for (let spawn of toSpawn)
		{
			// Don't use TriggerHelper.SpawnUnits here because that is too slow,
			// needlessly trying all spawn points near the ships footprint which all fail

			for (let i = 0; i < spawn.count; ++i)
			{
				let ent = Engine.AddEntity("units/" + spawn.template);
				Engine.QueryInterface(ent, IID_Ownership).SetOwner(gaulPlayer);

				if (spawn.hero)
					this.heroes.push({ "template": spawn.template, "ent": ent });

				cmpGarrisonHolder.Garrison(ent);
			}
		}
	}

	this.fillShipsTimer = this.DoAfterDelay(shipFillInterval() * 60 * 1000, "FillShips", {});
};

/**
 * Attack the closest enemy ships around, then patrol the sea.
 */
Trigger.prototype.AttackAndPatrol = function(attackers, targetClass, triggerPointRef, debugName, attack = true)
{
	if (!attackers.length)
		return;

	let allTargets = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager).GetNonGaiaEntities().filter(ent => {
		let cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
		return cmpIdentity && MatchesClassList(cmpIdentity.GetClassesList(), targetClass);
	});

	let targets = allTargets.sort((ent1, ent2) =>
		DistanceBetweenEntities(attackers[0], ent1) - DistanceBetweenEntities(attackers[0], ent2)).slice(0, targetCount);

	this.debugLog(debugName + " " + uneval(attackers) + " attack " + uneval(targets));

	if (attack)
		for (let target of targets)
			ProcessCommand(gaulPlayer, {
				"type": "attack",
				"entities": attackers,
				"target": target,
				"queued": true,
				"allowCapture": false
			});

	let patrolTargets = shuffleArray(this.GetTriggerPoints(triggerPointRef)).slice(0, patrolCount);
	this.debugLog(debugName + " " + uneval(attackers) + " patrol to " + uneval(patrolTargets));

	for (let patrolTarget of patrolTargets)
	{
		let pos = Engine.QueryInterface(patrolTarget, IID_Position).GetPosition2D();
		ProcessCommand(gaulPlayer, {
			"type": "patrol",
			"entities": attackers,
			"x": pos.x,
			"z": pos.y,
			"targetClasses": {
				"attack": [targetClass]
			},
			"queued": true,
			"allowCapture": false
		});
	}
};

/**
 * To avoid unloading unlimited amounts of units on empty riversides,
 * only add attackers to riversides where player buildings exist that are
 * actually targeted.
 */
Trigger.prototype.GetActiveRiversides = function()
{
	let left = false;
	let right = false;

	for (let ent of Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager).GetNonGaiaEntities())
	{
		let cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
		if (!cmpIdentity || !cmpIdentity.HasClass(siegeTargetClass))
			continue;

		if (this.IsLeftRiverside(ent))
			left = true;
		else
			right = true;

		if (left && right)
			break;
	}

	return [left, right];
};

Trigger.prototype.IsLeftRiverside = function(ent)
{
	return this.riverDirection.cross(Vector2D.sub(Engine.QueryInterface(ent, IID_Position).GetPosition2D(), this.mapCenter)) > 0;
};

/**
 * Order all ships to abort naval warfare and move to the shoreline all few minutes.
 */
Trigger.prototype.UngarrisonShipsOrder = function()
{
	let [ungarrisonLeft, ungarrisonRight] = this.GetActiveRiversides();
	if (!ungarrisonLeft && !ungarrisonRight)
		return;

	// Determine which ships should ungarrison on which side of the river
	let shipsLeft = [];
	let shipsRight = [];

	if (ungarrisonLeft && ungarrisonRight)
	{
		shipsLeft = shuffleArray(this.ships).slice(0, Math.round(this.ships.length / 2));
		shipsRight = this.ships.filter(ship => shipsLeft.indexOf(ship) == -1);
	}
	else if (ungarrisonLeft)
		shipsLeft = this.ships;
	else if (ungarrisonRight)
		shipsRight = this.ships;

	// Determine which ships should ungarrison and patrol at which trigger point names
	let sides = [];
	if (shipsLeft.length)
		sides.push({
			"ships": shipsLeft,
			"ungarrisonPointRef": triggerPointUngarrisonLeft,
			"landPointRef": triggerPointLandPatrolLeft
		});

	if (shipsRight.length)
		sides.push({
			"ships": shipsRight,
			"ungarrisonPointRef": triggerPointUngarrisonRight,
			"landPointRef": triggerPointLandPatrolRight
		});

	// Order those ships to move to a randomly chosen trigger point on the determined
	// side of the river. Remember that chosen ungarrison point and the name of the
	// trigger points where the ungarrisoned units should patrol afterwards.
	for (let side of sides)
		for (let ship of side.ships)
		{
			let ungarrisonPoint = pickRandom(this.GetTriggerPoints(side.ungarrisonPointRef));
			let ungarrisonPos = Engine.QueryInterface(ungarrisonPoint, IID_Position).GetPosition2D();

			this.debugLog("Ship " + ship + " will ungarrison at " + side.ungarrisonPointRef +
				" (" + ungarrisonPos.x + "," + ungarrisonPos.y + ")");

			Engine.QueryInterface(ship, IID_UnitAI).Walk(ungarrisonPos.x, ungarrisonPos.y, false);
			this.shipTarget[ship] = { "landPointRef": side.landPointRef, "ungarrisonPoint": ungarrisonPoint };
		}

	this.DoAfterDelay(shipUngarrisonInterval() * 60 * 1000, "UngarrisonShipsOrder", {});
};

/**
 * Check frequently whether the ships are close enough to unload at the shoreline.
 */
Trigger.prototype.CheckShipRange = function()
{
	for (let ship of this.ships)
	{
		if (!this.shipTarget[ship] || DistanceBetweenEntities(ship, this.shipTarget[ship].ungarrisonPoint) > shipUngarrisonDistance)
			continue;

		let cmpGarrisonHolder = Engine.QueryInterface(ship, IID_GarrisonHolder);
		if (!cmpGarrisonHolder)
			continue;

		let attackers = cmpGarrisonHolder.GetEntities();
		let siegeEngines = attackers.filter(ent => Engine.QueryInterface(ent, IID_Identity).HasClass("Siege"));
		let others = attackers.filter(ent => siegeEngines.indexOf(ent) == -1);

		this.debugLog("Ungarrisoning ship " + ship + " at " + uneval(this.shipTarget[ship]));
		cmpGarrisonHolder.UnloadAll();

		if (randBool(formationProbability))
			ProcessCommand(gaulPlayer, {
				"type": "formation",
				"entities": others,
				"name": "special/formations/" + pickRandom(unitFormations)
			});

		this.AttackAndPatrol(siegeEngines, siegeTargetClass, this.shipTarget[ship].landPointRef, "Siege");
		this.AttackAndPatrol(others, unitTargetClass, this.shipTarget[ship].landPointRef, "Units");
		delete this.shipTarget[ship];

		this.AttackAndPatrol([ship], shipTargetClass, triggerPointShipPatrol, "Ships");
	}
};

Trigger.prototype.DanubiusOwnershipChange = function(data)
{
	if (data.from != 0)
		return;

	let shipIdx = this.ships.indexOf(data.entity);
	if (shipIdx != -1)
	{
		this.debugLog("Ship " + data.entity + " sunk");
		this.ships.splice(shipIdx, 1);
	}

	let ritualIdx = this.ritualEnts.indexOf(data.entity);
	if (ritualIdx != -1)
		this.ritualEnts.splice(ritualIdx, 1);

	let heroIdx = this.heroes.findIndex(hero => hero.ent == data.entity);
	if (ritualIdx != -1)
		this.heroes.splice(heroIdx, 1);

	let ccIdx = this.civicCenters.indexOf(data.entity);
	if (ccIdx != -1)
	{
		this.debugLog("Gaia civic center " + data.entity + " destroyed or captured");
		this.civicCenters.splice(ccIdx, 1);
	}
};


{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);

	let gaiaEnts = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager).GetEntitiesByPlayer(0);

	cmpTrigger.ritualEnts = [];

	// To prevent spawning more than the limits, track IDs of current entities
	cmpTrigger.ships = [];
	cmpTrigger.heroes = [];

	// Remember gaia CCs to spawn attackers from
	cmpTrigger.civicCenters = [];

	// Maps from gaia ship entity ID to ungarrison trigger point entity ID and land patrol triggerpoint name
	cmpTrigger.shipTarget = {};
	cmpTrigger.fillShipsTimer = undefined;

	// Be able to distinguish between the left and right riverside
	let mapSize = Engine.QueryInterface(SYSTEM_ENTITY, IID_Terrain).GetMapSize();
	cmpTrigger.mapCenter = new Vector2D(mapSize / 2, mapSize / 2);
	cmpTrigger.riverDirection = Vector2D.sub(
		Engine.QueryInterface(cmpTrigger.GetTriggerPoints(triggerPointRiverDirection)[0], IID_Position).GetPosition2D(),
		cmpTrigger.mapCenter);

	cmpTrigger.StartCelticRitual(gaiaEnts);
	cmpTrigger.GarrisonAllGallicBuildings(gaiaEnts);
	cmpTrigger.SpawnInitialCCDefenders(gaiaEnts);
	cmpTrigger.SpawnCCAttackers(gaiaEnts);

	cmpTrigger.SpawnShips();
	cmpTrigger.DoAfterDelay(shipUngarrisonInterval() * 60 * 1000, "UngarrisonShipsOrder", {});
	cmpTrigger.DoRepeatedly(5 * 1000, "CheckShipRange", {});

	cmpTrigger.RegisterTrigger("OnOwnershipChanged", "DanubiusOwnershipChange", { "enabled": true });
}
