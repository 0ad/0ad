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
data.interval = 1000; // every second
cmpTrigger.numberOfTimerTrigger = 0;
cmpTrigger.maxNumberOfTimerTrigger = 10; // execute it 10 times maximum
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
