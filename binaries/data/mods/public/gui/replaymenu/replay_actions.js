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
		"isNetworked" : false,
		"playerAssignments": {
			"local":{
				"name": translate("You"),
				"player": -1
			}
		},
		"savedGUIData": "",
		"isReplay" : true
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
		let gameMods = replay.attribs.mods ? replay.attribs.mods : [];
		errMsg = translate("You don't have the same mods active as the replay does.") + "\n";
		errMsg += sprintf(translate("Required: %(mods)s"), { "mods": gameMods.join(", ") }) + "\n";
		errMsg += sprintf(translate("Active: %(mods)s"), { "mods": g_EngineInfo.mods.join(", ") });
	}
	else
		errMsg = translate("This replay is not compatible with your version of the game!");

	messageBox(500, 200, errMsg, translate("Incompatible replay"), 0, [translate("Ok")], [null]);
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
		messageBox(500, 200, translate("No summary data available."), translate("Error"), 0, [translate("Ok")], [null]);
		return;
	}

	// Open summary screen
	summary.isReplay = true;
	summary.gameResult = translate("Scores at the end of the game.");
	Engine.SwitchGuiPage("page_summary.xml", summary);
}

/**
 * Callback.
 */
function deleteReplayButtonPressed()
{
	if (!Engine.GetGUIObjectByName("deleteReplayButton").enabled)
		return;

	if (Engine.HotkeyIsPressed("session.savedgames.noConfirmation"))
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

	// Show confirmation message
	var btCaptions = [translate("Yes"), translate("No")];
	var btCode = [function() { reallyDeleteReplay(replay.directory); }, null];

	var title = translate("Delete replay");
	var question = translate("Are you sure to delete this replay permanently?") + "\n" + escapeText(replay.file);

	messageBox(500, 200, question, title, 0, btCaptions, btCode);
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
	if (!Engine.DeleteReplay(replayDirectory))
		error(sprintf("Could not delete replay '%(id)s'", { "id": replayDirectory }));

	// Refresh replay list
	init();
}
