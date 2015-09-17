/**
 * The maximum number of players that the engine supports.
 * TODO: Maybe we can support more than 8 players sometime.
 */
const g_MaxPlayers = 8;

/**
 * The maximum number of teams allowed.
 */
const g_MaxTeams = 4;

// The following settings will be loaded here:
// AIDifficulties, Ceasefire, GameSpeeds, GameTypes, MapTypes,
// MapSizes, PlayerDefaults, PopulationCapacity, StartingResources

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
		"Ceasefire": loadCeasefire(),
		"GameSpeeds": loadSettingValuesFile("game_speeds.json"),
		"MapTypes": loadMapTypes(),
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
		translateObjectKeys(json, json.TranslatedKeys);

	return json.Data;
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
			"Title": translate("Scenario") // TODO: update the translation (but not shortly before a release)
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
	const subdir = "victory_conditions/"

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
		"Title": translate("None"),
		"Description": translate("Endless Game"),
		"Scripts": []
	});

	return victoryConditions;
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
 * @param settingValues {Array}
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
