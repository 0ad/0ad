Trigger.prototype.InitTutorial = function(data)
{
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
			if (key === "Init")
				continue;
			if (key === "IsDone")
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

	Trigger.prototype.Init = goal.Init || null;
	if (!deserializing && this.Init)
		this.Init();

	Trigger.prototype.IsDone = goal.IsDone || (() => false);
	let goalAlreadyDone = this.IsDone();

	for (let event of this.tutorialEvents)
	{
		let action = event + "Trigger";
		if (goal[event])
		{
			Trigger.prototype[action] = goal[event];
			this.EnableTrigger(event, action);
			if (!goalAlreadyDone)
				needDelay = false;
		}
		else
			this.DisableTrigger(event, action);
	}

	// Goals without actions to be performed by the player must have
	// - either the property delay (a value > 0 to wait for a given time, and -1 to display the Ready button)
	// - or no trigger functions (needDelay will be set automatically to true and the Ready button displayed)
	if (goal.delay || needDelay)
	{
		if (goal.delay && goal.delay > 0)
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

Trigger.prototype.GoalMessage = function(instructions, readyButton=false, leave=false)
{
	let cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	cmpGUIInterface.PushNotification({
		"type": "tutorial",
		"players": [1],
		"instructions": instructions,
		"readyButton": readyButton,
		"leave": leave
	});
};

Trigger.prototype.WarningMessage = function(warning)
{
	let cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	cmpGUIInterface.PushNotification({
		"type": "tutorial",
		"players": [1],
		"warning": warning
	});
};

Trigger.prototype.OnDeserializedTrigger = function()
{
	this.index = Math.max(0, this.index - 1);

	// Display messages from already processed goals
	for (let i = 0; i < this.index; ++i)
		this.GoalMessage(this.tutorialGoals[i].instructions, false, false);

	this.NextGoal(true);
};
