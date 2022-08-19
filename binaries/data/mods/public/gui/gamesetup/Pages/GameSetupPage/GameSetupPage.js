/**
 * This class owns all handlers of the game setup page, excluding controllers that apply to all subpages and handlers for specific subpages.
 */
SetupWindowPages.GameSetupPage = class
{
	constructor(setupWindow)
	{
		Engine.ProfileStart("GameSetupPage");
		
		// No save data has been loaded for now
		g_isSaveLoaded = false;
		g_savedGameId = undefined;

		// This class instance owns all game setting GUI controls such as dropdowns and checkboxes visible in this page.
		this.gameSettingControlManager = new GameSettingControlManager(setupWindow);

		// These classes manage GUI buttons.
		{
			let loadGameButton = new LoadGameButton(setupWindow);
			let startGameButton = new StartGameButton(setupWindow, loadGameButton);
			let readyButton = new ReadyButton(setupWindow, loadGameButton);
			let cancelButton = new CancelButton(setupWindow, startGameButton, readyButton)
			this.panelButtons = {
				"civInfoButton": new CivInfoButton(),
				"lobbyButton": new LobbyButton(),
				"cancelButton": cancelButton,
				"readyButton": readyButton,
				"startGameButton": startGameButton,
				"loadGameButton": loadGameButton
			};
		}

		// These classes manage GUI Objects.
		{
			let gameSettingTabs = new GameSettingTabs(setupWindow, this.panelButtons.lobbyButton);
			let gameSettingsPanel = new GameSettingsPanel(
				setupWindow, gameSettingTabs, this.gameSettingControlManager);

			this.panels = {
				"chatPanel": new ChatPanel(setupWindow, this.gameSettingControlManager, gameSettingsPanel),
				"gameSettingWarning": new GameSettingWarning(setupWindow, this.panelButtons.cancelButton),
				"gameDescription": new GameDescription(setupWindow, gameSettingTabs),
				"gameSettingsPanel": gameSettingsPanel,
				"gameSettingsTabs": gameSettingTabs,
				"mapPreview": new MapPreview(setupWindow),
				"resetCivsButton": new ResetCivsButton(setupWindow),
				"resetTeamsButton": new ResetTeamsButton(setupWindow),
				"soundNotification": new SoundNotification(setupWindow),
				"tipsPanel": new TipsPanel(gameSettingsPanel),
				"tooltip": new Tooltip(this.panelButtons.cancelButton)
			};
		}

		setupWindow.controls.gameSettingsController.registerLoadingChangeHandler((loading) => this.onLoadingChange(loading));

		Engine.ProfileStop();
	}

	onLoadingChange(loading)
	{
		Engine.GetGUIObjectByName("gameSetupPage").hidden = loading;
	}
};
