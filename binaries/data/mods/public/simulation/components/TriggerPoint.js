function TriggerPoint() {}

TriggerPoint.prototype.Schema =
	"<optional>" +
		"<element name='Reference'>" +
			"<text/>" +
		"</element>" +
	"</optional>";

TriggerPoint.prototype.Init = function()
{
	if (this.template && this.template.Reference)
	{
		var cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
		cmpTrigger.RegisterTriggerPoint(this.template.Reference, this.entity);
	}
	this.currentCollections = {};
	this.actions = {};
};

TriggerPoint.prototype.OnDestroy = function()
{
	if (this.template && this.template.EntityReference)
	{
		var cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
		cmpTrigger.RemoveRegisteredTriggerPoint(this.template.Reference, this.entity);
	}
};

/**
 * @param action Name of the action function defined under Trigger
 * @param data The data is an object containing information for the range query
 * Some of the data has sendible defaults (mentionned next to the object)
 * data.players = [1,2,3,...]  * list of player ids
 * data.minRange = 0           * Minimum range for the query
 * data.maxRange = -1          * Maximum range for the query (-1 = no maximum)
 * data.requiredComponent = -1 * Required component id the entities will have
 * data.enabled = false        * If the query is enabled by default
 */
TriggerPoint.prototype.RegisterRangeTrigger = function(action, data)
{
	
	if (data.players)
		var players = data.players;
	else
	{
		var numPlayers = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetNumPlayers();
		var players = [];
		for (var i = 0; i < numPlayers; i++)
			players.push(i);
	}
	var minRange = data.minRange || 0;
	var maxRange = data.maxRange || -1;
	var cid = data.requiredComponent || -1;
	
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var tag = cmpRangeManager.CreateActiveQuery(this.entity, minRange, maxRange, players, cid, cmpRangeManager.GetEntityFlagMask("normal"));

    this.currentCollections[tag] = [];
	this.actions[tag] = action;
	return tag;
};

TriggerPoint.prototype.OnRangeUpdate = function(msg)
{
	var collection = this.currentCollections[msg.tag];
	if (!collection)
		return;

	for (var ent of msg.removed)
	{
		var index = collection.indexOf(ent);
		if (index > -1)
			collection.splice(index, 1);
	}
		
	for each (var entity in msg.added)
		collection.push(entity);

	var r = {"currentCollection": collection.slice()};
	r.added = msg.added;
	r.removed = msg.removed;
	var cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.DoAction({"action":this.actions[msg.tag], "data": r});
};


Engine.RegisterComponentType(IID_TriggerPoint, "TriggerPoint", TriggerPoint);
