Trigger.prototype.ConquestOwnershipChanged = function(msg)
{
	if (!this.conquestDataInit || !this.conquestClassFilter)
		return;

	if (!TriggerHelper.EntityMatchesClassList(msg.entity, this.conquestClassFilter))
		return;

	if (msg.to > 0)
		this.conquestEntitiesByPlayer[msg.to].push(msg.entity);

	if (msg.from > 0)
	{
		let entities = this.conquestEntitiesByPlayer[msg.from];
		let index = entities.indexOf(msg.entity);
		if (index != -1)
			entities.splice(index, 1);

		if (!entities.length)
		{
			let cmpPlayer = QueryPlayerIDInterface(msg.from);
			if (cmpPlayer)
				cmpPlayer.SetState("defeated", this.conquestDefeatReason);
		}
	}
};

Trigger.prototype.ConquestStartGameCount = function()
{
	if (!this.conquestClassFilter)
	{
		warn("ConquestStartGameCount: conquestClassFilter undefined");
		return;
	}

	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);

	let numPlayers = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetNumPlayers();
	for (let i = 1; i < numPlayers; ++i)
		this.conquestEntitiesByPlayer[i] =
			cmpRangeManager.GetEntitiesByPlayer(i).filter(ent =>
				TriggerHelper.EntityMatchesClassList(ent, this.conquestClassFilter));

	this.conquestDataInit = true;
};

{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.conquestEntitiesByPlayer = {};
	cmpTrigger.conquestDataInit = false;
	cmpTrigger.conquestClassFilter = "";
	cmpTrigger.RegisterTrigger("OnOwnershipChanged", "ConquestOwnershipChanged", { "enabled": true });
	cmpTrigger.DoAfterDelay(0, "ConquestStartGameCount", null);
}
