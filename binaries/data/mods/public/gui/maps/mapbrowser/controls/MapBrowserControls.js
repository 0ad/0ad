class MapBrowserPageControls
{
	constructor(mapBrowserPage, gridBrowser, setupWindow = undefined)
	{
		for (let name in this)
			this[name] = new this[name](mapBrowserPage, gridBrowser);

		this.mapBrowserPage = mapBrowserPage;
		this.gridBrowser = gridBrowser;
		this.setupWindow = setupWindow;
		
		this.originalCloseSize = undefined;

		this.setupButtons();

		if (this.onSettingsChanged)
		{
			this.setupWindow.controls.gameSettingsController.registerSettingsChangeHandler(
				this.onSettingsChanged.bind(this));
		}
	}

	onSettingsChanged()
	{
		// If the player isn't the controller or if save data has been loaded, 
		// hide the pickRandom and select buttons, update the size of the close
		// button
		let hidden = !g_IsController || g_isSaveLoaded;
 
		this.pickRandom.hidden = hidden;
		this.select.hidden = hidden;

		if (hidden)
			this.close.size = this.select.size;
		else
			this.close.size = this.originalCloseSize;
	}

	setupButtons()
	{
		this.pickRandom = Engine.GetGUIObjectByName("mapBrowserPagePickRandom");
		if (!g_IsController)
			this.pickRandom.hidden = true;
		this.pickRandom.onPress = () => {
			let index = randIntInclusive(0, this.gridBrowser.itemCount - 1);
			this.gridBrowser.setSelectedIndex(index);
			this.gridBrowser.goToPageOfSelected();
		};

		this.select = Engine.GetGUIObjectByName("mapBrowserPageSelect");
		this.select.onPress = () => this.onSelect();

		this.close = Engine.GetGUIObjectByName("mapBrowserPageClose");
		this.originalCloseSize = this.close.size;

		if (g_SetupWindow)
			this.close.tooltip = colorizeHotkey(
				translate("%(hotkey)s: Close map browser and discard the selection."), "cancel");
		else
		{
			this.close.caption = translate("Close");
			this.close.tooltip = colorizeHotkey(
				translate("%(hotkey)s: Close map browser."), "cancel");
		}

		this.close.onPress = () => this.mapBrowserPage.closePage();

		this.select.hidden = !g_IsController;
		if (!g_IsController)
			this.close.size = this.select.size;

		this.gridBrowser.registerSelectionChangeHandler(() => this.onSelectionChange());
	}

	onSelectionChange()
	{
		this.select.enabled = this.gridBrowser.selected != -1;
	}

	onSelect()
	{
		this.mapBrowserPage.submitMapSelection();
	}
}
