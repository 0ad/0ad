function AttackDetection() {}

AttackDetection.prototype.Schema =
	"<a:help>Detects incoming attacks.</a:help>" +
	"<a:example/>" +
	"<element name='SuppressionTransferRange' a:help='Any attacks within this range in meters will replace the previous attack suppression'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
	"<element name='SuppressionRange' a:help='Other attacks within this range in meters will not be registered'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
	"<element name='SuppressionTime' a:help='Other attacks within this time in milliseconds will not be registered'>" +
		"<data type='positiveInteger'/>" +
	"</element>";

AttackDetection.prototype.Init = function()
{
	this.suppressionTime = +this.template.SuppressionTime;
	// Use squared distance to avoid sqrts
	this.suppressionTransferRangeSquared = +this.template.SuppressionTransferRange * +this.template.SuppressionTransferRange;
	this.suppressionRangeSquared = +this.template.SuppressionRange * +this.template.SuppressionRange;
	this.suppressedList = [];
};

AttackDetection.prototype.AddSuppression = function(event)
{
	this.suppressedList.push(event);

	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	cmpTimer.SetTimeout(this.entity, IID_AttackDetection, "HandleTimeout", this.suppressionTime);
};

//// Message handlers ////

AttackDetection.prototype.OnGlobalAttacked = function(msg)
{
	var cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
	var cmpOwnership = Engine.QueryInterface(msg.target, IID_Ownership);
	if (cmpOwnership.GetOwner() == cmpPlayer.GetPlayerID())
		Engine.PostMessage(msg.target, MT_MinimapPing);
};

//// External interface ////

AttackDetection.prototype.AttackAlert = function(target, attacker)
{
	var cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
	var cmpTargetOwnership = Engine.QueryInterface(target, IID_Ownership);
	// Don't register attacks dealt against other players
	if (cmpTargetOwnership.GetOwner() != cmpPlayer.GetPlayerID())
		return;
	var cmpAttackerOwnership = Engine.QueryInterface(attacker, IID_Ownership);
	// Don't register attacks dealt by myself
	if (cmpAttackerOwnership.GetOwner() == cmpPlayer.GetPlayerID())
		return;

	var cmpPosition = Engine.QueryInterface(target, IID_Position);
	if (!cmpPosition || !cmpPosition.IsInWorld())
		return;
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	var event = {target: target, position: cmpPosition.GetPosition(), time: cmpTimer.GetTime()};

	for (var i = 0; i < this.suppressedList.length; i++)
	{
		var element = this.suppressedList[i];

		// If the new attack is within suppression distance of this element,
		// then check if the element should be updated and return
		var dist = SquaredDistance(element.position, event.position);
		if (dist < this.suppressionRangeSquared)
		{
			if (dist < this.suppressionTransferRangeSquared)
				element = event;
			return;
		}
	}

	this.AddSuppression(event);
	Engine.PostMessage(this.entity, MT_AttackDetected, { "player": cmpPlayer.GetPlayerID(), "event": event });
	var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	cmpGuiInterface.PushNotification({"type": "attack", "players": [cmpPlayer.GetPlayerID()], "attacker": cmpAttackerOwnership.GetOwner() });
	PlaySound("attacked", target);
};

AttackDetection.prototype.GetSuppressionTime = function()
{
	return this.suppressionTime;
};

AttackDetection.prototype.HandleTimeout = function()
{
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	var now = cmpTimer.GetTime();
	for (var i = 0; i < this.suppressedList.length; i++)
	{
		var event = this.suppressedList[i];

		// Check if this event has timed out
		if (now - event.time >= this.suppressionTime)
		{
			this.suppressedList.splice(i, 1);
			i--;
			return;
		}
	}
};

AttackDetection.prototype.GetIncomingAttacks = function()
{
	return this.suppressedList;
};

// Utility function for calculating the squared-distance between two attack events
function SquaredDistance(pos1, pos2)
{
	var xs = pos2.x - pos1.x;
	var zs = pos2.z - pos1.z;
	return xs*xs + zs*zs;
};

Engine.RegisterComponentType(IID_AttackDetection, "AttackDetection", AttackDetection);
