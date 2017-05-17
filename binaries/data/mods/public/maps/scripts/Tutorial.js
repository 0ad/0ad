Trigger.prototype.InitTutorial = function(data)
{
	this.count = 0;
	this.index = 0;
	this.fullText = "";
	this.tutorialEvents = [];

	// Register needed triggers
	this.RegisterTrigger("OnDeserialized", "OnDeserializedTrigger", { "enabled": true });
	this.RegisterTrigger("OnPlayerCommand", "OnPlayerCommandTrigger", { "enabled": false });
	this.tutorialEvents.push("OnPlayerCommand");

	for (let goal of this.tutorialGoals)
	{
		for (let key in goal)
		{
			if (typeof goal[key] !== "function" || this.tutorialEvents.indexOf(key) != -1)
				continue;
			let action = key + "Trigger";
			this.RegisterTrigger(key, action, { "enabled": false });
			this.tutorialEvents.push(key);
		}
	}

	this.NextGoal();
};

Trigger.prototype.NextGoal = function(deserializing = false)
{
	if (this.index > this.tutorialGoals.length)
		return;
	let goal = this.tutorialGoals[this.index];
	let needDelay = true;
	let readyButton = false;
	for (let event of this.tutorialEvents)
	{
		let action = event + "Trigger";
		if (goal[event])
		{
			Trigger.prototype[action] = goal[event];
			this.EnableTrigger(event, action);
			needDelay = false;
			if (!deserializing)
				this.count = 0;
		}
		else
			this.DisableTrigger(event, action);
	}
	if (needDelay)	// no actions for the next goal
	{
		if (goal.delay)
			this.DoAfterDelay(+goal.delay, "NextGoal", {});
		else
		{
			this.EnableTrigger("OnPlayerCommand", "OnPlayerCommandTrigger");
			Trigger.prototype.OnPlayerCommandTrigger = function(msg)
			{ 
				if (msg.cmd.type == "dialog-answer" && msg.cmd.tutorial && msg.cmd.tutorial == "ready")
					this.NextGoal();
			};
			readyButton = true;
		}
	}

	this.GoalMessage(goal.instructions, readyButton, ++this.index == this.tutorialGoals.length);
};

Trigger.prototype.GoalMessage = function(text, readyButton=false, leave=false)
{
	let cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	cmpGUIInterface.PushNotification({
		"type": "tutorial",
		"players": [1],
		"message": text,
		"translateMessage": true,
		"readyButton": readyButton,
		"leave": leave
	});
};

Trigger.prototype.WarningMessage = function(txt)
{
	let cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	cmpGUIInterface.PushNotification({
		"type": "tutorial",
		"players": [1],
		"message": txt,
		"translateMessage": true,
		"warning": true
	});
};

Trigger.prototype.OnPlayerCommandTrigger = function() {};

Trigger.prototype.OnResearchQueuedTrigger = function() {};

Trigger.prototype.OnResearchFinishedTrigger = function() {};

Trigger.prototype.OnStructureBuiltTrigger = function() {};

Trigger.prototype.OnTrainingQueuedTrigger = function() {};

Trigger.prototype.OnTrainingFinishedTrigger = function() {};

Trigger.prototype.OnDeserializedTrigger = function()
{
	this.index = Math.max(0, this.index - 1);

	// Display messages from already processed goals
	for (let i = 0; i < this.index; ++i)
		this.GoalMessage(this.tutorialGoals[i].instructions, false, false);

	this.NextGoal(true);
};
