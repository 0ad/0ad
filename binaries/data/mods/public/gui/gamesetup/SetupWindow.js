/**
 * This class stores the GameSetupPage and every subpage that is shown in the gamesetup.
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
			"startGameControl": startGameControl,
			"netMessages": netMessages,
			"gameRegisterStanza":
				Engine.HasXmppClient() &&
				new GameRegisterStanza(
					initData, this, netMessages, gameSettingsControl, playerAssignmentsControl, mapCache)
		};

		// These are the pages within the setup window that may use the controls defined above
		this.pages = {};
		for (let name in SetupWindowPages)
			this.pages[name] = new SetupWindowPages[name](this);

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
