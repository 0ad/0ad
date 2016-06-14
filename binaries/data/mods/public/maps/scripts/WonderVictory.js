Trigger.prototype.CheckWonderVictory = function(data)
{
	let ent = data.entity;
	let cmpWonder = Engine.QueryInterface(ent, IID_Wonder);
	if (!cmpWonder)
		return;

	let timer = this.wonderVictoryTimers[ent];
	let messages = this.wonderVictoryMessages[ent] || {};

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	let cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);

	// Remove existing messages if any
	if (timer)
	{
		cmpTimer.CancelTimer(timer);
		cmpGuiInterface.DeleteTimeNotification(messages.ownMessage);
		cmpGuiInterface.DeleteTimeNotification(messages.otherMessage);
	}

	if (data.to <= 0)
		return;

	// Create new messages, and start timer to register defeat.
	let cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	let numPlayers = cmpPlayerManager.GetNumPlayers();

	// Add -1 to notify observers too
	let players = [-1];
	for (let i = 1; i < numPlayers; i++)
		if (i != data.to)
			players.push(i);

	let time = cmpWonder.GetVictoryDuration();
	let cmpPlayer = QueryOwnerInterface(ent, IID_Player);

	messages.otherMessage = cmpGuiInterface.AddTimeNotification({
		"message": markForTranslation("%(player)s will have won in %(time)s"),
		"players": players,
		"parameters": {"player": cmpPlayer.GetName()},
		"translateMessage": true,
		"translateParameters": [],
	}, time);

	messages.ownMessage = cmpGuiInterface.AddTimeNotification({
		"message": markForTranslation("You will have won in %(time)s"),
		"players": [data.to],
		"translateMessage": true,
	}, time);

	timer = cmpTimer.SetTimeout(SYSTEM_ENTITY, IID_EndGameManager, "MarkPlayerAsWon", time, data.to);

	this.wonderVictoryTimers[ent] = timer;
	this.wonderVictoryMessages[ent] = messages;
};

var cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);

var data = {"enabled": true};
cmpTrigger.RegisterTrigger("OnOwnershipChanged", "CheckWonderVictory", data);
cmpTrigger.wonderVictoryTimers = {};
cmpTrigger.wonderVictoryMessages = {};
