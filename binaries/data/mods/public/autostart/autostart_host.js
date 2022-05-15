class AutoStartHost
{
	constructor(initData)
	{
		this.launched = false;
		this.maxPlayers = initData.maxPlayers;
		this.storeReplay = initData.storeReplay;
		this.playerAssignments = {};

		this.initAttribs = initData.attribs;

		try
		{
			// Stun and password not implemented for autostart.
			Engine.StartNetworkHost(initData.playerName, initData.port, false, "", initData.storeReplay);
		}
		catch (e)
		{
			const message = sprintf(translate("Cannot host game: %(message)s."), { "message": e.message });
			messageBox(400, 200, message, translate("Error"));
		}
	}

	/**
	 * Handles a simple implementation of player assignments.
	 * Should not need be overloaded in mods unless you want to change that logic.
	 */
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
				Engine.SendNetworkReady(2);
				let max = 0;
				for (const uid in this.playerAssignments)
				{
					max = Math.max(this.playerAssignments[uid].player, max);
					if (this.playerAssignments[uid].player == -1)
						Engine.AssignNetworkPlayer(++max, uid);
				}
				break;
			case "ready":
				this.playerAssignments[message.guid].status = message.status;
				break;
			case "start":
				return true;
			default:
			}
		}

		if (!this.launched && Object.keys(this.playerAssignments).length == this.maxPlayers)
		{
			for (const uid in this.playerAssignments)
				if (this.playerAssignments[uid].player == -1 || this.playerAssignments[uid].status == 0)
					return false;
			this.onLaunch();
		}
		return false;
	}

	/**
	 * In the visual autostart path, we need to show the loading screen.
	 * Overload this as appropriate.
	 */
	onLaunch()
	{
		this.launched = true;

		this.settings = new GameSettings().init();
		this.settings.fromInitAttributes(this.initAttribs);
		this.settings.playerCount.setNb(Object.keys(this.playerAssignments).length);
		this.settings.launchGame(this.playerAssignments, this.storeReplay);
		Engine.SwitchGuiPage("page_loading.xml", {
			"attribs": this.initAttribs,
			"playerAssignments": this.playerAssignments
		});
	}
}
