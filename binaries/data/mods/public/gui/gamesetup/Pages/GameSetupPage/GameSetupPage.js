/**
 * This class owns all handlers of the gamesetup page, excluding controllers that apply to all subpages and handlers for specific subpages.
 */
class GameSetupPage
{
	constructor(setupWindow, gameSettingsControl, playerAssignmentsControl, netMessages, gameRegisterStanza, mapCache, mapFilters, startGameControl, readyControl)
	{
		Engine.ProfileStart("GameSetupPage");

		// This class instance owns all gamesetting GUI controls such as dropdowns and checkboxes visible in this page.
		this.gameSettingControlManager =
			new GameSettingControlManager(setupWindow, gameSettingsControl, mapCache, mapFilters, netMessages, playerAssignmentsControl);

		// These classes manage GUI buttons.
		{
			let startGameButton = new StartGameButton(setupWindow, startGameControl, netMessages, readyControl, playerAssignmentsControl);
			let readyButton = new ReadyButton(readyControl, netMessages, playerAssignmentsControl);
			this.panelButtons = {
				"cancelButton": new CancelButton(setupWindow, startGameButton, readyButton, gameRegisterStanza),
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
				setupWindow, gameSettingTabs, gameSettingsControl, this.gameSettingControlManager);

			this.panels = {
				"chatPanel": new ChatPanel(this.gameSettingControlManager, gameSettingsControl, netMessages, playerAssignmentsControl, readyControl, gameSettingsPanel),
				"gameSettingWarning": new GameSettingWarning(gameSettingsControl, this.panelButtons.cancelButton),
				"gameDescription": new GameDescription(mapCache, gameSettingTabs, gameSettingsControl),
				"gameSettingsPanel": gameSettingsPanel,
				"gameSettingsTabs": gameSettingTabs,
				"mapPreview": new MapPreview(gameSettingsControl, mapCache),
				"resetCivsButton": new ResetCivsButton(gameSettingsControl),
				"resetTeamsButton": new ResetTeamsButton(gameSettingsControl),
				"soundNotification": new SoundNotification(netMessages, playerAssignmentsControl),
				"tipsPanel": new TipsPanel(gameSettingsPanel),
				"tooltip": new Tooltip(this.panelButtons.cancelButton)
			};
		}

		Engine.ProfileStop();
	}
}
