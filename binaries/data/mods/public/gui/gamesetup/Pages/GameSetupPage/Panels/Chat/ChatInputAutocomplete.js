class ChatInputAutocomplete
{
	constructor(gameSettingControlManager, gameSettingsController, playerAssignmentsController)
	{
		this.gameSettingControlManager = gameSettingControlManager;
		this.entries = undefined;

		playerAssignmentsController.registerPlayerAssignmentsChangeHandler(this.onAutocompleteChange.bind(this));
		gameSettingsController.registerSettingsChangeHandler(this.onAutocompleteChange.bind(this));
	}

	onAutocompleteChange()
	{
		this.entries = undefined;
	}

	// Collects all strings that can be autocompleted and
	// sorts them by priority (so that playernames are always autocompleted first).
	getAutocompleteEntries()
	{
		if (this.entries)
			return this.entries;

		// Maps from priority to autocompletable strings
		let entries = { "0": [] };

		this.gameSettingControlManager.addAutocompleteEntries(entries);

		let allEntries = Object.keys(entries).sort((a, b) => +b - +a).reduce(
			(all, priority) => all.concat(entries[priority]),
			[]);

		this.entries = Array.from(new Set(allEntries));

		return this.entries;
	}
}
