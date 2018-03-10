Trigger.prototype.ConquestOwnershipChanged = function(msg)
{
	if (!this.conquestDataInit)
		return;

	for (let query of this.conquestQueries)
	{
		if (!TriggerHelper.EntityMatchesClassList(msg.entity, query.classFilter))
			continue;

		if (msg.to > 0)
			query.entitiesByPlayer[msg.to].push(msg.entity);

		if (msg.from <= 0)
			continue;

		let entities = query.entitiesByPlayer[msg.from];
		let index = entities.indexOf(msg.entity);
		if (index != -1)
			entities.splice(index, 1);

		if (!entities.length)
		{
			let cmpPlayer = QueryPlayerIDInterface(msg.from);
			if (cmpPlayer)
				cmpPlayer.SetState("defeated", query.defeatReason);
		}
	}
};

Trigger.prototype.ConquestStartGameCount = function()
{
	if (!this.conquestQueries.length)
	{
		warn("ConquestStartGameCount: no conquestQueries set");
		return;
	}

	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	let entitiesByPlayer = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetAllPlayers().map(playerID =>
		cmpRangeManager.GetEntitiesByPlayer(playerID));

	for (let query of this.conquestQueries)
		query.entitiesByPlayer = entitiesByPlayer.map(
			ents => ents.filter(
				ent => TriggerHelper.EntityMatchesClassList(ent, query.classFilter)));

	this.conquestDataInit = true;
};

Trigger.prototype.ConquestAddVictoryCondition = function(data)
{
	this.conquestQueries.push(data);
};

{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.conquestDataInit = false;
	cmpTrigger.conquestQueries = [];
	cmpTrigger.RegisterTrigger("OnOwnershipChanged", "ConquestOwnershipChanged", { "enabled": true });
	cmpTrigger.DoAfterDelay(0, "ConquestStartGameCount", null);
}
