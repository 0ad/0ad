/**
 * This class stores the GameSetupPage and every subpage that is shown in the game setup.
 */
class SetupWindowPages
{
}

/**
 * The SetupWindow is the root class owning all other class instances.
 * The class shall be ineligible to perform any GUI object logic and shall defer that task to owned classes.
 */
class SetupWindow
{
	constructor(initData, hotloadData)
	{
		if (!g_Settings)
			return;

		Engine.ProfileStart("SetupWindow");

		this.loadHandlers = new Set();
		this.closePageHandlers = new Set();
		this.getHotloadDataHandlers = new Set();

		if (initData?.backPage)
			this.backPage = initData.backPage;

		const mapCache = new MapCache();
		g_GameSettings = new GameSettings().init(mapCache);

		let netMessages = new NetMessages();
		let mapFilters = new MapFilters(mapCache);
		let playerAssignmentsController = new PlayerAssignmentsController(this, netMessages);
		let gameSettingsController = new GameSettingsController(this, netMessages, playerAssignmentsController, mapCache);
		let readyController = new ReadyController(netMessages, gameSettingsController, playerAssignmentsController);
		let lobbyGameRegistrationController = Engine.HasXmppClient() &&
			new LobbyGameRegistrationController(initData, this, netMessages, mapCache, playerAssignmentsController);

		// These class instances control central data and do not manage any GUI Object.
		this.controls = {
			"gameSettingsController": gameSettingsController,
			"playerAssignmentsController": playerAssignmentsController,
			"mapCache": mapCache,
			"mapFilters": mapFilters,
			"readyController": readyController,
			"netMessages": netMessages,
			"lobbyGameRegistrationController": lobbyGameRegistrationController
		};

		// These are the pages within the setup window that may use the controls defined above
		this.pages = {};
		for (let name in SetupWindowPages)
			this.pages[name] = new SetupWindowPages[name](this);

		netMessages.registerNetMessageHandler("netwarn", addNetworkWarning);
		setTimeout(displayGamestateNotifications, 1000);
		Engine.GetGUIObjectByName("setupWindow").onTick = () => this.onTick();

		// This event is triggered after all classes have been instantiated and subscribed to each others events
		for (let handler of this.loadHandlers)
			handler(initData, hotloadData);

		Engine.ProfileStop();
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

	onTick()
	{
		this.controls.netMessages.pollPendingMessages();
		updateTimers();
	}

	closePage()
	{
		for (let handler of this.closePageHandlers)
			handler();

		Engine.DisconnectNetworkGame();

		if (this.backPage)
			Engine.SwitchGuiPage(this.backPage.page, this.backPage?.data);
		else if (Engine.HasXmppClient())
			Engine.SwitchGuiPage("page_lobby.xml", { "dialog": false });
		else
			Engine.SwitchGuiPage("page_pregame.xml");
	}
}
