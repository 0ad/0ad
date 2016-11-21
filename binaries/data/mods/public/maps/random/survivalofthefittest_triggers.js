var disabledTemplates = (civ) => [
	// Economic structures
	"structures/" + civ + "_corral",
	"structures/" + civ + "_farmstead",
	"structures/" + civ + "_field",
	"structures/" + civ + "_storehouse",
	"structures/brit_rotarymill",
	"units/maur_support_elephant",

	// Expansions
	"structures/" + civ + "_civil_centre",
	"structures/" + civ + "_military_colony",

	// Walls
	"structures/" + civ + "_wallset_stone",
	"structures/rome_wallset_siege",
	"other/wallset_palisade",

	// Shoreline
	"structures/" + civ + "_dock",
	"structures/brit_crannog",
	"structures/cart_super_dock",
	"structures/ptol_lighthouse"
];

var treasures =
[
	"gaia/special_treasure_food_barrel",
	"gaia/special_treasure_food_bin",
	"gaia/special_treasure_food_crate",
	"gaia/special_treasure_food_jars",
	"gaia/special_treasure_metal",
	"gaia/special_treasure_stone",
	"gaia/special_treasure_wood",
	"gaia/special_treasure_wood",
	"gaia/special_treasure_wood"
];

var attackerEntityTemplates =
[
	[
		"units/athen_champion_infantry",
		"units/athen_champion_marine",
		"units/athen_champion_ranged",
		"units/athen_mechanical_siege_lithobolos_packed",
		"units/athen_mechanical_siege_oxybeles_packed",
	],
	[
		"units/brit_champion_cavalry",
		"units/brit_champion_infantry",
		"units/brit_mechanical_siege_ram",
	],
	[
		"units/cart_champion_cavalry",
		"units/cart_champion_elephant",
		"units/cart_champion_infantry",
		"units/cart_champion_pikeman",
	],
	[
		"units/gaul_champion_cavalry",
		"units/gaul_champion_fanatic",
		"units/gaul_champion_infantry",
		"units/gaul_mechanical_siege_ram",
	],
	[
		"units/iber_champion_cavalry",
		"units/iber_champion_infantry",
		"units/iber_mechanical_siege_ram",
	],
	[
		"units/mace_champion_cavalry",
		"units/mace_champion_infantry_a",
		"units/mace_champion_infantry_e",
		"units/mace_mechanical_siege_lithobolos_packed",
		"units/mace_mechanical_siege_oxybeles_packed",
	],
	[
		"units/maur_champion_chariot",
		"units/maur_champion_elephant",
		"units/maur_champion_infantry",
		"units/maur_champion_maiden",
		"units/maur_champion_maiden_archer",
	],
	[
		"units/pers_champion_cavalry",
		"units/pers_champion_infantry",
		"units/pers_champion_elephant",
	],
	[
		"units/ptol_champion_cavalry",
		"units/ptol_champion_elephant",
	],
	[
		"units/rome_champion_cavalry",
		"units/rome_champion_infantry",
		"units/rome_mechanical_siege_ballista_packed",
		"units/rome_mechanical_siege_scorpio_packed",
	],
	[
		"units/sele_champion_cavalry",
		"units/sele_champion_chariot",
		"units/sele_champion_elephant",
		"units/sele_champion_infantry_pikeman",
		"units/sele_champion_infantry_swordsman",
	],
	[
		"units/spart_champion_infantry_pike",
		"units/spart_champion_infantry_spear",
		"units/spart_champion_infantry_sword",
		"units/spart_mechanical_siege_ram",
	],
];

Trigger.prototype.StartAnEnemyWave = function()
{
	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	let attackerTemplates = attackerEntityTemplates[Math.floor(Math.random() * attackerEntityTemplates.length)];
	// A soldier for each 2-3 minutes of the game. Should be waves of 20 soldiers after an hour
	let nextTime = Math.round(120000 + Math.random() * 60000);
	let attackersPerTemplate = Math.ceil(cmpTimer.GetTime() / nextTime / attackerTemplates.length);
	let spawned = false;

	for (let point of this.GetTriggerPoints("A"))
	{
		let cmpPlayer = QueryOwnerInterface(point, IID_Player);
		if (cmpPlayer.GetPlayerID() == 0 || cmpPlayer.GetState() != "active")
			continue;

		let cmpPosition =  Engine.QueryInterface(this.playerCivicCenter[cmpPlayer.GetPlayerID()], IID_Position);
		if (!cmpPosition || !cmpPosition.IsInWorld)
			continue;
		let targetPos = cmpPosition.GetPosition();

		for (let template of attackerTemplates)
		{
			let entities = TriggerHelper.SpawnUnits(point, template, attackersPerTemplate, 0);

			ProcessCommand(0, {
				"type": "attack-walk",
				"entities": entities,
				"x": targetPos.x,
				"z": targetPos.z,
				"queued": true,
				"targetClasses": undefined
			});
		}
		spawned = true;
	}

	if (!spawned)
		return;

	let cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	cmpGUIInterface.PushNotification({
		"message": markForTranslation("An enemy wave is attacking!"),
		"translateMessage": true
	});
	this.DoAfterDelay(nextTime, "StartAnEnemyWave", {}); // The next wave will come in 3 minutes
};

Trigger.prototype.InitGame = function()
{
	let numberOfPlayers = TriggerHelper.GetNumberOfPlayers();
	// Find all of the civic centers, disable some structures
	for (let i = 1; i < numberOfPlayers; ++i)
	{
		let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		let playerEntities = cmpRangeManager.GetEntitiesByPlayer(i); // Get all of each player's entities

		for (let entity of playerEntities)
		{
			if (TriggerHelper.EntityHasClass(entity, "CivilCentre"))
				this.playerCivicCenter[i] = entity;
			else if (TriggerHelper.EntityHasClass(entity, "Female"))
			{
				let cmpDamageReceiver = Engine.QueryInterface(entity, IID_DamageReceiver);
				cmpDamageReceiver.SetInvulnerability(true);

				let cmpHealth = Engine.QueryInterface(entity, IID_Health);
				cmpHealth.SetUndeletable(true);
			}
		}
	}

	this.PlaceTreasures();

	for (let i = 1; i < numberOfPlayers; ++i)
	{
		let cmpPlayer = QueryPlayerIDInterface(i);
		let civ = cmpPlayer.GetCiv();
		cmpPlayer.SetDisabledTemplates(disabledTemplates(civ));
	}
};

Trigger.prototype.PlaceTreasures = function()
{
	let point = ["B", "C", "D"][Math.floor(Math.random() * 3)];
	let triggerPoints = this.GetTriggerPoints(point);
	for (let point of triggerPoints)
	{
		let template = treasures[Math.floor(Math.random() * treasures.length)];
		TriggerHelper.SpawnUnits(point, template, 1, 0);
	}
	this.DoAfterDelay(4*60*1000, "PlaceTreasures", {}); // Place more treasures after 4 minutes
};

Trigger.prototype.InitializeEnemyWaves = function()
{
	let time = (5 + Math.round(Math.random() * 10)) * 60 * 1000;
	let cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	cmpGUIInterface.AddTimeNotification({
		"message": markForTranslation("The first wave will start in %(time)s!"),
		"translateMessage": true
	}, time);
	this.DoAfterDelay(time, "StartAnEnemyWave", {});
};

Trigger.prototype.DefeatPlayerOnceCCIsDestroyed = function(data)
{
	if (data.entity == this.playerCivicCenter[data.from])
		TriggerHelper.DefeatPlayer(data.from);
};


{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.playerCivicCenter = {};
	cmpTrigger.DoAfterDelay(0, "InitGame", {});
	cmpTrigger.DoAfterDelay(1000, "InitializeEnemyWaves", {});
	cmpTrigger.RegisterTrigger("OnOwnershipChanged", "DefeatPlayerOnceCCIsDestroyed", { "enabled": true });
}
