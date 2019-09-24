/**
 * This class manages styling and performance of the savegame delete button.
 *
 * If a savegame was deleted, class instances that subscribed via
 * registerSavegameListChangeHandler will be informed that the list of
 * available savegames have changed.
 */
class SavegameDeleter
{
	constructor()
	{
		this.savegameListChangeHandlers = [];

		this.deleteGameButton = Engine.GetGUIObjectByName("deleteGameButton");
		this.deleteGameButton.tooltip = this.deleteButtonTooltip();
	}

	registerSavegameListChangeHandler(savegameListChangeHandler)
	{
		this.savegameListChangeHandlers.push(savegameListChangeHandler);
	}

	onSelectionChange(gameID, metadata, label)
	{
		this.deleteGameButton.enabled = !!metadata;
		this.deleteGameButton.onPress = () => {
			this.deleteGame(gameID, label);
		};
	}

	deleteButtonTooltip()
	{
		let tooltip = colorizeHotkey(
			translate("Delete the selected savegame using %(hotkey)s."),
			"session.savedgames.delete");

		if (tooltip)
			tooltip += colorizeHotkey(
				"\n" + translate("Hold %(hotkey)s to skip the confirmation dialog while deleting."),
				"session.savedgames.noconfirmation");

		return tooltip;
	}

	deleteGame(gameID, label)
	{
		if (Engine.HotkeyIsPressed("session.savedgames.noconfirmation"))
			this.reallyDeleteGame(gameID);
		else
			messageBox(
				500, 200,
				sprintf(translate("\"%(label)s\""), { "label": label }) + "\n" +
					translate("Saved game will be permanently deleted, are you sure?"),
				translate("DELETE"),
				[translate("No"), translate("Yes")],
				[null, () => { this.reallyDeleteGame(gameID); }]);
	}

	reallyDeleteGame(gameID)
	{
		if (!Engine.DeleteSavedGame(gameID))
			error("Could not delete saved game: " + gameID);

		for (let handler of this.savegameListChangeHandlers)
			handler.onSavegameListChange()
	}
}
