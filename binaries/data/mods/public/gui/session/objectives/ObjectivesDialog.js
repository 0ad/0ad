class ObjectivesDialog
{
	constructor(playerViewControl, mapCache)
	{
		this.gameDescription = Engine.GetGUIObjectByName("gameDescription");
		this.objectivesPlayerstate = Engine.GetGUIObjectByName("objectivesPlayerstate");
		this.objectivesPanel = Engine.GetGUIObjectByName("objectivesPanel");
		this.objectivesTitle = Engine.GetGUIObjectByName("objectivesTitle");

		// TODO: atlas should support this
		if (!Engine.IsAtlasRunning())
			Engine.GetGUIObjectByName("gameDescriptionText").caption = getGameDescription(mapCache);

		Engine.GetGUIObjectByName("closeObjectives").onPress = this.close.bind(this);

		registerPlayersInitHandler(this.rebuild.bind(this));
		registerPlayersFinishedHandler(this.rebuild.bind(this));
		playerViewControl.registerPlayerIDChangeHandler(this.rebuild.bind(this));
	}

	open()
	{
		this.objectivesPanel.hidden = false;
	}

	close()
	{
		this.objectivesPanel.hidden = true;
	}

	isOpen()
	{
		return !this.objectivesPanel.hidden;
	}

	toggle()
	{
		let open = this.isOpen();

		closeOpenDialogs();

		if (!open)
			this.open();
	}

	rebuild()
	{
		let player = g_Players[Engine.GetPlayerID()];
		let playerState = player && player.state;
		let isActive = !playerState || playerState == "active";

		this.objectivesPlayerstate.hidden = isActive;
		this.objectivesPlayerstate.caption = g_PlayerStateMessages[playerState] || "";

		let size = this.gameDescription.size;
		size.top = (isActive ? this.objectivesTitle : this.objectivesPlayerstate).size.bottom;
		this.gameDescription.size = size;
	}
}
