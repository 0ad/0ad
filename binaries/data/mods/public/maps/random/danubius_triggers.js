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

const danubiusAttackerTemplates = deepfreeze({
	"ships": TriggerHelper.GetTemplateNamesByClasses("Warship", "gaul", undefined, undefined, true),
	"siege": TriggerHelper.GetTemplateNamesByClasses("Siege","gaul", undefined, undefined, true),
	"females": TriggerHelper.GetTemplateNamesByClasses("FemaleCitizen","gaul", undefined, undefined, true),
	"healers": TriggerHelper.GetTemplateNamesByClasses("Healer","gaul", undefined, undefined, true),
	"champions": TriggerHelper.GetTemplateNamesByClasses("Champion", "gaul", undefined, undefined, true),
	"champion_infantry": TriggerHelper.GetTemplateNamesByClasses("Champion+Infantry", "gaul", undefined, undefined, true),
	"citizen_soldiers": TriggerHelper.GetTemplateNamesByClasses("CitizenSoldier", "gaul", undefined, "Basic", true),
	"heroes": [
		// Excludes the Vercingetorix variant
		"units/gaul_hero_viridomarus",
		"units/gaul_hero_vercingetorix",
		"units/gaul_hero_brennus"
	]
});

var ccDefenders = [
	{ "count": 8, "templates": danubiusAttackerTemplates.citizen_soldiers },
	{ "count": 13, "templates": danubiusAttackerTemplates.champions },
	{ "count": 4, "templates": danubiusAttackerTemplates.healers },
	{ "count": 5, "templates": danubiusAttackerTemplates.females },
	{ "count": 10, "templates": ["gaia/fauna_sheep"] }
];

var gallicBuildingGarrison = [
	{
		"buildingClasses": ["House"],
		"unitTemplates": danubiusAttackerTemplates.females.concat(danubiusAttackerTemplates.healers)
	},
	{
		"buildingClasses": ["CivCentre", "Temple"],
		"unitTemplates": danubiusAttackerTemplates.champions,
	},
	{
		"buildingClasses": ["DefenseTower", "Outpost"],
		"unitTemplates": danubiusAttackerTemplates.champion_infantry
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
 * Number of siege engines to add per shipload.
 */
var siegeCount = t => 1 + Math.min(2, Math.floor(t / 30));

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

Trigger.prototype.GarrisonAllGallicBuildings = function(gaiaEnts)
{
	this.debugLog("Garrisoning all gallic buildings");

	for (let buildingGarrison of gallicBuildingGarrison)
		for (let buildingClass of buildingGarrison.buildingClasses)
		{
			let unitCounts = TriggerHelper.SpawnAndGarrisonAtClasses(gaulPlayer, buildingClass, buildingGarrison.unitTemplates, 1);
			this.debugLog("Garrisoning at " + buildingClass + ": " + uneval(unitCounts));
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
			for (let ent of TriggerHelper.SpawnUnits(gaiaEnt, pickRandom(ccDefender.templates), ccDefender.count, gaulPlayer))
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

		let templateCounts = TriggerHelper.BalancedTemplateComposition(this.GetAttackerComposition(time, false), ccAttackerCount(time));
		this.debugLog("Spawning civic center attackers at " + gaiaCC + ": " + uneval(templateCounts));

		let ccAttackers = [];

		for (let templateName in templateCounts)
		{
			let ents = TriggerHelper.SpawnUnits(gaiaCC, templateName, templateCounts[templateName], gaulPlayer);

			if (danubiusAttackerTemplates.heroes.indexOf(templateName) != -1 && ents[0])
				this.heroes.push(ents[0]);

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
		this.ships.push(TriggerHelper.SpawnUnits(pickRandom(this.GetTriggerPoints(triggerPointShipSpawn)), pickRandom(danubiusAttackerTemplates.ships), 1, gaulPlayer)[0]);

	for (let ship of this.ships)
		this.AttackAndPatrol([ship], shipTargetClass, triggerPointShipPatrol, "Ships");

	this.DoAfterDelay(shipRespawnTime(time) * 60 * 1000, "SpawnShips", {});

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	cmpTimer.CancelTimer(this.fillShipsTimer);

	this.FillShips();
};

Trigger.prototype.GetAttackerComposition = function(time, siegeEngines)
{
	let champRatio = championRatio(time);
	return [
		{
			"templates": danubiusAttackerTemplates.heroes,
			"count": randBool(heroProbability(time)) ? 1 : 0,
			"unique_entities": this.heroes
		},
		{
			"templates": danubiusAttackerTemplates.siege,
			"count": siegeEngines ? siegeCount(time) : 0
		},
		{
			"templates": danubiusAttackerTemplates.healers,
			"frequency": healerRatio(time)
		},
		{
			"templates": danubiusAttackerTemplates.champions,
			"frequency": champRatio
		},
		{
			"templates": danubiusAttackerTemplates.citizen_soldiers,
			"frequency": 1 - champRatio
		}
	];
};

Trigger.prototype.FillShips = function()
{
	let time = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer).GetTime() / 60 / 1000;
	for (let ship of this.ships)
	{
		let cmpGarrisonHolder = Engine.QueryInterface(ship, IID_GarrisonHolder);
		if (!cmpGarrisonHolder)
			continue;

		let templateCounts = TriggerHelper.BalancedTemplateComposition(
			this.GetAttackerComposition(time, true),
			Math.max(0, attackersPerShip(time) - cmpGarrisonHolder.GetEntities().length));

		this.debugLog("Filling ship " + ship + " with " + uneval(templateCounts));

		for (let templateName in templateCounts)
		{
			let ents = TriggerHelper.SpawnGarrisonedUnits(ship, templateName, templateCounts[templateName], gaulPlayer);
			if (danubiusAttackerTemplates.heroes.indexOf(templateName) != -1 && ents[0])
				this.heroes.push(ents[0]);
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

	let heroIdx = this.heroes.findIndex(ent => ent == data.entity);
	if (ritualIdx != -1)
		this.heroes.splice(heroIdx, 1);

	let ccIdx = this.civicCenters.indexOf(data.entity);
	if (ccIdx != -1)
	{
		this.debugLog("Gaia civic center " + data.entity + " destroyed or captured");
		this.civicCenters.splice(ccIdx, 1);
	}
};

Trigger.prototype.InitDanubius = function()
{
	this.ritualEnts = [];

	// To prevent spawning more than the limits, track IDs of current entities
	this.ships = [];
	this.heroes = [];

	// Remember gaia CCs to spawn attackers from
	this.civicCenters = [];

	// Maps from gaia ship entity ID to ungarrison trigger point entity ID and land patrol triggerpoint name
	this.shipTarget = {};
	this.fillShipsTimer = undefined;

	// Be able to distinguish between the left and right riverside
	let mapSize = Engine.QueryInterface(SYSTEM_ENTITY, IID_Terrain).GetMapSize();
	this.mapCenter = new Vector2D(mapSize / 2, mapSize / 2);
	this.riverDirection = Vector2D.sub(
		Engine.QueryInterface(this.GetTriggerPoints(triggerPointRiverDirection)[0], IID_Position).GetPosition2D(),
		this.mapCenter);

	let gaiaEnts = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager).GetEntitiesByPlayer(0);
	this.StartCelticRitual(gaiaEnts);
	this.GarrisonAllGallicBuildings(gaiaEnts);
	this.SpawnInitialCCDefenders(gaiaEnts);
	this.SpawnCCAttackers();

	this.SpawnShips();
	this.DoAfterDelay(shipUngarrisonInterval() * 60 * 1000, "UngarrisonShipsOrder", {});
	this.DoRepeatedly(5 * 1000, "CheckShipRange", {});
};

{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.RegisterTrigger("OnInitGame", "InitDanubius", { "enabled": true });
	cmpTrigger.RegisterTrigger("OnOwnershipChanged", "DanubiusOwnershipChange", { "enabled": true });
}
