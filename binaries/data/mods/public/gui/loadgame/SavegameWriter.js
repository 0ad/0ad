/**
 * This class obtains the current simulation state to be saved along the savegame,
 * asks the user whether to overwrite a savegame and performs the saving.
 */
class SavegameWriter
{
	constructor(savedGameData)
	{
		this.savedGameData = savedGameData;

		let saveNew = () => {
			this.saveGame();
		};

		this.confirmButton = Engine.GetGUIObjectByName("confirmButton");
		this.confirmButton.caption = translate("Save");
		this.confirmButton.onPress = saveNew;

		this.saveGameDesc = Engine.GetGUIObjectByName("saveGameDesc");
		this.saveGameDesc.hidden = false;
		this.saveGameDesc.onPress = saveNew;
	}

	onSelectionChange(gameID, metadata, label)
	{
		this.confirmButton.enabled = !!metadata || Engine.IsGameStarted();
		this.confirmButton.onPress = () => {
			this.saveGame(gameID, label);
		};
	}

	saveGame(gameID, label)
	{
		let desc = this.saveGameDesc.caption;
		let name = gameID || "savegame";

		if (!gameID)
		{
			this.reallySaveGame(name, desc, true);
			return;
		}

		messageBox(
			500, 200,
			sprintf(translate("\"%(label)s\""), { "label": label }) + "\n" +
				translate("Saved game will be permanently overwritten, are you sure?"),
			translate("OVERWRITE SAVE"),
			[translate("No"), translate("Yes")],
			[null, () => { this.reallySaveGame(name, desc, false); }]);
	}

	reallySaveGame(name, desc, nameIsPrefix)
	{
		let simulationState = Engine.GuiInterfaceCall("GetSimulationState");
		this.savedGameData.timeElapsed = simulationState.timeElapsed;
		this.savedGameData.states = simulationState.players.map(pState => pState.state);

		if (nameIsPrefix)
			Engine.SaveGamePrefix(name, desc, this.savedGameData);
		else
			Engine.SaveGame(name, desc, this.savedGameData);

		Engine.PopGuiPage();
	}
}
