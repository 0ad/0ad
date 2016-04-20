Trigger.prototype.CheckWonderVictory = function(data)
{
	var ent = data.entity;
	var cmpWonder = Engine.QueryInterface(ent, IID_Wonder);
	if (!cmpWonder)
		return;

	var timer = this.wonderVictoryTimers[ent];
	var messages = this.wonderVictoryMessages[ent] || {};

	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
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
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var numPlayers = cmpPlayerManager.GetNumPlayers();
	var cmpPlayer = QueryOwnerInterface(ent, IID_Player);
	// Add -1 to notify observers too
	var players = [-1];
	for (var i = 1; i < numPlayers; i++)
		if (i != data.to)
			players.push(i);

	var time = cmpWonder.GetVictoryDuration();
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
