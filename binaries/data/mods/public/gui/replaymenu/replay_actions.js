/**
 * Creates the data for restoring selection, order and filters when returning to the replay menu.
 */
function createReplaySelectionData(selectedDirectory)
{
	let replaySelection = Engine.GetGUIObjectByName("replaySelection");
	let dateTimeFilter = Engine.GetGUIObjectByName("dateTimeFilter");
	let playersFilter = Engine.GetGUIObjectByName("playersFilter");
	let mapNameFilter = Engine.GetGUIObjectByName("mapNameFilter");
	let mapSizeFilter = Engine.GetGUIObjectByName("mapSizeFilter");
	let populationFilter = Engine.GetGUIObjectByName("populationFilter");
	let durationFilter = Engine.GetGUIObjectByName("durationFilter");
	let compatibilityFilter = Engine.GetGUIObjectByName("compatibilityFilter");
	let singleplayerFilter = Engine.GetGUIObjectByName("singleplayerFilter");
	let victoryConFilter = Engine.GetGUIObjectByName("victoryConditionFilter");
	let ratedGamesFilter = Engine.GetGUIObjectByName("ratedGamesFilter");

	return {
		"directory": selectedDirectory,
		"column": replaySelection.selected_column,
		"columnOrder": replaySelection.selected_column_order,
		"filters": {
			"date": dateTimeFilter.list_data[dateTimeFilter.selected],
			"playernames": playersFilter.caption,
			"mapName": mapNameFilter.list_data[mapNameFilter.selected],
			"mapSize": mapSizeFilter.list_data[mapSizeFilter.selected],
			"popCap": populationFilter.list_data[populationFilter.selected],
			"duration": durationFilter.list_data[durationFilter.selected],
			"compatibility": compatibilityFilter.checked,
			"singleplayer": singleplayerFilter.list_data[singleplayerFilter.selected],
			"victoryCondition": victoryConFilter.list_data[victoryConFilter.selected],
			"ratedGames": ratedGamesFilter.selected
		}
	};
}

/**
 * Starts the selected visual replay, or shows an error message in case of incompatibility.
 */
function startReplay()
{
	var selected = Engine.GetGUIObjectByName("replaySelection").selected;
	if (selected == -1)
		return;

	var replay = g_ReplaysFiltered[selected];
	if (isReplayCompatible(replay))
		reallyStartVisualReplay(replay.directory);
	else
		displayReplayCompatibilityError(replay);
}

/**
 * Attempts the visual replay, regardless of the compatibility.
 *
 * @param replayDirectory {string}
 */
function reallyStartVisualReplay(replayDirectory)
{
	// TODO: enhancement: restore filter settings and selected replay when returning from the summary screen.
	Engine.StartVisualReplay(replayDirectory);
	Engine.SwitchGuiPage("page_loading.xml", {
		"attribs": Engine.GetReplayAttributes(replayDirectory),
		"isNetworked": false,
		"playerAssignments": {
			"local":{
				"name": singleplayerName(),
				"player": -1
			}
		},
		"savedGUIData": "",
		"isReplay": true,
		"replaySelectionData": createReplaySelectionData(replayDirectory)
	});
}

/**
 * Shows an error message stating why the replay is not compatible.
 *
 * @param replay {Object}
 */
function displayReplayCompatibilityError(replay)
{
	var errMsg;
	if (replayHasSameEngineVersion(replay))
	{
		let gameMods = replay.attribs.mods || [];
		errMsg = translate("You don't have the same mods active as the replay does.") + "\n";
		errMsg += sprintf(translate("Required: %(mods)s"), { "mods": gameMods.join(translate(", ")) }) + "\n";
		errMsg += sprintf(translate("Active: %(mods)s"), { "mods": g_EngineInfo.mods.join(translate(", ")) });
	}
	else
	{
		errMsg = translate("This replay is not compatible with your version of the game!") + "\n";
		errMsg += sprintf(translate("Your version: %(version)s"), { "version": g_EngineInfo.engine_version }) + "\n";
		errMsg += sprintf(translate("Required version: %(version)s"), { "version": replay.attribs.engine_version });
	}

	messageBox(500, 200, errMsg, translate("Incompatible replay"));
}

/**
 * Opens the summary screen of the given replay, if its data was found in that directory.
 */
function showReplaySummary()
{
	var selected = Engine.GetGUIObjectByName("replaySelection").selected;
	if (selected == -1)
		return;

	// Load summary screen data from the selected replay directory
	var summary = Engine.GetReplayMetadata(g_ReplaysFiltered[selected].directory);

	if (!summary)
	{
		messageBox(500, 200, translate("No summary data available."), translate("Error"));
		return;
	}

	// Open summary screen
	summary.isReplay = true;
	summary.gameResult = translate("Scores at the end of the game.");
	summary.replayDirectory = g_ReplaysFiltered[selected].directory;
	summary.replaySelectionData = createReplaySelectionData(g_ReplaysFiltered[selected].directory);
	Engine.SwitchGuiPage("page_summary.xml", summary);
}

/**
 * Callback.
 */
function deleteReplayButtonPressed()
{
	if (!Engine.GetGUIObjectByName("deleteReplayButton").enabled)
		return;

	if (Engine.HotkeyIsPressed("session.savedgames.noconfirmation"))
		deleteReplayWithoutConfirmation();
	else
		deleteReplay();
}
/**
 * Shows a confirmation dialog and deletes the selected replay from the disk in case.
 */
function deleteReplay()
{
	// Get selected replay
	var selected = Engine.GetGUIObjectByName("replaySelection").selected;
	if (selected == -1)
		return;

	var replay = g_ReplaysFiltered[selected];

	messageBox(
		500, 200,
		translate("Are you sure you want to delete this replay permanently?") + "\n" + escapeText(replay.file),
		translate("Delete replay"),
		[translate("No"), translate("Yes")],
		[null, function() { reallyDeleteReplay(replay.directory); }]
	);
}

/**
 * Attempts to delete the selected replay from the disk.
 */
function deleteReplayWithoutConfirmation()
{
	var selected = Engine.GetGUIObjectByName("replaySelection").selected;
	if (selected > -1)
		reallyDeleteReplay(g_ReplaysFiltered[selected].directory);
}

/**
 * Attempts to delete the given replay directory from the disk.
 *
 * @param replayDirectory {string}
 */
function reallyDeleteReplay(replayDirectory)
{
	var replaySelection = Engine.GetGUIObjectByName("replaySelection");
	var selectedIndex = replaySelection.selected;

	if (!Engine.DeleteReplay(replayDirectory))
		error(sprintf("Could not delete replay '%(id)s'", { "id": replayDirectory }));

	// Refresh replay list
	init();

	replaySelection.selected = Math.min(selectedIndex, g_ReplaysFiltered.length - 1);
}
