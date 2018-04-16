/**
 * This will print the statistics at the end of a game.
 * In order for this to work, the player's state has to be changed before the event.
 */
Trigger.prototype.EndGameAction = function()
{
	if (!this.once || Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetActivePlayers().length)
		return;

	this.once = false;

	for (let player of Engine.GetEntitiesWithInterface(IID_StatisticsTracker))
	{
		let cmpStatisticsTracker = Engine.QueryInterface(player, IID_StatisticsTracker);
		if (cmpStatisticsTracker)
			print(cmpStatisticsTracker.GetStatisticsJSON() + "\n");
	}
};

{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.RegisterTrigger("OnPlayerWon", "EndGameAction", { "enabled": true });
	cmpTrigger.RegisterTrigger("OnPlayerDefeated", "EndGameAction", { "enabled": true });
	cmpTrigger.once = true;
}
