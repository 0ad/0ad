/**
 * This class obtains the current simulation state to be saved along the savegame,
 * asks the user whether to overwrite a savegame and performs the saving.
 */
class SavegameWriter
{
	constructor(data)
	{
		this.savedGameData = data && data.savedGameData || {};
		let simulationState = Engine.GuiInterfaceCall("GetSimulationState");
		this.savedGameData.timeElapsed = simulationState.timeElapsed;
		this.savedGameData.states = simulationState.players.map(pState => pState.state);
	}

	getSavedGameData()
	{
		return this.savedGameData;
	}

	saveGame()
	{
		let gameSelection = Engine.GetGUIObjectByName("gameSelection");
		let gameLabel = gameSelection.list[gameSelection.selected];
		let gameID = gameSelection.list_data[gameSelection.selected];
		let desc = Engine.GetGUIObjectByName("saveGameDesc").caption;
		let name = gameID || "savegame";

		if (!gameID)
		{
			this.reallySaveGame(name, desc, true);
			return;
		}

		messageBox(
			500, 200,
			sprintf(translate("\"%(label)s\""), { "label": gameLabel }) + "\n" +
				translate("Saved game will be permanently overwritten, are you sure?"),
			translate("OVERWRITE SAVE"),
			[translate("No"), translate("Yes")],
			[null, () => { this.reallySaveGame(name, desc, false); }]);
	}

	reallySaveGame(name, desc, nameIsPrefix)
	{
		if (nameIsPrefix)
			Engine.SaveGamePrefix(name, desc, this.savedGameData);
		else
			Engine.SaveGame(name, desc, this.savedGameData);

		Engine.PopGuiPage();
	}
}
