class GameSettingTabs
{
	constructor(setupWindow, lobbyButton)
	{
		this.lobbyButton = lobbyButton;

		this.tabSelectHandlers = new Set();
		this.tabsResizeHandlers = new Set();

		this.settingsTabButtonsFrame = Engine.GetGUIObjectByName("settingTabButtonsFrame");

		for (let tab in g_GameSettingsLayout)
			g_GameSettingsLayout[tab].tooltip =
				sprintf(this.ToggleTooltip, { "name": g_GameSettingsLayout[tab].label }) +
				colorizeHotkey("\n" + this.HotkeyDownTooltip, this.ConfigNameHotkeyDown) +
				colorizeHotkey("\n" + this.HotkeyUpTooltip, this.ConfigNameHotkeyUp);

		setupWindow.registerLoadHandler(this.onLoad.bind(this));
		Engine.SetGlobalHotkey("cancel", "Press", selectPanel);
	}

	registerTabsResizeHandler(handler)
	{
		this.tabsResizeHandlers.add(handler);
	}

	registerTabSelectHandler(handler)
	{
		this.tabSelectHandlers.add(handler);
	}

	onLoad()
	{
		placeTabButtons(
			g_GameSettingsLayout,
			this.TabButtonHeight,
			this.TabButtonMargin,
			this.onTabPress.bind(this),
			this.onTabSelect.bind(this));

		this.resize();

		if (!g_IsController)
			selectPanel();
	}

	resize()
	{
		let size = this.settingsTabButtonsFrame.size;
		size.bottom = size.top + g_GameSettingsLayout.length * (this.TabButtonHeight + this.TabButtonMargin);

		if (!this.lobbyButton.lobbyButton.hidden)
		{
			let lobbyButtonSize = this.lobbyButton.lobbyButton.parent.size;
			size.right -= lobbyButtonSize.right - lobbyButtonSize.left + this.LobbyButtonMargin;
		}
		this.settingsTabButtonsFrame.size = size;

		for (let handler of this.tabsResizeHandlers)
			handler(this.settingsTabButtonsFrame);
	}

	onTabPress(category)
	{
		selectPanel(category == g_TabCategorySelected ? undefined : category);
	}

	onTabSelect()
	{
		for (let handler of this.tabSelectHandlers)
			handler();
	}
}

GameSettingTabs.prototype.ToggleTooltip =
	translate("Toggle the %(name)s settings tab.");

GameSettingTabs.prototype.HotkeyUpTooltip =
	translate("Use %(hotkey)s to move a settings tab up.");

GameSettingTabs.prototype.HotkeyDownTooltip =
	translate("Use %(hotkey)s to move a settings tab down.");

GameSettingTabs.prototype.ConfigNameHotkeyUp =
	"tab.next";

GameSettingTabs.prototype.ConfigNameHotkeyDown =
	"tab.prev";

GameSettingTabs.prototype.TabButtonHeight = 30;

GameSettingTabs.prototype.TabButtonMargin = 4;

/**
 * Horizontal space between tab buttons and lobby button.
 */
GameSettingTabs.prototype.LobbyButtonMargin = 8;
