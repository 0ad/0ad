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
		"units/athen_siege_lithobolos_packed",
		"units/athen_siege_oxybeles_packed",
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
	let attackerEntities = attackerEntityTemplates[Math.floor(Math.random() * attackerEntityTemplates.length)];
	// A soldier for each 2-3 minutes of the game. Should be waves of 20 soldiers after an hour
	let nextTime = Math.round(120000 + Math.random() * 60000);
	let attackerCount = Math.round(cmpTimer.GetTime() / nextTime / attackerEntities.length);

	// spawn attackers
	let attackers =  [];
	for (let attackerEntity of attackerEntities)
		attackers.push(TriggerHelper.SpawnUnitsFromTriggerPoints("A", attackerEntity, attackerCount, 0));

	for (let entityType of attackers)
	{
		for (let origin in entityType)
		{
			let cmpPlayer = QueryOwnerInterface(+origin, IID_Player);
			if (cmpPlayer.GetState() != "active")
				continue;

			let cmpPosition =  Engine.QueryInterface(this.playerCivicCenter[cmpPlayer.GetPlayerID()], IID_Position);
			// this shouldn't happen if the player is still active
			if (!cmpPosition || !cmpPosition.IsInWorld)
				continue;

			// store the x and z coordinates in the command
			let cmd = cmpPosition.GetPosition();
			cmd.type = "attack-walk";
			cmd.entities = entityType[origin];
			cmd.queued = true;
			cmd.targetClasses = undefined;
			// send the attack-walk command
			ProcessCommand(0, cmd);
		}
	}

	let cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	cmpGUIInterface.PushNotification({
		"message": markForTranslation("An enemy wave is attacking!"),
		"translateMessage": true
	});
	cmpTrigger.DoAfterDelay(nextTime, "StartAnEnemyWave", {}); // The next wave will come in 3 minutes
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
			if (TriggerHelper.EntityHasClass(entity, "CivilCentre"))
				cmpTrigger.playerCivicCenter[i] = entity;
	}

	// Fix alliances
	/* Until we can do something about limiting the victory conditions available for this map to "None"
	for (let i = 1; i < numberOfPlayers; ++i)
	{
		let cmpPlayer = QueryPlayerIDInterface(i);
		for (let j = 1; j < numberOfPlayers; ++j)
			if (i != j)
				cmpPlayer.SetAlly(j);
		cmpPlayer.SetLockTeams(true);
	}*/

	// Make gaia black
	QueryPlayerIDInterface(0).SetColor(0, 0, 0);

	// Place the treasures
	this.PlaceTreasures();

	// Disable farms, civic centers and walls for all players
	for (let i = 1; i < numberOfPlayers; ++i)
	{
		let cmpPlayer = QueryPlayerIDInterface(i);
		let civ = cmpPlayer.GetCiv();
		cmpPlayer.SetDisabledTemplates([
			"structures/" + civ + "_field",
			"structures/" + civ + "_corral",
			"structures/" + civ + "_civil_centre",
			"structures/" + civ + "_military_colony",
			"structures/" + civ + "_wallset_stone",
			"other/wallset_palisade"
		]);
	}
};

Trigger.prototype.PlaceTreasures = function()
{
	let point = ["B", "C", "D"][Math.floor(Math.random() * 3)];
	let triggerPoints = cmpTrigger.GetTriggerPoints(point);
	for (let point of triggerPoints)
	{
		let template = treasures[Math.floor(Math.random() * treasures.length)]
		TriggerHelper.SpawnUnits(point, template, 1, 0);
	}
	cmpTrigger.DoAfterDelay(4*60*1000, "PlaceTreasures", {}); //Place more treasures after 4 minutes
};

Trigger.prototype.InitializeEnemyWaves = function()
{
	let time = (5 + Math.round(Math.random() * 10)) * 60 * 1000;
	let cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	cmpGUIInterface.AddTimeNotification({
		"message": markForTranslation("The first wave will start in %(time)s!"),
		"translateMessage": true
	}, time);
	cmpTrigger.DoAfterDelay(time, "StartAnEnemyWave", {});
};

Trigger.prototype.DefeatPlayerOnceCCIsDestroyed = function(data)
{
	// Defeat a player that has lost his civic center
	if (data.entity == cmpTrigger.playerCivicCenter[data.from])
	{
		TriggerHelper.DefeatPlayer(data.from);

		// Check if only one player remains. He will be the winner.
		let lastPlayerStanding = 0;
		let numPlayersStanding = 0;
		let numberOfPlayers = TriggerHelper.GetNumberOfPlayers();
		for (let i = 1; i < numberOfPlayers; ++i)
		{
			if (QueryPlayerIDInterface(i).GetState() == "active")
			{
				lastPlayerStanding = i;
				++numPlayersStanding;
			}
		}
		if (numPlayersStanding == 1)
			TriggerHelper.SetPlayerWon(lastPlayerStanding);
	}
};

var cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
cmpTrigger.playerCivicCenter = {};
cmpTrigger.DoAfterDelay(0, "InitGame", {});
cmpTrigger.DoAfterDelay(1000, "InitializeEnemyWaves", {});

cmpTrigger.RegisterTrigger("OnOwnershipChanged", "DefeatPlayerOnceCCIsDestroyed", { "enabled": true });
