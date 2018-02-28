var g_Descriptions;
var g_SavedGameData;

function selectDescription()
{
	let gameSelection = Engine.GetGUIObjectByName("gameSelection");
	let gameID = gameSelection.list_data[gameSelection.selected];
	Engine.GetGUIObjectByName("deleteGameButton").enabled = !!gameID;

	if (!gameID)
		return;

	Engine.GetGUIObjectByName("saveGameDesc").caption = g_Descriptions[gameID];
}

function init(data)
{
	g_SavedGameData = data && data.savedGameData || {};
	let simulationState = Engine.GuiInterfaceCall("GetSimulationState");
	g_SavedGameData.timeElapsed = simulationState.timeElapsed;
	g_SavedGameData.states = simulationState.players.map(pState => pState.state);

	let savedGames = Engine.GetSavedGames().sort(sortDecreasingDate);

	let gameSelection = Engine.GetGUIObjectByName("gameSelection");
	gameSelection.enabled = savedGames.length;

	if (!savedGames.length)
	{
		gameSelection.list = [translate("No saved games found")];
		gameSelection.selected = -1;
		return;
	}

	g_Descriptions = {};
	for (let game of savedGames)
		g_Descriptions[game.id] = game.metadata.description || "";

	let engineInfo = Engine.GetEngineInfo();
	gameSelection.list = savedGames.map(game => generateSavegameLabel(game.metadata, engineInfo));
	gameSelection.list_data = savedGames.map(game => game.id);
	gameSelection.selected = Math.min(gameSelection.selected, gameSelection.list.length - 1);

	Engine.GetGUIObjectByName("deleteGameButton").tooltip = deleteTooltip();
}

function saveGame()
{
	let gameSelection = Engine.GetGUIObjectByName("gameSelection");
	let gameLabel = gameSelection.list[gameSelection.selected];
	let gameID = gameSelection.list_data[gameSelection.selected];
	let desc = Engine.GetGUIObjectByName("saveGameDesc").caption;
	let name = gameID || "savegame";

	if (!gameID)
	{
		reallySaveGame(name, desc, true);
		return;
	}

	messageBox(
		500, 200,
		sprintf(translate("\"%(label)s\""), { "label": gameLabel }) + "\n" +
			translate("Saved game will be permanently overwritten, are you sure?"),
		translate("OVERWRITE SAVE"),
		[translate("No"), translate("Yes")],
		[null, function(){ reallySaveGame(name, desc, false); }]
	);
}

function reallySaveGame(name, desc, nameIsPrefix)
{
	if (nameIsPrefix)
		Engine.SaveGamePrefix(name, desc, g_SavedGameData);
	else
		Engine.SaveGame(name, desc, g_SavedGameData);

	closeSave();
}

function closeSave()
{
	Engine.PopGuiPageCB(0);
}

// HACK: Engine.SaveGame* expects this function to be defined on the current page.
// That's why we have to pass the data to this page even if we don't need it.
function getSavedGameData()
{
	return g_SavedGameData;
}
