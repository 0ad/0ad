Trigger.prototype.IntroductionMessage = function(data)
{
	if (this.state != "start")
		return;

	var cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	// Refer to this wiki article for more information about translation support for messages: http://trac.wildfiregames.com/wiki/Internationalization
	cmpGUIInterface.PushNotification({
		"players": [1,2], 
		"message": markForTranslation("Collect the treasures before your enemy does! May the better win!"),
		translateMessage: true
	});
};

Trigger.prototype.TreasureCollected = function(data)
{
	var cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);

	var count = ++this.treasureCount.players[data.player];
	var goalCount = this.treasureCount.maximum / 2 + 1
	var otherPlayer = (data.player == 1 ? 2 : 1);
	
	// Check if having more treasures than the enemy is still possible
	if ( (count == this.treasureCount.maximum / 2) && 
		(this.treasureCount.players[otherPlayer] == this.treasureCount.maximum / 2) )
	{
		cmpGUIInterface.PushNotification({"players": [1,2], "message": "No winner yet, prepare for battle!"});
		
		// keep notifying the player that the victory condition has changed.
		var timerData = {"enabled": true, "delay": 10000, "interval": 12000}
		this.RegisterTrigger("OnInterval", "BattleMessage", timerData);
	}
	else if (count >= goalCount) // Check for victory
	{
		cmpGUIInterface.PushNotification({
			"players": [otherPlayer], 
			"message": markForTranslation("Your enemy's treasury is filled to the brim, you loose!"),
			"translateMessage": true
		});
		cmpGUIInterface.PushNotification({
			"players": [data.player], 
			"message": markForTranslation("Your treasury is filled to the brim, you are victorious!"),
			"translateMessage": true
		});
		this.DoAfterDelay(5000, "Victory", data.player);
	}
	else
	{
		// Notify if the other player if a player is close to victory (3 more treasures to collect)
		if (count + 3 == goalCount)
		{
			cmpGUIInterface.PushNotification({
				"players": [otherPlayer], 
				"message": markForTranslation("Hurry up! Your enemy is close to victory!"),
				"translateMessage": true
			});
		}
		
		if (count + 3 >= goalCount)
		{
			var remainingTreasures = ( goalCount - count);
			cmpGUIInterface.PushNotification({
				"players": [data.player],
				"parameters": {"remainingTreasures": remainingTreasures},
				"message": markForTranslation("Treasures remaining to collect for victory:  %(remainingTreasures)s!"),
				"translateMessage": true
			});
		}
		else
		{
			cmpGUIInterface.PushNotification({
				"players": [data.player], 
				"message": markForTranslation("You have collected a treasure!"),
				"translateMessage": true
			});
		}
	}
};

Trigger.prototype.BattleMessage = function()
{
	var cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	cmpGUIInterface.PushNotification({
		"players": [1,2], 
		"message": markForTranslation("Defeat your enemy to win!"),
		"translateMessage": true
	});
}

Trigger.prototype.Victory = function(playerID)
{
	TriggerHelper.SetPlayerWon(playerID);
}

var cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger); 
	
// TODO: It would be nice to get the total number of treasure on the map automatically somehow
cmpTrigger.treasureCount = { "players": { "1":0,"2":0 }, "maximum": 36 };
cmpTrigger.state = "start";
cmpTrigger.DoAfterDelay(2000, "IntroductionMessage", {});
cmpTrigger.RegisterTrigger("OnTreasureCollected", "TreasureCollected", { "enabled": true });
