/**
 * The city is patroled along its paths by infantry champions that respawn reoccuringly.
 * There are increasingly great gaia attacks started from the different buildings.
 * The players can destroy gaia buildings to reduce the number of attackers for the future.
 */

/**
 * If set to true, it will print how many templates would be spawned if the players were not defeated.
 */
const dryRun = false;

/**
 * If enabled, prints the number of units to the command line output.
 */
const showDebugLog = false;

// TODO: harass attackers

var jebelBarkal_rank = "Advanced";

/**
 * These are the templates spawned at the gamestart and during the game.
 */
var jebelBarkal_templateClasses = deepfreeze({
	"heroes": "Hero",
	"champions": "Champion+!Elephant",
	"elephants": "Champion+Elephant",
	"champion_infantry": "Champion+Infantry",
	"champion_infantry_melee": "Champion+Infantry+Melee",
	"champion_infantry_ranged": "Champion+Infantry+Ranged",
	"champion_cavalry": "Champion+Cavalry",
	"citizenSoldiers": "CitizenSoldier",
	"citizenSoldier_infantry": "CitizenSoldier+Infantry",
	"citizenSoldier_infantry_melee": "CitizenSoldier+Infantry+Melee",
	"citizenSoldier_infantry_ranged": "CitizenSoldier+Infantry+Ranged",
	"citizenSoldier_cavalry": "CitizenSoldier+Cavalry",
	"healers": "Healer",
	"females": "FemaleCitizen"
});

var jebelBarkal_templates = deepfreeze(Object.keys(jebelBarkal_templateClasses).reduce((templates, name) => {
	templates[name] = TriggerHelper.GetTemplateNamesByClasses(jebelBarkal_templateClasses[name], "kush", undefined, jebelBarkal_rank, true);
	return templates;
}, {}));

/**
 * These are the formations patroling and attacking units can use.
*/
var jebelBarkal_formations = [
	"special/formations/line_closed",
	"special/formations/column_closed"
];

/**
 *  Balancing helper function.
 *
 *  @returns min0 value at the beginning of the game, min60 after an hour of gametime or longer and
 *  a proportionate number between these two values before the first hour is reached.
 */
var scaleByTime = (minCurrent, min0, min60) => min0 + (min60 - min0) * Math.min(1, minCurrent / 60);

/**
 *  @returns min0 value at the beginning of the game, min60 after an hour of gametime or longer and
 *  a proportionate number between these two values before the first hour is reached.
 */
var scaleByMapSize = (min, max) => min + (max - min) * (TriggerHelper.GetMapSizeTiles() - 128) / (512 - 128);

/**
 * Defensive Infantry units patrol along the paths of the city.
 */
var jebelBarkal_cityPatrolGroup_count = time => scaleByTime(time, 3, scaleByMapSize(3, 10));
var jebelBarkal_cityPatrolGroup_interval = time => scaleByTime(time, 5, 3);
var jebelBarkal_cityPatrolGroup_balancing = {
	"buildingClasses": ["Wonder", "Temple", "CivCentre", "Fortress", "Barracks", "Embassy"],
	"unitCount": time => Math.min(20, scaleByTime(time, 10, 45)),
	"unitComposition": (time, heroes) => [
		{
			"templates": jebelBarkal_templates.champion_infantry_melee,
			"frequency": scaleByTime(time, 0, 2)
		},
		{
			"templates": jebelBarkal_templates.champion_infantry_ranged,
			"frequency": scaleByTime(time, 0, 3)
		},
		{
			"templates": jebelBarkal_templates.citizenSoldier_infantry_melee,
			"frequency": scaleByTime(time, 2, 0)
		},
		{
			"templates": jebelBarkal_templates.citizenSoldier_infantry_ranged,
			"frequency": scaleByTime(time, 3, 0)
		}
	]
};

/**
 * Frequently the buildings spawn different units that attack the players groupwise.
 */
var jebelBarkal_attackInterval = time => randFloat(5, 7);

/**
 * Frequently cavalry is spawned at few buildings to harass the enemy trade.
 */
var jebelBarkal_harassInterval = time => randFloat(6, 8);

/**
 * Assume gaia to be the native kushite player.
 */
var jebelBarkal_playerID = 0;

/**
 * Soldiers will patrol along the city grid.
 */
var jebelBarkal_triggerPointPath = "A";

/**
 * Attacker groups approach these player buildings and attack enemies of the given classes encountered.
 */
var jebelBarkal_pathTargetClasses = "CivCentre Wonder";

/**
 * Soldiers and siege towers prefer these targets when attacking or patroling.
 */
var jebelBarkal_soldierTargetClasses = "Unit+!Ship";

/**
 * Elephants focus these units when attacking.
 */
var jebelBarkal_elephantTargetClasses = "Defensive SiegeEngine";

/**
 * This defines which units are spawned and garrisoned at the gamestart per building.
 */
var jebelBarkal_buildingGarrison = [
	{
		"buildingClasses": ["Wonder", "Temple", "CivCentre", "Fortress"],
		"unitTemplates": jebelBarkal_templates.champions
	},
	{
		"buildingClasses": ["Barracks", "Embassy"],
		"unitTemplates": [...jebelBarkal_templates.citizenSoldiers, ...jebelBarkal_templates.champions]
	},
	{
		"buildingClasses": ["DefenseTower"],
		"unitTemplates": jebelBarkal_templates.champion_infantry
	},
	{
		"buildingClasses": ["ElephantStables"],
		"unitTemplates": jebelBarkal_templates.elephants
	},
	{
		"buildingClasses": ["Stable"],
		"unitTemplates": jebelBarkal_templates.champion_cavalry
	},
	{
		"buildingClasses": ["Pyramid"],
		"unitTemplates": [...jebelBarkal_templates.citizenSoldiers, ...jebelBarkal_templates.healers]
	},
	{
		"buildingClasses": ["House"],
		"unitTemplates": [...jebelBarkal_templates.females, ...jebelBarkal_templates.healers]
	}
];

/**
 * This defines which units are spawned at the different buildings at the given time.
 * The buildings are ordered by strength.
 * Notice that there are always 2 groups of these count spawned, one for each side!
 * The units should do a walk-attack to random player CCs
 */
var jebelBarkal_attackerGroup_balancing = [
	{
		"buildingClasses": ["Wonder"],
		"unitCount": time => scaleByTime(time, 0, 85),
		"unitComposition": (time, heroes) => [
			{
				"templates": jebelBarkal_templates.heroes,
				"count": randBool(scaleByTime(time, -0.5, 2)) ? 1 : 0,
				"unique_entities": heroes
			},
			{
				"templates": jebelBarkal_templates.healers,
				"frequency": randFloat(0, 0.1)
			},
			{
				"templates": jebelBarkal_templates.champions,
				"frequency": scaleByTime(time, 0, 0.6)
			},
			{
				"templates": jebelBarkal_templates.champion_infantry_ranged,
				"frequency": scaleByTime(time, 0, 0.4)
			},
			{
				"templates": jebelBarkal_templates.citizenSoldiers,
				"frequency": scaleByTime(time, 1, 0)
			},
			{
				"templates": jebelBarkal_templates.citizenSoldier_infantry_ranged,
				"frequency": scaleByTime(time, 1, 0)
			}
		]
	},
	{
		"buildingClasses": ["Fortress"],
		"unitCount": time => scaleByTime(time, 0, 45),
		"unitComposition": (time, heroes) => [
			{
				"templates": jebelBarkal_templates.heroes,
				"count": randBool(scaleByTime(time, -0.5, 1.5)) ? 1 : 0,
				"unique_entities": heroes
			},
			{
				"templates": jebelBarkal_templates.champions,
				"frequency": scaleByTime(time, 0, 1)
			}
		]
	},
	{
		"buildingClasses": ["Temple"],
		"unitCount": time => Math.min(45, scaleByTime(time, 0, 90)),
		"unitComposition": (time, heroes) => [
			{
				"templates": jebelBarkal_templates.heroes,
				"count": randBool(scaleByTime(time, -0.5, 1)) ? 1 : 0,
				"unique_entities": heroes
			},
			{
				"templates": jebelBarkal_templates.champion_infantry_melee,
				"frequency": 0.5
			},
			{
				"templates": jebelBarkal_templates.champion_infantry_ranged,
				"frequency": 0.5
			},
			{
				"templates": jebelBarkal_templates.healers,
				"frequency": randFloat(0.05, 0.2)
			}
		]
	},
	{
		"buildingClasses": ["CivCentre"],
		"unitCount": time => Math.min(40, scaleByTime(time, 0, 80)),
		"unitComposition": (time, heroes) => [
			{
				"templates": jebelBarkal_templates.heroes,
				"count": randBool(scaleByTime(time, -0.5, 0.5)) ? 1 : 0,
				"unique_entities": heroes
			},
			{
				"templates": jebelBarkal_templates.champion_infantry,
				"frequency": scaleByTime(time, 0, 1)
			},
			{
				"templates": jebelBarkal_templates.citizenSoldiers,
				"frequency": scaleByTime(time, 1, 0)
			}
		]
	},
	{
		"buildingClasses": ["Stable"],
		"unitCount": time => Math.min(30, scaleByTime(time, 0, 80)),
		"harasserSize": time => Math.min(20, scaleByTime(time, 0, 50)),
		"unitComposition": (time, heroes) => [
			{
				"templates": jebelBarkal_templates.citizenSoldier_cavalry,
				"frequency": scaleByTime(time, 2, 0)
			},
			{
				"templates": jebelBarkal_templates.champion_cavalry,
				"frequency": scaleByTime(time, 0, 1)
			}
		]
	},
	{
		"buildingClasses": ["Barracks", "Embassy"],
		"unitCount": time => Math.min(35, scaleByTime(time, 0, 70)),
		"unitComposition": (time, heroes) => [
			{
				"templates": jebelBarkal_templates.citizenSoldier_infantry,
				"frequency": 1
			}
		]
	},
	{
		"buildingClasses": ["ElephantStables"],
		"unitCount": time => scaleByTime(time, 0, 10),
		"unitComposition": (time, heroes) => [
			{
				"templates": jebelBarkal_templates.elephants,
				"frequency": 1
			}
		]
	}
];

Trigger.prototype.debugLog = function(txt)
{
	if (showDebugLog)
		print("DEBUG [" + Math.round(TriggerHelper.GetMinutes()) + "] " + txt + "\n");
};

Trigger.prototype.JebelBarkal_Init = function()
{
	this.JebelBarkal_TrackUnits();
	this.JebelBarkal_SetDefenderStance();
	this.JebelBarkal_GarrisonBuildings();
	this.JebelBarkal_SpawnCityPatrolGroups();
	this.JebelBarkal_SpawnAttackerGroups();
};

Trigger.prototype.JebelBarkal_TrackUnits = function()
{
	// Each item is an entity ID
	this.jebelBarkal_heroes = [];

	// Each item is an array of entity IDs
	this.jebelBarkal_patrolingUnits = [];

	// Array of entityIDs where patrol groups can spawn
	this.jebelBarkal_patrolGroupSpawnPoints = TriggerHelper.GetPlayerEntitiesByClass(
		jebelBarkal_playerID,
		jebelBarkal_cityPatrolGroup_balancing.buildingClasses);

	this.debugLog("Patrol spawn points: " + uneval(this.jebelBarkal_patrolGroupSpawnPoints));

	// Array of entityIDs where attacker groups can spawn
	this.jebelBarkal_attackerGroupSpawnPoints = TriggerHelper.GetPlayerEntitiesByClass(
		jebelBarkal_playerID,
		jebelBarkal_attackerGroup_balancing.reduce((classes, attackerSpawning) => classes.concat(attackerSpawning.buildingClasses), []));

	this.debugLog("Attacker spawn points: " + uneval(this.jebelBarkal_attackerGroupSpawnPoints));
};

Trigger.prototype.JebelBarkal_SetDefenderStance = function()
{
	for (let ent of TriggerHelper.GetPlayerEntitiesByClass(jebelBarkal_playerID, "Soldier"))
		TriggerHelper.SetUnitStance(ent, "defensive");
};

Trigger.prototype.JebelBarkal_GarrisonBuildings = function()
{
	for (let buildingGarrison of jebelBarkal_buildingGarrison)
		TriggerHelper.SpawnAndGarrisonAtClasses(jebelBarkal_playerID, buildingGarrison.buildingClasses, buildingGarrison.unitTemplates, 1);
};

/**
 * Spawn new groups if old ones were wiped out.
 */
Trigger.prototype.JebelBarkal_SpawnCityPatrolGroups = function()
{
	if (!this.jebelBarkal_patrolGroupSpawnPoints.length)
		return;

	let time = TriggerHelper.GetMinutes();
	let groupCount = Math.floor(Math.max(0, jebelBarkal_cityPatrolGroup_count(time)) - this.jebelBarkal_patrolingUnits.length);

	for (let i = 0; i < groupCount; ++i)
	{
		let spawnEnt = pickRandom(this.jebelBarkal_patrolGroupSpawnPoints);

		let templateCounts = TriggerHelper.BalancedTemplateComposition(
			jebelBarkal_cityPatrolGroup_balancing.unitComposition(time, this.jebelBarkal_heroes),
			jebelBarkal_cityPatrolGroup_balancing.unitCount(time));

		this.debugLog("Spawning " + groupCount + " city patrol groups, " +
			this.jebelBarkal_patrolingUnits.length + " exist, templates:\n" + uneval(templateCounts));

		let groupEntities = this.JebelBarkal_SpawnTemplates(spawnEnt, templateCounts);

		this.jebelBarkal_patrolingUnits.push(groupEntities);

		for (let ent of groupEntities)
			TriggerHelper.SetUnitStance(ent, "defensive");

		TriggerHelper.SetUnitFormation(jebelBarkal_playerID, groupEntities, pickRandom(jebelBarkal_formations));

		for (let entTriggerPoint of this.GetTriggerPoints(jebelBarkal_triggerPointPath))
		{
			let pos = TriggerHelper.GetEntityPosition2D(entTriggerPoint);
			ProcessCommand(jebelBarkal_playerID, {
				"type": "patrol",
				"entities": groupEntities,
				"x": pos.x,
				"z": pos.y,
				"targetClasses": {
					"attack": jebelBarkal_soldierTargetClasses
				},
				"queued": true,
				"allowCapture": false
			});
		}
	}

	this.DoAfterDelay(jebelBarkal_cityPatrolGroup_interval(time) * 60 * 1000, "JebelBarkal_SpawnCityPatrolGroups", {});
};

Trigger.prototype.JebelBarkal_SpawnTemplates = function(spawnEnt, templateCounts)
{
	let groupEntities = [];

	for (let templateName in templateCounts)
	{
		let ents = TriggerHelper.SpawnUnits(spawnEnt, templateName, templateCounts[templateName], jebelBarkal_playerID);

		groupEntities = groupEntities.concat(ents);

		if (jebelBarkal_templates.heroes.indexOf(templateName) != -1 && ents[0])
			this.jebelBarkal_heroes.push(ents[0]);
	}

	return groupEntities;
};

/**
 * Spawn a group of attackers at every remaining building.
 */
Trigger.prototype.JebelBarkal_SpawnAttackerGroups = function()
{
	let time = TriggerHelper.GetMinutes();
	this.debugLog("Attacker wave");

	let targets = TriggerHelper.GetAllPlayersEntitiesByClass(jebelBarkal_pathTargetClasses);
	if (!targets.length)
		return;

	let spawnedAnything = false;
	for (let spawnEnt of this.jebelBarkal_attackerGroupSpawnPoints)
	{
		let spawnPointBalancing = jebelBarkal_attackerGroup_balancing.find(balancing =>
			TriggerHelper.EntityMatchesClassList(spawnEnt, balancing.buildingClasses));

		let unitCount = Math.round(spawnPointBalancing.unitCount(time));

		if (!unitCount)
			continue;

		let templateCounts = TriggerHelper.BalancedTemplateComposition(spawnPointBalancing.unitComposition(time, this.jebelBarkal_heroes), unitCount);

		this.debugLog("Spawning " + unitCount + " attackers at " + uneval(spawnPointBalancing.buildingClasses) + " " + spawnEnt + ":\n" + uneval(templateCounts));

		if (dryRun)
			continue;

		let groupEntities = this.JebelBarkal_SpawnTemplates(spawnEnt, templateCounts);
		spawnedAnything = true;

		let isElephant = TriggerHelper.EntityMatchesClassList(groupEntities[0], "Elephant");

		for (let ent of groupEntities)
			TriggerHelper.SetUnitStance(ent, isElephant ? "aggressive" : "violent");

		if (!isElephant)
			TriggerHelper.SetUnitFormation(jebelBarkal_playerID, groupEntities, pickRandom(jebelBarkal_formations));

		let targetPos = TriggerHelper.GetEntityPosition2D(pickRandom(targets));
		if (!targetPos)
			continue;

		ProcessCommand(jebelBarkal_playerID, {
			"type": "attack-walk",
			"entities": groupEntities,
			"x": targetPos.x,
			"z": targetPos.y,
			"targetClasses": isElephant ? jebelBarkal_elephantTargetClasses : jebelBarkal_soldierTargetClasses,
			"allowCapture": false,
			"queued": false
		});
	}

	if (spawnedAnything)
		Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface).PushNotification({
			"message": markForTranslation("Napata is attacking!"),
			"translateMessage": true
		});

	this.DoAfterDelay(jebelBarkal_attackInterval(TriggerHelper.GetMinutes()) * 60 * 1000, "JebelBarkal_SpawnAttackerGroups", {});
};

/**
 * Keep track of heroes, so that each of them remains unique.
 * Keep track of spawn points, as only there units should be spawned.
 * 
 */
Trigger.prototype.JebelBarkal_OwnershipChange = function(data)
{
	if (data.from != 0)
		return;

	for (let array of [this.jebelBarkal_heroes, this.jebelBarkal_patrolGroupSpawnPoints, this.jebelBarkal_attackerGroupSpawnPoints, ...this.jebelBarkal_patrolingUnits])
	{
		let idx = array.indexOf(data.entity);
		if (idx != -1)
			array.splice(idx, 1);
	}

	this.jebelBarkal_patrolingUnits = this.jebelBarkal_patrolingUnits.filter(entities => entities.length);
};


{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.RegisterTrigger("OnInitGame", "JebelBarkal_Init", { "enabled": true });
	cmpTrigger.RegisterTrigger("OnOwnershipChanged", "JebelBarkal_OwnershipChange", { "enabled": true });
}
