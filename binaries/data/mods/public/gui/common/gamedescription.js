/**
 * Highlights the victory condition in the game-description.
 */
var g_DescriptionHighlight = "orange";

/**
 * The rating assigned to lobby players who didn't complete a ranked 1v1 yet.
 */
var g_DefaultLobbyRating = 1200;

/**
 * XEP-0172 doesn't restrict nicknames, but our lobby policy does.
 * So use this human readable delimiter to separate buddy names in the config file.
 */
var g_BuddyListDelimiter = ",";


/**
 * Returns the nickname without the lobby rating.
 */
function splitRatingFromNick(playerName)
{
	let result = /^(\S+)\ \((\d+)\)$/g.exec(playerName);
	return { "nick": result ? result[1] : playerName, "rating": result ? +result[2] : "" };
}

/**
 * Array of playernames that the current user has marked as buddies.
 */
var g_Buddies = Engine.ConfigDB_GetValue("user", "lobby.buddies").split(g_BuddyListDelimiter);

/**
 * Denotes which players are a lobby buddy of the current user.
 */
var g_BuddySymbol = '•';

/**
 * Returns a formatted string describing the player assignments.
 * Needs g_CivData to translate!
 *
 * @param {Object} playerDataArray - As known from game setup and sim state.
 * @param {(string[]|false)} playerStates - One of "won", "defeated", "active" for each player.
 * @returns {string}
 */
function formatPlayerInfo(playerDataArray, playerStates)
{
	let playerDescriptions = {};
	let playerIdx = 0;

	for (let playerData of playerDataArray)
	{
		if (playerData == null || playerData.Civ && playerData.Civ == "gaia")
			continue;

		++playerIdx;
		let teamIdx = playerData.Team;
		let isAI = playerData.AI && playerData.AI != "";
		let playerState = playerStates && playerStates[playerIdx] || playerData.State;
		let isActive = !playerState || playerState == "active";

		let playerDescription;
		if (isAI)
		{
			if (playerData.Civ)
			{
				if (isActive)
					// Translation: Describe a player in a selected game, f.e. in the replay- or savegame menu
					playerDescription = translate("%(playerName)s (%(civ)s, %(AIdescription)s)");
				else
					// Translation: Describe a player in a selected game, f.e. in the replay- or savegame menu
					playerDescription = translate("%(playerName)s (%(civ)s, %(AIdescription)s, %(state)s)");
			}
			else
			{
				if (isActive)
					// Translation: Describe a player in a selected game, f.e. in the replay- or savegame menu
					playerDescription = translate("%(playerName)s (%(AIdescription)s)");
				else
					// Translation: Describe a player in a selected game, f.e. in the replay- or savegame menu
					playerDescription = translate("%(playerName)s (%(AIdescription)s, %(state)s)");
			}
		}
		else
		{
			if (playerData.Offline)
			{
				// Can only occur in the lobby for now, so no strings with civ needed
				if (isActive)
					// Translation: Describe a player in a selected game, f.e. in the replay- or savegame menu
					playerDescription = translate("%(playerName)s (OFFLINE)");
				else
					// Translation: Describe a player in a selected game, f.e. in the replay- or savegame menu
					playerDescription = translate("%(playerName)s (OFFLINE, %(state)s)");
			}
			else
			{
				if (playerData.Civ)
					if (isActive)
						// Translation: Describe a player in a selected game, f.e. in the replay- or savegame menu
						playerDescription = translate("%(playerName)s (%(civ)s)");
					else
						// Translation: Describe a player in a selected game, f.e. in the replay- or savegame menu
						playerDescription = translate("%(playerName)s (%(civ)s, %(state)s)");
				else
					if (isActive)
						// Translation: Describe a player in a selected game, f.e. in the replay- or savegame menu
						playerDescription = translate("%(playerName)s");
					else
						// Translation: Describe a player in a selected game, f.e. in the replay- or savegame menu
						playerDescription = translate("%(playerName)s (%(state)s)");
			}
		}

		// Sort player descriptions by team
		if (!playerDescriptions[teamIdx])
			playerDescriptions[teamIdx] = [];

		let playerNick = splitRatingFromNick(playerData.Name).nick;
		playerDescriptions[teamIdx].push(sprintf(playerDescription, {
			"playerName":
			coloredText(
				(g_Buddies.indexOf(playerNick) != -1 ? g_BuddySymbol + " " : "") +
				escapeText(playerData.Name),
				(typeof getPlayerColor == 'function' ?
					(isAI ? "white" : getPlayerColor(playerNick)) :
					rgbToGuiColor(playerData.Color || g_Settings.PlayerDefaults[playerIdx].Color))),

			"civ":
				!playerData.Civ ?
					translate("Unknown Civilization") :
					g_CivData && g_CivData[playerData.Civ] && g_CivData[playerData.Civ].Name ?
						translate(g_CivData[playerData.Civ].Name) :
						playerData.Civ,

			"state":
				playerState == "defeated" ?
					translateWithContext("playerstate", "defeated") :
					translateWithContext("playerstate", "won"),

			"AIdescription": translateAISettings(playerData)
		}));
	}

	let teams = Object.keys(playerDescriptions);
	if (teams.indexOf("observer") > -1)
		teams.splice(teams.indexOf("observer"), 1);

	let teamDescription = [];

	// If there are no teams, merge all playersDescriptions
	if (teams.length == 1)
		teamDescription.push(playerDescriptions[teams[0]].join("\n"));

	// If there are teams, merge "Team N:" + playerDescriptions
	else
		teamDescription = teams.map(team => {

			let teamCaption = team == -1 ?
				translate("No Team") :
				sprintf(translate("Team %(team)s"), { "team": +team + 1 });

			// Translation: Describe players of one team in a selected game, f.e. in the replay- or savegame menu or lobby
			return sprintf(translate("%(team)s:\n%(playerDescriptions)s"), {
				"team": '[font="sans-bold-14"]' + teamCaption + "[/font]",
				"playerDescriptions": playerDescriptions[team].join("\n")
			});
		});

	if (playerDescriptions.observer)
		teamDescription.push(sprintf(translate("%(team)s:\n%(playerDescriptions)s"), {
			"team": '[font="sans-bold-14"]' + translatePlural("Observer", "Observers", playerDescriptions.observer.length) + "[/font]",
			"playerDescriptions": playerDescriptions.observer.join("\n")
		}));

	return teamDescription.join("\n\n");
}

/**
 * Sets an additional map label, map preview image and describes the chosen game settings more closely.
 *
 * Requires g_VictoryConditions.
 *
 * @param gameSettings - Serialised-format/JSON game settings.
 * (this takes serialised data to avoid loadings the gamesettings in the session GUI page)
 */
function getGameDescription(gameSettings, mapCache)
{
	let titles = [];
	if (!gameSettings.settings.VictoryConditions.length)
		titles.push({
			"label": translateWithContext("victory condition", "Endless Game"),
			"value": translate("No winner will be determined, even if everyone is defeated.")
		});

	for (let victoryCondition of g_VictoryConditions)
	{
		if (gameSettings.settings.VictoryConditions.indexOf(victoryCondition.Name) == -1)
			continue;

		let title = translateVictoryCondition(victoryCondition.Name);
		if (victoryCondition.Name == "wonder")
		{
			let wonderDuration = Math.round(gameSettings.settings.WonderDuration);
			title = sprintf(
				translatePluralWithContext(
					"victory condition",
					"Wonder (%(min)s minute)",
					"Wonder (%(min)s minutes)",
					wonderDuration
				),
				{ "min": wonderDuration });
		}

		let isCaptureTheRelic = victoryCondition.Name == "capture_the_relic";
		if (isCaptureTheRelic)
		{
			let relicDuration = Math.round(gameSettings.settings.RelicDuration);
			title = sprintf(
				translatePluralWithContext(
					"victory condition",
					"Capture the Relic (%(min)s minute)",
					"Capture the Relic (%(min)s minutes)",
					relicDuration
				),
				{ "min": relicDuration });
		}

		titles.push({
			"label": title,
			"value": victoryCondition.Description
		});

		if (isCaptureTheRelic)
			titles.push({
				"label": translate("Relic Count"),
				"value": Math.round(gameSettings.settings.RelicCount)
			});

		if (victoryCondition.Name == "regicide")
			if (gameSettings.settings.RegicideGarrison)
				titles.push({
					"label": translate("Hero Garrison"),
					"value": translate("Heroes can be garrisoned.")
				});
			else
				titles.push({
					"label": translate("Exposed Heroes"),
					"value": translate("Heroes cannot be garrisoned and they are vulnerable to raids.")
				});
	}

	if (gameSettings.settings.RatingEnabled &&
	    gameSettings.settings.PlayerData.length == 2)
		titles.push({
			"label": translate("Rated game"),
			"value": translate("When the winner of this match is determined, the lobby score will be adapted.")
		});

	if (gameSettings.settings.LockTeams)
		titles.push({
			"label": translate("Locked Teams"),
			"value": translate("Players can't change the initial teams.")
		});
	else
		titles.push({
			"label": translate("Diplomacy"),
			"value": translate("Players can make alliances and declare war on allies.")
		});

	if (gameSettings.settings.LastManStanding)
		titles.push({
			"label": translate("Last Man Standing"),
			"value": translate("Only one player can win the game. If the remaining players are allies, the game continues until only one remains.")
		});
	else
		titles.push({
			"label": translate("Allied Victory"),
			"value": translate("If one player wins, his or her allies win too. If one group of allies remains, they win.")
		});

	let ceasefire = Math.round(gameSettings.settings.Ceasefire);
	titles.push({
		"label": translate("Ceasefire"),
		"value":
			!ceasefire ?
				translate("disabled") :
				sprintf(translatePlural(
					"For the first minute, other players will stay neutral.",
					"For the first %(min)s minutes, other players will stay neutral.",
					ceasefire),
				{ "min": ceasefire })
	});

	if (gameSettings.map == "random")
		titles.push({
			"label": translateWithContext("Map Selection", "Random Map"),
			"value": translate("Randomly select a map from the list.")
		});
	else
	{
		titles.push({
			"label": translate("Map Name"),
			"value": mapCache.translateMapName(
				mapCache.getTranslatableMapName(gameSettings.mapType, gameSettings.map, gameSettings))
		});

		titles.push({
			"label": translate("Map Description"),
			"value": mapCache.getTranslatedMapDescription(gameSettings.mapType, gameSettings.map)
		});
	}

	titles.push({
		"label": translate("Map Type"),
		"value": g_MapTypes.Title[g_MapTypes.Name.indexOf(gameSettings.mapType)]
	});

	if (gameSettings.mapType == "random")
	{
		let mapSize = g_MapSizes.Name[g_MapSizes.Tiles.indexOf(gameSettings.settings.Size)];
		if (mapSize)
			titles.push({
				"label": translate("Map Size"),
				"value": mapSize
			});
	}

	if (gameSettings.settings.Biome)
	{
		let biome = g_Settings.Biomes.find(b => b.Id == gameSettings.settings.Biome);
		titles.push({
			"label": biome ? biome.Title : translateWithContext("biome", "Random Biome"),
			"value": biome ? biome.Description : translate("Randomly select a biome from the list.")
		});
	}

	if (gameSettings.settings.TriggerDifficulty !== undefined)
	{
		let triggerDifficulty = g_Settings.TriggerDifficulties.find(difficulty => difficulty.Difficulty == gameSettings.settings.TriggerDifficulty);
		titles.push({
			"label": triggerDifficulty.Title,
			"value": triggerDifficulty.Tooltip
		});
	}

	if (gameSettings.settings.Nomad !== undefined)
		titles.push({
			"label": gameSettings.settings.Nomad ? translate("Nomad Mode") : translate("Civic Centers"),
			"value":
				gameSettings.settings.Nomad ?
					translate("Players start with only few units and have to find a suitable place to build their city.") :
					translate("Players start with a Civic Center.")
		});

	if (gameSettings.settings.StartingResources !== undefined)
		titles.push({
			"label": translate("Starting Resources"),
			"value":
				gameSettings.settings.PlayerData &&
				gameSettings.settings.PlayerData.some(pData => pData && pData.Resources !== undefined) ?
					translateWithContext("starting resources", "Per Player") :
					sprintf(translate("%(startingResourcesTitle)s (%(amount)s)"), {
						"startingResourcesTitle":
							g_StartingResources.Title[
								g_StartingResources.Resources.indexOf(
									gameSettings.settings.StartingResources)],
						"amount": gameSettings.settings.StartingResources
					})
		});

	if (gameSettings.settings.PopulationCap !== undefined)
		titles.push({
			"label": translate("Population Limit"),
			"value":
				gameSettings.settings.PlayerData &&
				gameSettings.settings.PlayerData.some(pData => pData && pData.PopulationLimit !== undefined) ?
					translateWithContext("population limit", "Per Player") :
					g_PopulationCapacities.Title[
						g_PopulationCapacities.Population.indexOf(
							gameSettings.settings.PopulationCap)]
		});

	if (gameSettings.settings.WorldPopulationCap !== undefined)
		titles.push({
			"label": translate("World Population Cap"),
			"value":
				g_WorldPopulationCapacities.Title[
					g_WorldPopulationCapacities.Population.indexOf(
						gameSettings.settings.WorldPopulationCap)]
		});

	titles.push({
		"label": translate("Treasures"),
		"value": gameSettings.settings.DisableTreasures ?
			translateWithContext("treasures", "Disabled") :
			translateWithContext("treasures", "As defined by the map.")
	});

	titles.push({
		"label": translate("Revealed Map"),
		"value": gameSettings.settings.RevealMap
	});

	titles.push({
		"label": translate("Explored Map"),
		"value": gameSettings.settings.ExploreMap
	});

	titles.push({
		"label": translate("Cheats"),
		"value": gameSettings.settings.CheatsEnabled
	});

	return titles.map(title => sprintf(translate("%(label)s %(details)s"), {
		"label": coloredText(title.label, g_DescriptionHighlight),
		"details":
			title.value === true ? translateWithContext("game setup option", "enabled") :
				title.value || translateWithContext("game setup option", "disabled")
	})).join("\n");
}

/**
 * Sets the win/defeat icon to indicate current player's state.
 */
function setOutcomeIcon(state, image)
{
	if (state == "won")
	{
		image.sprite = "stretched:session/icons/victory.png";
		image.tooltip = translate("Victorious");
	}
	else if (state == "defeated")
	{
		image.sprite = "stretched:session/icons/defeat.png";
		image.tooltip = translate("Defeated");
	}
}

function translateAISettings(playerData)
{
	if (!playerData.AI)
		return "";

	return sprintf(translate("%(AIdifficulty)s %(AIbehavior)s %(AIname)s"), {
		"AIname": translateAIName(playerData.AI),
		"AIdifficulty": translateAIDifficulty(playerData.AIDiff),
		"AIbehavior": translateAIBehavior(playerData.AIBehavior),
	});
}
