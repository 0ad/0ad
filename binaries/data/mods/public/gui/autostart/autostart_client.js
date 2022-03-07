class AutoStartClient
{
	constructor(initData)
	{
		Engine.GetGUIObjectByName("ticker").onTick = this.onTick.bind(this);

		this.playerAssignments = {};

		try
		{
			Engine.StartNetworkJoin(initData.playerName, initData.ip, initData.port, initData.storeReplay);
		}
		catch (e)
		{
			messageBox(
				400, 200,
				sprintf(translate("Cannot join game: %(message)s."), { "message": e.message }),
				translate("Error")
			);
		}
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
			case "start":
				Engine.SwitchGuiPage("page_loading.xml", {
					"attribs": message.initAttributes,
					"isRejoining": true,
					"playerAssignments": this.playerAssignments
				});

				// Process further pending netmessages in the session page.
				return;
			default:
			}
		}
	}
}

function init(initData)
{
	new AutoStartClient(initData);
}
