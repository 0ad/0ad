/**
 * The maximum number of players that the engine supports.
 * TODO: Maybe we can support more than 8 players sometime.
 */
const g_MaxPlayers = 8;

/**
 * The maximum number of teams allowed.
 */
const g_MaxTeams = 4;

/**
 * Directory containing all editable settings.
 */
const g_SettingsDirectory = "simulation/data/settings/";

/**
 * An object containing all values given by setting name.
 * Used by lobby, gamesetup, session, summary screen and replay menu.
 */
const g_Settings = loadSettingsValues();

/**
 * Loads and translates all values of all settings which
 * can be configured by dropdowns in the gamesetup.
 *
 * @returns {Object|undefined}
 */
function loadSettingsValues()
{
	var settings = {
		"AIDescriptions": loadAIDescriptions(),
		"AIDifficulties": loadAIDifficulties(),
		"Ceasefire": loadCeasefire(),
		"WonderDurations": loadWonderDuration(),
		"GameSpeeds": loadSettingValuesFile("game_speeds.json"),
		"MapTypes": loadMapTypes(),
		"MapSizes": loadSettingValuesFile("map_sizes.json"),
		"PlayerDefaults": loadPlayerDefaults(),
		"PopulationCapacities": loadPopulationCapacities(),
		"StartingResources": loadSettingValuesFile("starting_resources.json"),
		"VictoryConditions": loadVictoryConditions()
	};

	if (Object.keys(settings).some(key => settings[key] === undefined))
		return undefined;

	return settings;
}

/**
 * Returns an array of objects reflecting all possible values for a given setting.
 *
 * @param {string} filename
 * @see simulation/data/settings/
 * @returns {Array|undefined}
 */
function loadSettingValuesFile(filename)
{
	var json = Engine.ReadJSONFile(g_SettingsDirectory + filename);

	if (!json || !json.Data)
	{
		error("Could not load " + filename + "!");
		return undefined;
	}

	if (json.TranslatedKeys)
	{
		let keyContext = json.TranslatedKeys;

		if (json.TranslationContext)
		{
			keyContext = {};
			for (let key of json.TranslatedKeys)
				 keyContext[key] = json.TranslationContext;
		}

		translateObjectKeys(json.Data, keyContext);
	}

	return json.Data;
}

/**
 * Loads the descriptions as defined in simulation/ai/.../data.json and loaded by ICmpAIManager.cpp.
 *
 * @returns {Array}
 */
function loadAIDescriptions()
{
	var ais = Engine.GetAIs();
	translateObjectKeys(ais, ["name", "description"]);
	return ais.sort((a, b) => a.data.name.localeCompare(b.data.name));
}

/**
 * Hardcoded, as modding is not supported without major changes.
 * Notice the AI code parses the difficulty level by the index, not by name.
 *
 * @returns {Array}
 */
function loadAIDifficulties()
{
	return [
		{
			"Name": "sandbox",
			"Title": translateWithContext("aiDiff", "Sandbox")
		},
		{
			"Name": "very easy",
			"Title": translateWithContext("aiDiff", "Very Easy")
		},
		{
			"Name": "easy",
			"Title": translateWithContext("aiDiff", "Easy")
		},
		{
			"Name": "medium",
			"Title": translateWithContext("aiDiff", "Medium"),
			"Default": true
		},
		{
			"Name": "hard",
			"Title": translateWithContext("aiDiff", "Hard")
		},
		{
			"Name": "very hard",
			"Title": translateWithContext("aiDiff", "Very Hard")
		}
	];
}

/**
 * Loads available wonder-victory times
 */
function loadWonderDuration()
{
	var jsonFile = "wonder_times.json";
	var json = Engine.ReadJSONFile(g_SettingsDirectory + jsonFile);

	if (!json || json.Default === undefined || !json.Times || !Array.isArray(json.Times))
	{
		error("Could not load " + jsonFile);
		return undefined;
	}

	return json.Times.map(duration => ({
		"Duration": duration,
		"Default": duration == json.Default,
		"Title": sprintf(translatePluralWithContext("wonder victory", "%(min)s minute", "%(min)s minutes", duration), { "min": duration })
	}));
}

/**
 * Loads available ceasefire settings.
 *
 * @returns {Array|undefined}
 */
function loadCeasefire()
{
	var json = Engine.ReadJSONFile(g_SettingsDirectory + "ceasefire.json");

	if (!json || json.Default === undefined || !json.Times || !Array.isArray(json.Times))
	{
		error("Could not load ceasefire.json");
		return undefined;
	}

	return json.Times.map(timeout => ({
		"Duration": timeout,
		"Default": timeout == json.Default,
		"Title": timeout == 0 ? translateWithContext("ceasefire", "No ceasefire") :
			sprintf(translatePluralWithContext("ceasefire", "%(minutes)s minute", "%(minutes)s minutes", timeout), { "minutes": timeout })
	}));
}

/**
 * Hardcoded, as modding is not supported without major changes.
 *
 * @returns {Array}
 */
function loadMapTypes()
{
	return [
		{
			"Name": "skirmish",
			"Title": translateWithContext("map", "Skirmish"),
			"Default": true
		},
		{
			"Name": "random",
			"Title": translateWithContext("map", "Random")
		},
		{
			"Name": "scenario",
			"Title": translateWithContext("map", "Scenario")
		}
	];
}

/**
 * Loads available gametypes.
 *
 * @returns {Array|undefined}
 */
function loadVictoryConditions()
{
	const subdir = "victory_conditions/";

	const files = Engine.BuildDirEntList(g_SettingsDirectory + subdir, "*.json", false).map(
		file => file.substr(g_SettingsDirectory.length));

	var victoryConditions = files.map(file => {
		let vc = loadSettingValuesFile(file);
		if (vc)
			vc.Name = file.substr(subdir.length, file.length - (subdir + ".json").length);
		return vc;
	});

	if (victoryConditions.some(vc => vc == undefined))
		return undefined;

	// TODO: We might support enabling victory conditions separately sometime.
	// Until then, we supplement the endless gametype here.
	victoryConditions.push({
		"Name": "endless",
		"Title": translateWithContext("victory condition", "None"),
		"Description": translate("Endless game."),
		"Scripts": []
	});

	return victoryConditions;
}

/**
 * Loads the default player settings (like civs and colors).
 *
 * @returns {Array|undefined}
 */
function loadPlayerDefaults()
{
	var json = Engine.ReadJSONFile(g_SettingsDirectory + "player_defaults.json");
	if (!json || !json.PlayerData)
	{
		error("Could not load player_defaults.json");
		return undefined;
	}
	return json.PlayerData;
}

/**
 * Loads available population capacities.
 *
 * @returns {Array|undefined}
 */
function loadPopulationCapacities()
{
	var json = Engine.ReadJSONFile(g_SettingsDirectory + "population_capacities.json");

	if (!json || json.Default === undefined || !json.PopulationCapacities || !Array.isArray(json.PopulationCapacities))
	{
		error("Could not load population_capacities.json");
		return undefined;
	}

	return json.PopulationCapacities.map(population => ({
		"Population": population,
		"Default": population == json.Default,
		"Title": population < 10000 ? population : translate("Unlimited")
	}));
}

/**
 * Creates an object with all values of that property of the given setting and
 * finds the index of the default value.
 *
 * This allows easy copying of setting values to dropdown lists.
 *
 * @param {Array} settingValues
 * @returns {Object|undefined}
 */
function prepareForDropdown(settingValues)
{
	if (!settingValues)
		return undefined;

	var settings = { "Default": 0 };
	for (let index in settingValues)
	{
		for (let property in settingValues[index])
		{
			if (property == "Default")
				continue;

			if (!settings[property])
				settings[property] = [];

			// Switch property and index
			settings[property][index] = settingValues[index][property];
		}

		// Copy default value
		if (settingValues[index].Default)
			settings.Default = +index;
	}
	return settings;
}

/**
 * Returns title or placeholder.
 *
 * @param {string} aiName - for example "petra"
 */
function translateAIName(aiName)
{
	var description = g_Settings.AIDescriptions.find(ai => ai.id == aiName);
	return description ? translate(description.data.name) : translateWithContext("AI name", "Unknown");
}

/**
 * Returns title or placeholder.
 *
 * @param {Number} index - index of AIDifficulties
 */
function translateAIDifficulty(index)
{
	var difficulty = g_Settings.AIDifficulties[index];
	return difficulty ? difficulty.Title : translateWithContext("AI difficulty", "Unknown");
}

/**
 * Returns title or placeholder.
 *
 * @param {string} mapType - for example "skirmish"
 * @returns {string}
 */
function translateMapType(mapType)
{
	var type = g_Settings.MapTypes.find(t => t.Name == mapType);
	return type ? type.Title : translateWithContext("map type", "Unknown");
}

/**
 * Returns title or placeholder "Default".
 *
 * @param {Number} mapSize - tilecount
 * @returns {string}
 */
function translateMapSize(tiles)
{
	var mapSize = g_Settings.MapSizes.find(mapSize => mapSize.Tiles == +tiles);
	return mapSize ? mapSize.Name : translateWithContext("map size", "Default");
}

/**
 * Returns title or placeholder.
 *
 * @param {Number} population - for example 300
 * @returns {string}
 */
function translatePopulationCapacity(population)
{
	var popCap = g_Settings.PopulationCapacities.find(p => p.Population == population);
	return popCap ? popCap.Title : translateWithContext("population capacity", "Unknown");
}

/**
 * Returns title or placeholder.
 *
 * @param {string} gameType - for example "conquest"
 * @returns {string}
 */
function translateVictoryCondition(gameType)
{
	var vc = g_Settings.VictoryConditions.find(vc => vc.Name == gameType);
	return vc ? vc.Title : translateWithContext("victory condition", "Unknown");
}
