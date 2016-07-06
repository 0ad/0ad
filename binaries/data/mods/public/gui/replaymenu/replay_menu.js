/**
 * Used for checking replay compatibility.
 */
const g_EngineInfo = Engine.GetEngineInfo();

/**
 * Needed for formatPlayerInfo to show the player civs in the details.
 */
const g_CivData = loadCivData();

/**
 * Used for creating the mapsize filter.
 */
const g_MapSizes = prepareForDropdown(g_Settings && g_Settings.MapSizes);

/**
 * All replays found in the directory.
 */
var g_Replays = [];

/**
 * List of replays after applying the display filter.
 */
var g_ReplaysFiltered = [];

/**
 * Array of unique usernames of all replays. Used for autocompleting usernames.
 */
var g_Playernames = [];

/**
 * Sorted list of unique maptitles. Used by mapfilter.
 */
var g_MapNames = [];

/**
 * Sorted list of the victory conditions occuring in the replays
 */
var g_VictoryConditions = [];

/**
 * Directory name of the currently selected replay. Used to restore the selection after changing filters.
 */
var g_SelectedReplayDirectory = "";

/**
 * Initializes globals, loads replays and displays the list.
 */
function init(data)
{
	if (!g_Settings)
	{
		Engine.SwitchGuiPage("page_pregame.xml");
		return;
	}

	loadReplays(data && data.replaySelectionData);

	if (!g_Replays)
	{
		Engine.SwitchGuiPage("page_pregame.xml");
		return;
	}

	displayReplayList();
}

/**
 * Store the list of replays loaded in C++ in g_Replays.
 * Check timestamp and compatibility and extract g_Playernames, g_MapNames, g_VictoryConditions.
 * Restore selected filters and item.
 */
function loadReplays(replaySelectionData)
{
	g_Replays = Engine.GetReplays();

	if (!g_Replays)
		return;

	g_Playernames = [];
	for (let replay of g_Replays)
	{
		let nonAIPlayers = 0;
		// Use time saved in file, otherwise file mod date
		replay.timestamp = replay.attribs.timestamp ? +replay.attribs.timestamp : +replay.filemod_timestamp-replay.duration;

		// Check replay for compatibility
		replay.isCompatible = isReplayCompatible(replay);

		sanitizeGameAttributes(replay.attribs);

		// Extract map names
		if (g_MapNames.indexOf(replay.attribs.settings.Name) == -1 && replay.attribs.settings.Name != "")
			g_MapNames.push(replay.attribs.settings.Name);

		// Extract victory conditions
		if (replay.attribs.settings.GameType && g_VictoryConditions.indexOf(replay.attribs.settings.GameType) == -1)
			g_VictoryConditions.push(replay.attribs.settings.GameType);

		// Extract playernames
		for (let playerData of replay.attribs.settings.PlayerData)
		{
			if (!playerData || playerData.AI)
				continue;

			// Remove rating from nick
			let playername = playerData.Name;
			let ratingStart = playername.indexOf(" (");
			if (ratingStart != -1)
				playername = playername.substr(0, ratingStart);

			if (g_Playernames.indexOf(playername) == -1)
				g_Playernames.push(playername);

			++nonAIPlayers;
		}

		replay.isMultiplayer = nonAIPlayers > 1;

		replay.isRated = nonAIPlayers == 2 &&
			replay.attribs.settings.PlayerData.length == 2 &&
			replay.attribs.settings.RatingEnabled;
	}

	g_MapNames.sort();
	g_VictoryConditions.sort();

	// Reload filters (since they depend on g_Replays and its derivatives)
	initFilters(replaySelectionData && replaySelectionData.filters);

	// Restore user selection
	if (replaySelectionData)
	{
		if (replaySelectionData.directory)
			g_SelectedReplayDirectory = replaySelectionData.directory;

		let replaySelection = Engine.GetGUIObjectByName("replaySelection");
		if (replaySelectionData.column)
			replaySelection.selected_column = replaySelectionData.column;
		if (replaySelectionData.columnOrder)
			replaySelection.selected_column_order = replaySelectionData.columnOrder;
	}
}

/**
 * We may encounter malformed replays.
 */
function sanitizeGameAttributes(attribs)
{
	if (!attribs.settings)
		attribs.settings = {};

	if (!attribs.settings.Size)
		attribs.settings.Size = -1;

	if (!attribs.settings.Name)
		attribs.settings.Name = "";

	if (!attribs.settings.PlayerData)
		attribs.settings.PlayerData = [];

	if (!attribs.settings.PopulationCap)
		attribs.settings.PopulationCap = 300;

	if (!attribs.settings.mapType)
		attribs.settings.mapType = "skirmish";

	if (!attribs.settings.GameType)
		attribs.settings.GameType = "conquest";

	// Remove gaia
	if (attribs.settings.PlayerData.length && attribs.settings.PlayerData[0] == null)
		attribs.settings.PlayerData.shift();

	attribs.settings.PlayerData.forEach((pData, index) => {
		if (!pData.Name)
			pData.Name = "";
	});
}

/**
 * Filter g_Replays, fill the GUI list with that data and show the description of the current replay.
 */
function displayReplayList()
{
	// Remember previously selected replay
	var replaySelection = Engine.GetGUIObjectByName("replaySelection");
	if (replaySelection.selected != -1)
		g_SelectedReplayDirectory = g_ReplaysFiltered[replaySelection.selected].directory;

	filterReplays();

	var list = g_ReplaysFiltered.map(replay => {
		let works = replay.isCompatible;
		return {
			"directories": replay.directory,
			"months": greyout(getReplayDateTime(replay), works),
			"popCaps": greyout(translatePopulationCapacity(replay.attribs.settings.PopulationCap), works),
			"mapNames": greyout(getReplayMapName(replay), works),
			"mapSizes": greyout(translateMapSize(replay.attribs.settings.Size), works),
			"durations": greyout(getReplayDuration(replay), works),
			"playerNames": greyout(getReplayPlayernames(replay), works)
		};
	});

	if (list.length)
		list = prepareForDropdown(list);

	// Push to GUI
	replaySelection.selected = -1;
	replaySelection.list_name = list.months || [];
	replaySelection.list_players = list.playerNames || [];
	replaySelection.list_mapName = list.mapNames || [];
	replaySelection.list_mapSize = list.mapSizes || [];
	replaySelection.list_popCapacity = list.popCaps || [];
	replaySelection.list_duration = list.durations || [];

	// Change these last, otherwise crash
	replaySelection.list = list.directories || [];
	replaySelection.list_data = list.directories || [];

	replaySelection.selected = replaySelection.list.findIndex(directory => directory == g_SelectedReplayDirectory);

	displayReplayDetails();
}

/**
 * Shows preview image, description and player text in the right panel.
 */
function displayReplayDetails()
{
	let selected = Engine.GetGUIObjectByName("replaySelection").selected;
	let replaySelected = selected > -1;

	Engine.GetGUIObjectByName("replayInfo").hidden = !replaySelected;
	Engine.GetGUIObjectByName("replayInfoEmpty").hidden = replaySelected;
	Engine.GetGUIObjectByName("startReplayButton").enabled = replaySelected;
	Engine.GetGUIObjectByName("deleteReplayButton").enabled = replaySelected;
	Engine.GetGUIObjectByName("summaryButton").hidden = true;

	if (!replaySelected)
		return;

	let replay = g_ReplaysFiltered[selected];

	Engine.GetGUIObjectByName("sgMapName").caption = translate(replay.attribs.settings.Name);
	Engine.GetGUIObjectByName("sgMapSize").caption = translateMapSize(replay.attribs.settings.Size);
	Engine.GetGUIObjectByName("sgMapType").caption = translateMapType(replay.attribs.settings.mapType);
	Engine.GetGUIObjectByName("sgVictory").caption = translateVictoryCondition(replay.attribs.settings.GameType);
	Engine.GetGUIObjectByName("sgNbPlayers").caption = replay.attribs.settings.PlayerData.length;

	let metadata = Engine.GetReplayMetadata(replay.directory);
	Engine.GetGUIObjectByName("sgPlayersNames").caption =
		formatPlayerInfo(
			replay.attribs.settings.PlayerData,
			Engine.GetGUIObjectByName("showSpoiler").checked &&
				metadata &&
				metadata.playerStates &&
				metadata.playerStates.map(pState => pState.state)
		);

	let mapData = getMapDescriptionAndPreview(replay.attribs.settings.mapType, replay.attribs.map);
	Engine.GetGUIObjectByName("sgMapDescription").caption = mapData.description;

	Engine.GetGUIObjectByName("summaryButton").hidden = !Engine.HasReplayMetadata(replay.directory);

	setMapPreviewImage("sgMapPreview", mapData.preview);
}

/**
 * Adds grey font if replay is not compatible.
 */
function greyout(text, isCompatible)
{
	return isCompatible ? text : '[color="96 96 96"]' + text + '[/color]';
}

/**
 * Returns a human-readable version of the replay date.
 */
function getReplayDateTime(replay)
{
	return Engine.FormatMillisecondsIntoDateString(replay.timestamp * 1000, translate("yyyy-MM-dd HH:mm"));
}

/**
 * Returns a human-readable list of the playernames of that replay.
 *
 * @returns {string}
 */
function getReplayPlayernames(replay)
{
	return replay.attribs.settings.PlayerData.map(pData => pData.Name).join(", ");
}

/**
 * Returns the name of the map of the given replay.
 *
 * @returns {string}
 */
function getReplayMapName(replay)
{
	return translate(replay.attribs.settings.Name);
}

/**
 * Returns the month of the given replay in the format "yyyy-MM".
 *
 * @returns {string}
 */
function getReplayMonth(replay)
{
	return Engine.FormatMillisecondsIntoDateString(replay.timestamp * 1000, translate("yyyy-MM"));
}

/**
 * Returns a human-readable version of the time when the replay started.
 *
 * @returns {string}
 */
function getReplayDuration(replay)
{
	return timeToString(replay.duration * 1000);
}

/**
 * True if we can start the given replay with the currently loaded mods.
 */
function isReplayCompatible(replay)
{
	return replayHasSameEngineVersion(replay) && hasSameMods(replay.attribs, g_EngineInfo);
}

/**
 * True if we can start the given replay with the currently loaded mods.
 */
function replayHasSameEngineVersion(replay)
{
	return replay.attribs.engine_version && replay.attribs.engine_version == g_EngineInfo.engine_version;
}
