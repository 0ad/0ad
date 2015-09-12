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
const g_Settings = loadAvailableSettings();

/**
 * Loads and translates all values of all settings which
 * can be configured by dropdowns in the gamesetup.
 *
 * @returns {Object|undefined}
 */
function loadAvailableSettings()
{
	var settings = {};

	settings.Ceasefire = loadCeasefire();
	if (!settings.Ceasefire)
		return undefined;

	return settings;
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

			if (index == 0)
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
