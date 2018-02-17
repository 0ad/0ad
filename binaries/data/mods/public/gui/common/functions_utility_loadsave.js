function sortDecreasingDate(a, b)
{
	return b.metadata.time - a.metadata.time;
}

function isCompatibleSavegame(metadata, engineInfo)
{
	return engineInfo && hasSameEngineVersion(metadata, engineInfo) & hasSameMods(metadata.mods, engineInfo.mods);
}

function generateSavegameDateString(metadata, engineInfo)
{
	return compatibilityColor(
		Engine.FormatMillisecondsIntoDateStringLocal(metadata.time * 1000, translate("yyyy-MM-dd HH:mm:ss")),
		isCompatibleSavegame(metadata, engineInfo));
}

function generateSavegameLabel(metadata, engineInfo)
{
	return sprintf(
		metadata.description ?
			translate("%(dateString)s %(map)s - %(description)s") :
			translate("%(dateString)s %(map)s"),
		{
			"dateString": generateSavegameDateString(metadata, engineInfo),
			"map": metadata.initAttributes.map,
			"description": metadata.description || ""
		}
	);
}

/**
 * Check the version compatibility between the saved game to be loaded and the engine
 */
function hasSameEngineVersion(metadata, engineInfo)
{
	return metadata.engine_version && metadata.engine_version == engineInfo.engine_version;
}

function deleteGame()
{
	let gameSelection = Engine.GetGUIObjectByName("gameSelection");
	let gameID = gameSelection.list_data[gameSelection.selected];

	if (!gameID)
		return;

	if (Engine.HotkeyIsPressed("session.savedgames.noconfirmation"))
		reallyDeleteGame(gameID);
	else
		messageBox(
			500, 200,
			sprintf(translate("\"%(label)s\""), {
				"label": gameSelection.list[gameSelection.selected]
			}) + "\n" + translate("Saved game will be permanently deleted, are you sure?"),
			translate("DELETE"),
			[translate("No"), translate("Yes")],
			[null, function(){ reallyDeleteGame(gameID); }]
		);
}

function reallyDeleteGame(gameID)
{
	if (!Engine.DeleteSavedGame(gameID))
		error("Could not delete saved game: " + gameID);

	// Run init again to refresh saved game list
	init();
}

function deleteTooltip()
{
	let tooltip = colorizeHotkey(
		translate("Delete the selected entry using %(hotkey)s."),
		"session.savedgames.delete");

	if (tooltip)
		tooltip += colorizeHotkey(
			"\n" + translate("Hold %(hotkey)s to delete without confirmation."),
			"session.savedgames.noconfirmation");

	return tooltip;
}
