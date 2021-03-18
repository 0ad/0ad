class MapBrowserPageControls
{
	constructor(mapBrowserPage, gridBrowser)
	{
		for (let name in this)
			this[name] = new this[name](mapBrowserPage, gridBrowser);

		this.mapBrowserPage = mapBrowserPage;
		this.gridBrowser = gridBrowser;

		this.setupButtons();
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
