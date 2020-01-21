/**
 * This class owns all handlers of the gamesetup page, excluding controllers that apply to all subpages and handlers for specific subpages.
 */
SetupWindowPages.GameSetupPage = class
{
	constructor(setupWindow)
	{
		Engine.ProfileStart("GameSetupPage");

		// This class instance owns all gamesetting GUI controls such as dropdowns and checkboxes visible in this page.
		this.gameSettingControlManager = new GameSettingControlManager(setupWindow);

		// These classes manage GUI buttons.
		{
			let startGameButton = new StartGameButton(setupWindow);
			let readyButton = new ReadyButton(setupWindow);
			this.panelButtons = {
				"cancelButton": new CancelButton(setupWindow, startGameButton, readyButton),
				"civInfoButton": new CivInfoButton(),
				"lobbyButton": new LobbyButton(),
				"readyButton": readyButton,
				"startGameButton": startGameButton
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

		Engine.ProfileStop();
	}
}
