function CeasefireManager() {}

CeasefireManager.prototype.Schema =
	"<a:help>Lists the sound groups associated with this unit.</a:help>" +
	"<a:example>" +
		"<SoundGroups>" +
			"<ceasefire>interface/alarm/alarm_alert_0.xml</ceasefire>" +
		"</SoundGroups>" +
	"</a:example>" +
	"<element name='SoundGroups'>" +
		"<zeroOrMore>" + /* TODO: make this more specific, like a list of specific elements */
			"<element>" +
				"<anyName/>" +
				"<text/>" +
			"</element>" +
		"</zeroOrMore>" +
	"</element>";

CeasefireManager.prototype.Init = function()
{
	// Weather or not ceasefire is active currently.
	this.ceasefireIsActive = false;
	
	// Ceasefire timeout in milliseconds
	this.ceasefireTime = 0;
	
	// Time elapsed when the ceasefire was started
	this.ceasefireStartedTime = 0;

	// diplomacy states before the ceasefire started
	this.diplomacyBeforeCeasefire = [];

	// Message duration for the countdown in milliseconds
	this.countdownMessageDuration = 10000;

	// Duration for the post ceasefire message in milliseconds
	this.postCountdownMessageDuration = 5000;
};

CeasefireManager.prototype.IsCeasefireActive = function()
{
	return this.ceasefireIsActive;
};

CeasefireManager.prototype.GetCeasefireStartedTime = function()
{
	return this.ceasefireStartedTime;
};

CeasefireManager.prototype.GetCeasefireTime = function()
{
	return this.ceasefireTime;
};

CeasefireManager.prototype.GetDiplomacyBeforeCeasefire = function()
{
	return this.diplomacyBeforeCeasefire;
};

CeasefireManager.prototype.StartCeasefire = function(ceasefireTime)
{
	// If invalid timeout given, return
	if (ceasefireTime <= 0)
		return;

	// Remove existing timers
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	if (this.ceasefireCountdownMessageTimer)
		cmpTimer.CancelTimer(this.ceasefireCountdownMessageTimer);

	if (this.stopCeasefireTimer)
		cmpTimer.CancelTimer(this.stopCeasefireTimer);

	// Remove existing messages
	var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	if (this.ceasefireCountdownMessage)
		cmpGuiInterface.DeleteTimeNotification(this.ceasefireCountdownMessage);

	if (this.ceasefireEndedMessage)
		cmpGuiInterface.DeleteTimeNotification(this.ceasefireEndedMessage);


	// Save diplomacy and set everyone neutral
	if (!this.ceasefireIsActive)
	{
		// Save diplomacy
		var playerEntities = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetAllPlayerEntities();
        for (var i = 1; i < playerEntities.length; i++)
		{
			// Copy array with slice(), otherwise it will change
			var cmpPlayer = Engine.QueryInterface(playerEntities[i], IID_Player);
			this.diplomacyBeforeCeasefire.push(cmpPlayer.GetDiplomacy().slice());
		}

		// Set every enemy (except gaia) to neutral
        for (var i = 1; i < playerEntities.length; i++)
        {
			var cmpPlayer = Engine.QueryInterface(playerEntities[i], IID_Player);
	        for (var j = 1; j < playerEntities.length; j++)
			{
				if (this.diplomacyBeforeCeasefire[i-1][j] < 0)
					cmpPlayer.SetNeutral(j);
			}
		}
	}
	
	// Save other data
	this.ceasefireIsActive = true;
	this.ceasefireTime = ceasefireTime;
	this.ceasefireStartedTime = cmpTimer.GetTime();

	// Send message
	Engine.PostMessage(SYSTEM_ENTITY, MT_CeasefireStarted);

	// Add timers for countdown message and reseting diplomacy
	this.stopCeasefireTimer = cmpTimer.SetTimeout(SYSTEM_ENTITY, IID_CeasefireManager, "StopCeasefire", this.ceasefireTime);
	this.ceasefireCountdownMessageTimer = cmpTimer.SetTimeout(SYSTEM_ENTITY, IID_CeasefireManager, "ShowCeasefireCountdownMessage",
			this.ceasefireTime - this.countdownMessageDuration, this.countdownMessageDuration);
};

CeasefireManager.prototype.ShowCeasefireCountdownMessage = function(duration)
{
	var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	this.ceasefireCountdownMessage = cmpGuiInterface.AddTimeNotification({
		"message": markForTranslation("You can attack in %(time)s"),
		"translateMessage": true
	}, duration);
};

CeasefireManager.prototype.StopCeasefire = function()
{
	var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);

	// Remove previous message
	if (this.ceasefireCountdownMessage)
		cmpGuiInterface.DeleteTimeNotification(this.ceasefireCountdownMessage);
	
	// Show new message
	this.ceasefireEndedMessage = cmpGuiInterface.AddTimeNotification({
		"message": markForTranslation("You can attack now!"),
		"translateMessage": true
	}, this.postCountdownMessageDuration);
	
	// Reset diplomacies to original settings
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var playerEntities = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetAllPlayerEntities();
    for (var i = 1; i < playerEntities.length; i++)
	{
		var cmpPlayer = Engine.QueryInterface(playerEntities[i], IID_Player);
		cmpPlayer.SetDiplomacy(this.diplomacyBeforeCeasefire[i-1]);
	}		

	// Send chat notifications and update the diplomacy screen
    for (var i = 1; i < playerEntities.length; i++)
	{
		var cmpPlayer = Engine.QueryInterface(playerEntities[i], IID_Player);
        for (var j = 1; j < playerEntities.length; j++)
        {
			if (i != j && this.diplomacyBeforeCeasefire[i-1][j] == -1)
				cmpGuiInterface.PushNotification({"type": "diplomacy", "players": [j], "player1": [i], "status": "enemy"});
        }
	}	
	
	// Reset values
	this.ceasefireIsActive = false;
	this.ceasefireTime = 0;
	this.ceasefireStartedTime = 0;
	this.diplomacyBeforeCeasefire = [];

	// Send message
	Engine.PostMessage(SYSTEM_ENTITY, MT_CeasefireEnded);
};

Engine.RegisterSystemComponentType(IID_CeasefireManager, "CeasefireManager", CeasefireManager);
