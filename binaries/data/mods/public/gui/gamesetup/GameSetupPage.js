/**
 * The GamesetupPage is the root class owning all other class instances.
 * The class shall be ineligible to perform any GUI object logic and shall defer that task to owned classes.
 */
class GamesetupPage
{
	constructor(initData, hotloadData)
	{
		if (!g_Settings)
			return;

		Engine.ProfileStart("GamesetupPage");

		this.loadHandlers = new Set();
		this.closePageHandlers = new Set();
		this.getHotloadDataHandlers = new Set();

		let netMessages = new NetMessages(this);
		let startGameControl = new StartGameControl(netMessages);
		let mapCache = new MapCache();
		let mapFilters = new MapFilters(mapCache);
		let gameSettingsControl = new GameSettingsControl(this, netMessages, startGameControl, mapCache);
		let playerAssignmentsControl = new PlayerAssignmentsControl(this, netMessages);
		let readyControl = new ReadyControl(netMessages, gameSettingsControl, startGameControl, playerAssignmentsControl);

		// These class instances control central data and do not manage any GUI Object.
		this.controls = {
			"gameSettingsControl": gameSettingsControl,
			"playerAssignmentsControl": playerAssignmentsControl,
			"mapCache": mapCache,
			"mapFilters": mapFilters,
			"readyControl": readyControl,
			"startGameControl": startGameControl
		};

		// These class instances are interfaces to networked messages and do not manage any GUI Object.
		this.netMessages = {
			"netMessages": netMessages,
			"gameRegisterStanza":
				Engine.HasXmppClient() &&
				new GameRegisterStanza(
					initData, this, netMessages, gameSettingsControl, playerAssignmentsControl, mapCache)
		};

		// This class instance owns all gamesetting GUI controls such as dropdowns and checkboxes.
		// The controls also deterministically sanitize g_GameAttributes and g_PlayerAssignments
		// without broadcasting the change.
		this.gameSettingControlManager =
			new GameSettingControlManager(this, gameSettingsControl, mapCache, mapFilters, netMessages, playerAssignmentsControl);

		// These classes manage GUI buttons.
		{
			let startGameButton = new StartGameButton(this, startGameControl, netMessages, readyControl, playerAssignmentsControl);
			let readyButton = new ReadyButton(readyControl, netMessages, playerAssignmentsControl);
			this.panelButtons = {
				"cancelButton": new CancelButton(this, startGameButton, readyButton, this.netMessages.gameRegisterStanza),
				"civInfoButton": new CivInfoButton(),
				"lobbyButton": new LobbyButton(),
				"readyButton": readyButton,
				"startGameButton": startGameButton
			};
		}

		// These classes manage GUI Objects.
		{
			let gameSettingTabs = new GameSettingTabs(this, this.panelButtons.lobbyButton);
			let gameSettingsPanel = new GameSettingsPanel(
				this, gameSettingTabs, gameSettingsControl, this.gameSettingControlManager);

			this.panels = {
				"chatPanel": new ChatPanel(this.gameSettingControlManager, gameSettingsControl, netMessages, playerAssignmentsControl, readyControl, gameSettingsPanel),
				"gameSettingWarning": new GameSettingWarning(gameSettingsControl, this.panelButtons.cancelButton),
				"gameDescription": new GameDescription(mapCache, gameSettingTabs, gameSettingsControl),
				"gameSettingsPanel": gameSettingsPanel,
				"gameSettingsTabs": gameSettingTabs,
				"loadingWindow": new LoadingWindow(netMessages),
				"mapPreview": new MapPreview(gameSettingsControl, mapCache),
				"resetCivsButton": new ResetCivsButton(gameSettingsControl),
				"resetTeamsButton": new ResetTeamsButton(gameSettingsControl),
				"soundNotification": new SoundNotification(netMessages, playerAssignmentsControl),
				"tipsPanel": new TipsPanel(gameSettingsPanel),
				"tooltip": new Tooltip(this.panelButtons.cancelButton)
			};
		}

		setTimeout(displayGamestateNotifications, 1000);
		Engine.GetGUIObjectByName("setupWindow").onTick = updateTimers;

		// This event is triggered after all classes have been instantiated and subscribed to each others events
		for (let handler of this.loadHandlers)
			handler(initData, hotloadData);

		Engine.ProfileStop();

		if (gameSettingsControl.autostart)
			startGameControl.launchGame();
	}

	registerLoadHandler(handler)
	{
		this.loadHandlers.add(handler);
	}

	unregisterLoadHandler(handler)
	{
		this.loadHandlers.delete(handler);
	}

	registerClosePageHandler(handler)
	{
		this.closePageHandlers.add(handler);
	}

	unregisterClosePageHandler(handler)
	{
		this.closePageHandlers.delete(handler);
	}

	registerGetHotloadDataHandler(handler)
	{
		this.getHotloadDataHandlers.add(handler);
	}

	unregisterGetHotloadDataHandler(handler)
	{
		this.getHotloadDataHandlers.delete(handler);
	}

	getHotloadData()
	{
		let object = {};
		for (let handler of this.getHotloadDataHandlers)
			handler(object);
		return object;
	}

	closePage()
	{
		for (let handler of this.closePageHandlers)
			handler();

		if (Engine.HasXmppClient())
			Engine.SwitchGuiPage("page_lobby.xml", { "dialog": false });
		else
			Engine.SwitchGuiPage("page_pregame.xml");
	}
}
