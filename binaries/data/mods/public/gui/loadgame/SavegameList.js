/**
 * This class obtains the list of savegames from the engine,
 * builds the list dependent on selected filters and sorting order.
 *
 * If the selected savegame changes, class instances that subscribed via
 * registerSelectionChangeHandler will have their onSelectionChange function
 * called with the relevant savegame data.
 */
class SavegameList
{
	constructor()
	{
		this.savedGamesMetadata = [];
		this.selectionChangeHandlers = [];

		this.gameSelection = Engine.GetGUIObjectByName("gameSelection");
		this.gameSelectionFeedback = Engine.GetGUIObjectByName("gameSelectionFeedback");
		this.confirmButton = Engine.GetGUIObjectByName("confirmButton");
		this.compatibilityFilter = Engine.GetGUIObjectByName("compatibilityFilter");
		this.compatibilityFilter.onPress = () => { this.updateSavegameList(); };

		this.initSavegameList();
	}

	initSavegameList()
	{
		let engineInfo = Engine.GetEngineInfo();

		this.gameSelection.onSelectionColumnChange = () => { this.updateSavegameList(); };
		this.gameSelection.onMouseLeftDoubleClickItem = () => { this.confirmButton.onPress(); };
		this.gameSelection.onSelectionChange = () => {
			let gameId = this.gameSelection.list_data[this.gameSelection.selected];
			let metadata = this.savedGamesMetadata[this.gameSelection.selected];
			let label = this.generateSavegameLabel(metadata, engineInfo);
			for (let handler of this.selectionChangeHandlers)
				handler.onSelectionChange(gameId, metadata, label);
		};

		this.updateSavegameList();
	}

	registerSelectionChangeHandler(selectionChangeHandler)
	{
		this.selectionChangeHandlers.push(selectionChangeHandler);
	}

	onSavegameListChange()
	{
		this.updateSavegameList();

		// Allow subscribers (delete button) to update their press function in case
		// the list items changed but the selected index remained the same.
		this.gameSelection.onSelectionChange();
	}

	selectFirst()
	{
		if (this.gameSelection.list.length)
			this.gameSelection.selected = 0;
	}

	updateSavegameList()
	{
		let savedGames = Engine.GetSavedGames();

		// Get current game version and loaded mods
		let engineInfo = Engine.GetEngineInfo();

		if (this.compatibilityFilter.checked)
			savedGames = savedGames.filter(game => this.isCompatibleSavegame(game.metadata, engineInfo));

		this.gameSelection.enabled = !!savedGames.length;
		this.gameSelectionFeedback.hidden = !!savedGames.length;

		let selectedGameId = this.gameSelection.list_data[this.gameSelection.selected];

		// Save metadata for the detailed view
		this.savedGamesMetadata = savedGames.map(game => {
			game.metadata.id = game.id;
			return game.metadata;
		});

		let sortKey = this.gameSelection.selected_column;
		let sortOrder = this.gameSelection.selected_column_order;

		this.savedGamesMetadata = this.savedGamesMetadata.sort((a, b) => {
			let cmpA, cmpB;
			switch (sortKey)
			{
			case 'date':
				cmpA = +a.time;
				cmpB = +b.time;
				break;
			case 'mapName':
				cmpA = translate(a.initAttributes.settings.Name);
				cmpB = translate(b.initAttributes.settings.Name);
				break;
			case 'mapType':
				cmpA = translateMapType(a.initAttributes.mapType);
				cmpB = translateMapType(b.initAttributes.mapType);
				break;
			case 'description':
				cmpA = a.description;
				cmpB = b.description;
				break;
			}

			if (cmpA < cmpB)
				return -sortOrder;
			else if (cmpA > cmpB)
				return +sortOrder;

			return 0;
		});

		let list = this.savedGamesMetadata.map(metadata => {
			let isCompatible = this.isCompatibleSavegame(metadata, engineInfo);
			return {
				"date": this.generateSavegameDateString(metadata, engineInfo),
				"mapName": compatibilityColor(translate(metadata.initAttributes.settings.Name), isCompatible),
				"mapType": compatibilityColor(translateMapType(metadata.initAttributes.mapType), isCompatible),
				"description": compatibilityColor(metadata.description, isCompatible)
			};
		});

		if (list.length)
			list = prepareForDropdown(list);

		this.gameSelection.list_date = list.date || [];
		this.gameSelection.list_mapName = list.mapName || [];
		this.gameSelection.list_mapType = list.mapType || [];
		this.gameSelection.list_description = list.description || [];

		// Change these last, otherwise crash
		this.gameSelection.list = this.savedGamesMetadata.map(metadata => 0);
		this.gameSelection.list_data = this.savedGamesMetadata.map(metadata => metadata.id);

		 // Restore selection if the selected savegame still exists.
		// If the last savegame was deleted, or if it was hidden by the compatibility filter, select the new last item.
		let selectedGameIndex = this.savedGamesMetadata.findIndex(metadata => metadata.id == selectedGameId);
		if (selectedGameIndex != -1)
			this.gameSelection.selected = selectedGameIndex;
		else if (this.gameSelection.selected >= this.savedGamesMetadata.length)
			this.gameSelection.selected = this.savedGamesMetadata.length - 1;
	}

	isCompatibleSavegame(metadata, engineInfo)
	{
		return engineInfo &&
			metadata.engine_version &&
			metadata.engine_version == engineInfo.engine_version &&
			hasSameMods(metadata.mods, engineInfo.mods);
	}

	generateSavegameDateString(metadata, engineInfo)
	{
		return compatibilityColor(
			Engine.FormatMillisecondsIntoDateStringLocal(metadata.time * 1000, translate("yyyy-MM-dd HH:mm:ss")),
			this.isCompatibleSavegame(metadata, engineInfo));
	}

	generateSavegameLabel(metadata, engineInfo)
	{
		if (!metadata)
			return undefined;

		return sprintf(
			metadata.description ?
				translate("%(dateString)s %(map)s - %(description)s") :
				translate("%(dateString)s %(map)s"),
			{
				"dateString": this.generateSavegameDateString(metadata, engineInfo),
				"map": metadata.initAttributes.map,
				"description": metadata.description || ""
			});
	}
}
