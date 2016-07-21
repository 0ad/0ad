Trigger.prototype.CheckRegicideDefeat = function(data)
{
	if (data.entity == this.regicideHeroes[data.from])
		TriggerHelper.DefeatPlayer(data.from);
};

Trigger.prototype.InitRegicideGame = function(msg)
{
	let playersCivs = [];
	for (let playerID = 1; playerID < TriggerHelper.GetNumberOfPlayers(); ++playerID)
		playersCivs[playerID] = QueryPlayerIDInterface(playerID).GetCiv();

	// Get all hero templates of these civs
	let heroTemplates = {};
	let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	for (let templateName of cmpTemplateManager.FindAllTemplates(false))
	{
		if (templateName.substring(0,6) != "units/")
			continue;

		let identity = cmpTemplateManager.GetTemplate(templateName).Identity;
		let classes = GetIdentityClasses(identity);

		if (classes.indexOf("Hero") == -1 ||
		    playersCivs.every(civ => civ != identity.Civ))
			continue;

		if (!heroTemplates[identity.Civ])
			heroTemplates[identity.Civ] = [];

		if (heroTemplates[identity.Civ].indexOf(templateName) == -1)
			heroTemplates[identity.Civ].push({
				"templateName": templateName,
				"classes": classes
			});
	}

	// Sort available spawn points by preference
	let spawnPreference = ["Ship", "Structure", "CivilCentre"];
	let getSpawnPreference = entity => {
		let cmpIdentity = Engine.QueryInterface(entity, IID_Identity);
		let classes = cmpIdentity.GetClassesList();
		return spawnPreference.findIndex(className => classes.indexOf(className) != -1);
	};

	// Attempt to spawn one hero per player
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	for (let playerID = 1; playerID < TriggerHelper.GetNumberOfPlayers(); ++playerID)
	{
		let spawnPoints = cmpRangeManager.GetEntitiesByPlayer(playerID).sort((entity1, entity2) =>
			getSpawnPreference(entity2) - getSpawnPreference(entity1));

		this.regicideHeroes[playerID] = this.SpawnRegicideHero(playerID, heroTemplates[playersCivs[playerID]], spawnPoints);
	}
};

/**
 * Spawn a random hero at one of the given locations (which are checked in order).
 * Garrison it if the location is a ship.
 *
 * @param spawnPoints - entity IDs at which to spawn
 */
Trigger.prototype.SpawnRegicideHero = function(playerID, heroTemplates, spawnPoints)
{
	for (let heroTemplate of shuffleArray(heroTemplates))
		for (let spawnPoint of spawnPoints)
		{
			let cmpPosition = Engine.QueryInterface(spawnPoint, IID_Position);
			if (!cmpPosition || !cmpPosition.IsInWorld())
				continue;

			// Consider nomad maps where units start on a ship
			let isShip = TriggerHelper.EntityHasClass(spawnPoint, "Ship");
			if (isShip)
			{
				let cmpGarrisonHolder = Engine.QueryInterface(spawnPoint, IID_GarrisonHolder);
				if (cmpGarrisonHolder.IsFull() ||
				    !MatchesClassList(heroTemplate.classes, cmpGarrisonHolder.GetAllowedClasses()))
					continue;
			}

			let hero = TriggerHelper.SpawnUnits(spawnPoint, heroTemplate.templateName, 1, playerID);
			if (!hero.length)
				continue;

			hero = hero[0];

			if (isShip)
			{
				let cmpUnitAI = Engine.QueryInterface(hero, IID_UnitAI);
				cmpUnitAI.Garrison(spawnPoint);
			}

			return hero;
		}

	error("Couldn't spawn hero for player " + playerID);
	return undefined;
};

let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
cmpTrigger.regicideHeroes = [];
cmpTrigger.DoAfterDelay(0, "InitRegicideGame", {});
cmpTrigger.RegisterTrigger("OnOwnershipChanged", "CheckRegicideDefeat", { "enabled": true });
