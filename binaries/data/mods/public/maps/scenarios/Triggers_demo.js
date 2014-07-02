warn("loading the triggers file");

///////////////////////
// Trigger listeners //
///////////////////////

// every function just logs when it gets fired, and shows the data
Trigger.prototype.StructureBuiltAction = function(data)
{
	warn("The OnStructureBuilt event happened with the following data:");
	warn(uneval(data));
};

Trigger.prototype.ConstructionStartedAction = function(data)
{
	warn("The OnConstructionStarted event happened with the following data:");
	warn(uneval(data));
};

Trigger.prototype.TrainingFinishedAction = function(data)
{
	warn("The OnTrainingFinished event happened with the following data:");
	warn(uneval(data));
};

Trigger.prototype.TrainingQueuedAction = function(data)
{
	warn("The OnTrainingQueued event happened with the following data:");
	warn(uneval(data));
};

Trigger.prototype.ResearchFinishedAction = function(data)
{
	warn("The OnResearchFinished event happened with the following data:");
	warn(uneval(data));
};

Trigger.prototype.ResearchQueuedAction = function(data)
{
	warn("The OnResearchQueued event happened with the following data:");
	warn(uneval(data));
};

Trigger.prototype.OwnershipChangedAction = function(data)
{
	warn("The OnOwnershipChanged event happened with the following data:");
	warn(uneval(data));
};

Trigger.prototype.PlayerCommandAction = function(data)
{
	warn("The OnPlayerCommand event happened with the following data:");
	warn(uneval(data));
};

Trigger.prototype.IntervalAction = function(data)
{
	warn("The OnInterval event happened with the following data:");
	warn(uneval(data));
	this.numberOfTimerTrigger++;
	if (this.numberOfTimerTrigger >= this.maxNumberOfTimerTrigger)
		this.DisableTrigger("OnInterval", "IntervalAction");

	// try out the dialog
	var cmpGUIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	cmpGUIInterface.PushNotification({
		"type": "dialog",
		"players": [1,2,3,4,5,6,7,8],
		"dialogName": "yes-no",
		"data": {
			"text": {
				"caption": {
					"message": markForTranslation("Testing the yes-no dialog. Do you want to say sure or rather not?"),
					"translateMessage": true,
				},
			},
			"button1": {
				"caption": {
					"message": markForTranslation("Sure"),
					"translateMessage": true,
				},
				"tooltip": {
					"message": markForTranslation("Say sure"),
					"translateMessage": true,
				},
			},
			"button2": {
				"caption": {
					"message": markForTranslation("Rather not"),
					"translateMessage": true,
				},
				"tooltip": {
					"message": markForTranslation("Say rather not"),
					"translateMessage": true,
				},
			},

		},
	});
};

Trigger.prototype.RangeAction = function(data)
{
	warn("The OnRange event happened with the following data:");
	warn(uneval(data));
};

// Activate all possible triggers
var cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);

var data = {"enabled": true};
cmpTrigger.RegisterTrigger("OnStructureBuilt", "StructureBuiltAction", data);
cmpTrigger.RegisterTrigger("OnConstructionStarted", "ConstructionStartedAction", data);
cmpTrigger.RegisterTrigger("OnTrainingFinished", "TrainingFinishedAction", data);
cmpTrigger.RegisterTrigger("OnTrainingQueued", "TrainingQueuedAction", data);
cmpTrigger.RegisterTrigger("OnResearchFinished", "ResearchFinishedAction", data);
cmpTrigger.RegisterTrigger("OnResearchQueued", "ResearchQueuedAction", data);
cmpTrigger.RegisterTrigger("OnOwnershipChanged", "OwnershipChangedAction", data);
cmpTrigger.RegisterTrigger("OnPlayerCommand", "PlayerCommandAction", data);

data.delay = 10000; // after 10 seconds
data.interval = 5000; // every 5 seconds
cmpTrigger.numberOfTimerTrigger = 0;
cmpTrigger.maxNumberOfTimerTrigger = 3; // execute it 3 times maximum
cmpTrigger.RegisterTrigger("OnInterval", "IntervalAction", data);
var entities = cmpTrigger.GetTriggerPoints("A");
data = {
	"entities": entities, // central points to calculate the range circles
	"players": [1], // only count entities of player 1
	"maxRange": 40,
	"requiredComponent": IID_UnitAI, // only count units in range
	"enabled": true,
};
cmpTrigger.RegisterTrigger("OnRange", "RangeAction", data);
