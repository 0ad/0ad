class AutoStartHost
{
	constructor(initData)
	{
		this.maxPlayers = initData.maxPlayers;
		this.storeReplay = initData.storeReplay;
		this.playerAssignments = {};

		Engine.GetGUIObjectByName("ticker").onTick = this.onTick.bind(this);

		try
		{
			// Stun and password not implemented for autostart.
			Engine.StartNetworkHost(initData.playerName, initData.port, false, "", initData.storeReplay);
		}
		catch (e)
		{
			messageBox(
				400, 200,
				sprintf(translate("Cannot host game: %(message)s."), { "message": e.message }),
				translate("Error")
			);
		}

		this.settings = new GameSettings().init();
		this.settings.fromInitAttributes(initData.attribs);
	}

	onTick()
	{
		while (true)
		{
			const message = Engine.PollNetworkClient();
			if (!message)
				break;

			switch (message.type)
			{
			case "players":
				this.playerAssignments = message.newAssignments;
				break;
			default:
			}
		}

		if (Object.keys(this.playerAssignments).length == this.maxPlayers)
		{
			this.settings.launchGame(this.playerAssignments, this.storeReplay);

			Engine.SwitchGuiPage("page_loading.xml", {
				"attribs": this.settings.finalizedAttributes,
				"playerAssignments": this.playerAssignments
			});
		}
	}
}

function init(initData)
{
	new AutoStartHost(initData);
}
