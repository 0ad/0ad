function CeasefireManager() {}

CeasefireManager.prototype.Schema = "<a:component type='system'/><empty/>";

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
	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	if (this.ceasefireCountdownMessageTimer)
		cmpTimer.CancelTimer(this.ceasefireCountdownMessageTimer);

	if (this.stopCeasefireTimer)
		cmpTimer.CancelTimer(this.stopCeasefireTimer);

	// Remove existing messages
	let cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	if (this.ceasefireCountdownMessage)
		cmpGuiInterface.DeleteTimeNotification(this.ceasefireCountdownMessage);

	if (this.ceasefireEndedMessage)
		cmpGuiInterface.DeleteTimeNotification(this.ceasefireEndedMessage);

	// Save diplomacy and set everyone neutral
	if (!this.ceasefireIsActive)
	{
		// Save diplomacy
		let numPlayers = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetNumPlayers();
		for (let i = 1; i < numPlayers; ++i)
			this.diplomacyBeforeCeasefire.push(QueryPlayerIDInterface(i).GetDiplomacy());

		// Set every enemy (except gaia) to neutral
		for (let i = 1; i < numPlayers; ++i)
			for (let j = 1; j < numPlayers; ++j)
				if (this.diplomacyBeforeCeasefire[i-1][j] < 0)
					QueryPlayerIDInterface(i).SetNeutral(j);
	}

	this.ceasefireIsActive = true;
	this.ceasefireTime = ceasefireTime;
	this.ceasefireStartedTime = cmpTimer.GetTime();

	Engine.PostMessage(SYSTEM_ENTITY, MT_CeasefireStarted);

	// Add timers for countdown message and resetting diplomacy
	this.stopCeasefireTimer = cmpTimer.SetTimeout(SYSTEM_ENTITY, IID_CeasefireManager, "StopCeasefire", this.ceasefireTime);
	this.ceasefireCountdownMessageTimer = cmpTimer.SetTimeout(SYSTEM_ENTITY, IID_CeasefireManager, "ShowCeasefireCountdownMessage",
		this.ceasefireTime - this.countdownMessageDuration);
};

CeasefireManager.prototype.ShowCeasefireCountdownMessage = function()
{
	let cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	this.ceasefireCountdownMessage = cmpGuiInterface.AddTimeNotification({
			"message": markForTranslation("You can attack in %(time)s"),
			"translateMessage": true
		}, this.countdownMessageDuration);
};

CeasefireManager.prototype.StopCeasefire = function()
{
	let cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);

	if (this.ceasefireCountdownMessage)
		cmpGuiInterface.DeleteTimeNotification(this.ceasefireCountdownMessage);

	this.ceasefireEndedMessage = cmpGuiInterface.AddTimeNotification({
		"message": markForTranslation("You can attack now!"),
		"translateMessage": true
	}, this.postCountdownMessageDuration);

	// Reset diplomacies to original settings
	let numPlayers = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetNumPlayers();
	for (let i = 1; i < numPlayers; ++i)
		QueryPlayerIDInterface(i).SetDiplomacy(this.diplomacyBeforeCeasefire[i-1]);

	this.ceasefireIsActive = false;
	this.ceasefireTime = 0;
	this.ceasefireStartedTime = 0;
	this.diplomacyBeforeCeasefire = [];

	Engine.PostMessage(SYSTEM_ENTITY, MT_CeasefireEnded);

	cmpGuiInterface.PushNotification({
		"type": "ceasefire-ended",
		"players": [-1] // processed globally
	});
};

Engine.RegisterSystemComponentType(IID_CeasefireManager, "CeasefireManager", CeasefireManager);
