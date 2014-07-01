var treasures = ["gaia/special_treasure_food_barrel",
				 "gaia/special_treasure_food_bin",
				 "gaia/special_treasure_food_crate",
				 "gaia/special_treasure_food_jars",
				 "gaia/special_treasure_metal",
				 "gaia/special_treasure_stone",
				 "gaia/special_treasure_wood",
				 "gaia/special_treasure_wood",
				 "gaia/special_treasure_wood"];
var attackerEntityTemplates = ["units/athen_champion_infantry",
							   "units/athen_champion_marine",
							   "units/athen_champion_ranged",
							   "units/brit_champion_cavalry",
							   "units/brit_champion_infantry",
							   "units/cart_champion_cavalry",
							   "units/cart_champion_elephant",
							   "units/cart_champion_infantry",
							   "units/cart_champion_pikeman",
							   "units/gaul_champion_cavalry",
							   "units/gaul_champion_fanatic",
							   "units/gaul_champion_infantry",
							   "units/iber_champion_cavalry",
							   "units/iber_champion_infantry",
							   "units/mace_champion_cavalry",
							   "units/mace_champion_infantry_a",
							   "units/mace_champion_infantry_e",
							   "units/maur_champion_chariot",
							   "units/maur_champion_elephant",
							   "units/maur_champion_infantry",
							   "units/maur_champion_maiden",
							   "units/maur_champion_maiden_archer",
							   "units/pers_champion_cavalry",
							   "units/pers_champion_infantry",
							   "units/ptol_champion_cavalry",
							   "units/ptol_champion_elephant",
							   "units/rome_champion_cavalry",
							   "units/rome_champion_infantry",
							   "units/sele_champion_cavalry",
							   "units/sele_champion_chariot",
							   "units/sele_champion_elephant",
							   "units/sele_champion_infantry_pikeman",
							   "units/sele_champion_infantry_swordsman",
							   "units/spart_champion_infantry_pike",
							   "units/spart_champion_infantry_spear",
							   "units/spart_champion_infantry_sword"];

Trigger.prototype.StartAnEnemyWave = function()
{
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	var attackerEntity = attackerEntityTemplates[Math.floor(Math.random() * attackerEntityTemplates.length)];
	var attackerCount = Math.round(cmpTimer.GetTime() / 180000); // A soldier for each 3 minutes of the game. Should be waves of 20 soldiers after an hour

	// spawn attackers
	var attackers =  TriggerHelper.SpawnUnitsFromTriggerPoints("A", attackerEntity, attackerCount, 0);

	for (var origin in attackers)
	{
		var cmpPlayer = QueryOwnerInterface(+origin, IID_Player);
		if (cmpPlayer.GetState() != "active")
			continue;

		var cmpPosition =  Engine.QueryInterface(this.playerCivicCenter[cmpPlayer.GetPlayerID()], IID_Position);
		// this shouldn't happen if the player is still active
		if (!cmpPosition || !cmpPosition.IsInWorld)
			continue;

		// store the x and z coordinates in the command
		var cmd = cmpPosition.GetPosition();
		cmd.type = "attack-walk";
		cmd.entities = attackers[origin];
		cmd.queued = true;
		// send the attack-walk command
		ProcessCommand(0, cmd);
	}

	var cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	cmpGUIInterface.PushNotification({
		"players": [1,2,3,4,5,6,7,8], 
		"message": markForTranslation("An enemy wave is attacking!"),
		"translateMessage": true
	});
	cmpTrigger.DoAfterDelay(180000, "StartAnEnemyWave", {}); // The next wave will come in 3 minutes
}

Trigger.prototype.InitGame = function()
{
	var numberOfPlayers = TriggerHelper.GetNumberOfPlayers();
	// Find all of the civic centers
	for (var i = 1; i < numberOfPlayers; ++i)
	{
		var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		var playerEntities = cmpRangeManager.GetEntitiesByPlayer(i); // Get all of each player's entities
		
		for each (var entity in playerEntities)
		{
			if (TriggerHelper.EntityHasClass(entity, "CivilCentre"))
			{
				cmpTrigger.playerCivicCenter[i] = entity;
			}
		}
	}

	// make gaia black
	TriggerHelper.GetPlayerComponent(0).SetColour(0, 0, 0);
	
	// Place the treasures
	this.PlaceTreasures();
}

Trigger.prototype.PlaceTreasures = function()
{
	var triggerPoints = cmpTrigger.GetTriggerPoints("B");
	for (var point of triggerPoints)
	{
		var template = treasures[Math.floor(Math.random() * treasures.length)]
		TriggerHelper.SpawnUnits(point, template, 1, 0);
	}
	cmpTrigger.DoAfterDelay(4*60*1000, "PlaceTreasures", {}); //Place more treasures after 4 minutes
}

Trigger.prototype.InitializeEnemyWaves = function()
{
	var cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	cmpGUIInterface.PushNotification({
		"players": [1,2,3,4,5,6,7,8], 
		"message": markForTranslation("The first wave will start in 15 minutes!"),
		"translateMessage": true
	});
	cmpTrigger.DoAfterDelay(15*60*1000, "StartAnEnemyWave", {});
}

Trigger.prototype.DefeatPlayerOnceCCIsDestroyed = function(data)
{
	// Defeat a player that has lost his civic center
	if (data.entity == cmpTrigger.playerCivicCenter[data.from] && data.to == -1)
		TriggerHelper.DefeatPlayer(data.from);
	
	// Check if only one player remains. He will be the winner.
	var lastPlayerStanding = 0;
	var numPlayersStanding = 0;
	var numberOfPlayers = TriggerHelper.GetNumberOfPlayers();
	for (var i = 1; i < numberOfPlayers; ++i)
	{
		if (TriggerHelper.GetPlayerComponent(i).GetState() == "active")
		{
			lastPlayerStanding = i;
			++numPlayersStanding;
		}
	}
	if (numPlayersStanding == 1)
		TriggerHelper.SetPlayerWon(lastPlayerStanding);
}

var cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
cmpTrigger.playerCivicCenter = {};
cmpTrigger.DoAfterDelay(0, "InitGame", {}); 
cmpTrigger.DoAfterDelay(1000, "InitializeEnemyWaves", {});

cmpTrigger.RegisterTrigger("OnOwnershipChanged", "DefeatPlayerOnceCCIsDestroyed", {"enabled": true});
