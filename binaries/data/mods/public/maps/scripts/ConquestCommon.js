Trigger.prototype.ConquestHandlerOwnerShipChanged = function(msg)
{
	if (!this.conquestDataInit || !this.conquestClassFilter)
		return;

	if (!TriggerHelper.EntityHasClass(msg.entity, this.conquestClassFilter))
		return;

	if (msg.from == -1)
		return;

	if (msg.to > 0 && this.conquestEntitiesByPlayer[msg.to])
		this.conquestEntitiesByPlayer[msg.to].entities.push(msg.entity);

	if (!this.conquestEntitiesByPlayer[msg.from])
	{
		if (msg.from)
			warn("ConquestHandlerOwnerShipChanged: Unknow player " + msg.from);
		return;	
	}

	let entities = this.conquestEntitiesByPlayer[msg.from].entities;
	let index = entities.indexOf(msg.entity);

	if (index >= 0)
	{
		entities.splice(index, 1);
		if (!entities.length)
		{
			let cmpPlayer = QueryPlayerIDInterface(msg.from);
			if (cmpPlayer)
				cmpPlayer.SetState("defeated");
		}
	}
}

Trigger.prototype.ConquestAddStructure = function(msg)
{
	if (!this.conquestClassFilter || !TriggerHelper.EntityHasClass(msg.building, this.conquestClassFilter))
		return;

	let cmpOwnership = Engine.QueryInterface(msg.building, IID_Ownership);
	if (!cmpOwnership)
	{
		warn("ConquestAddStructure: Structure without Owner");
		return;
	}

	let player = cmpOwnership.GetOwner();
	if (!this.conquestEntitiesByPlayer[player])
	{
		if (player != 0)
			warn("ConquestAddStructure: Unknown player " + player);
		return;	
	}

	if (this.conquestEntitiesByPlayer[player].entities.indexOf(msg.building) >= 0)
		return;

	this.conquestEntitiesByPlayer[player].entities.push(msg.building);
}

Trigger.prototype.ConquestTrainingFinished = function(msg)
{
	if (msg.owner == 0 || !this.conquestClassFilter || !msg.entities.length || !msg.entities.every(elem => TriggerHelper.EntityHasClass(elem, this.conquestClassFilter)))
		return;

	let player = msg.owner;
	if (!this.conquestEntitiesByPlayer[player])
	{
		warn("ConquestTrainingFinished: Unknown player " + player);
		return;	
	}
	this.conquestEntitiesByPlayer[player].entities.push(...msg.entities);
};

Trigger.prototype.ConquestStartGameCount = function()
{
	if (!this.conquestClassFilter)
	{
		warn("ConquestStartGameCount: conquestClassFilter undefined");
		return;
	}

	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	let cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);

	cmpPlayerManager.GetAllPlayerEntities().forEach((elem, i) => {
		if (i == 0)
			return;

		let filterEntity = ent => TriggerHelper.EntityHasClass(ent, this.conquestClassFilter) 
			&& !Engine.QueryInterface(ent, IID_Foundation);
		this.conquestEntitiesByPlayer[i] = {"player" : elem, "entities" : [...cmpRangeManager.GetEntitiesByPlayer(i).filter(filterEntity)]};
	});

	this.conquestDataInit = true;
};

var cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);

cmpTrigger.conquestEntitiesByPlayer = {};
cmpTrigger.conquestDataInit = false;
cmpTrigger.conquestClassFilter = "";
