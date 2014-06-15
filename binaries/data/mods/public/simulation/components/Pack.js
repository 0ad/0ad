function Pack() {}

const PACKING_INTERVAL = 250;

Pack.prototype.Schema =
	"<element name='Entity' a:help='Entity to transform into'>" +
		"<text/>" +
	"</element>" +
	"<element name='Time' a:help='Time required to transform this entity, in milliseconds'>" +
		"<data type='nonNegativeInteger'/>" +
	"</element>" +
	"<element name='State' a:help='Whether this entity is packed or unpacked'>" +
		"<choice>" +
			"<value>packed</value>" +
			"<value>unpacked</value>" +
		"</choice>" +
	"</element>";

Pack.prototype.Init = function()
{
	this.packed = (this.template.State == "packed");
	this.packing = false;
	this.elapsedTime = 0;
	this.timer = undefined;
};

Pack.prototype.OnDestroy = function()
{
	this.CancelTimer();
};

Pack.prototype.CancelTimer = function()
{
	if (this.timer)
	{
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.timer);
		this.timer = undefined;
	}
};

Pack.prototype.IsPacked = function()
{
	return this.packed;
};

Pack.prototype.IsPacking = function()
{
	return this.packing;
};

Pack.prototype.Pack = function()
{
	// Ignore pointless pack command
	if (this.IsPacked() || this.IsPacking())
		return;

	this.packing = true;
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.timer = cmpTimer.SetInterval(this.entity, IID_Pack, "PackProgress", 0, PACKING_INTERVAL, {"packing": true});
	var cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (cmpVisual)
		cmpVisual.SelectAnimation("packing", true, 1.0, "packing");
};

Pack.prototype.Unpack = function()
{
	// Ignore pointless unpack command
	if (!this.IsPacked() || this.IsPacking())
		return;

	this.packing = true;
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.timer = cmpTimer.SetInterval(this.entity, IID_Pack, "PackProgress", 0, PACKING_INTERVAL, {"packing": false});
	var cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (cmpVisual)
		cmpVisual.SelectAnimation("unpacking", true, 1.0, "unpacking");
};

Pack.prototype.CancelPack = function()
{
	// Ignore pointless cancel command
	if (!this.IsPacking())
		return;

	this.CancelTimer();
	this.packing = false;
	this.SetElapsedTime(0);

	// Clear animation
	var cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (cmpVisual)
		cmpVisual.SelectAnimation("idle", false, 1.0, "");
};

Pack.prototype.GetPackTime = function()
{
	return ApplyValueModificationsToEntity("Pack/Time", +this.template.Time, this.entity);
};

Pack.prototype.GetElapsedTime = function()
{
	return this.elapsedTime;
};

Pack.prototype.GetProgress = function()
{
	return this.elapsedTime / this.GetPackTime();
};

Pack.prototype.SetElapsedTime = function(time)
{
	this.elapsedTime = time;
	Engine.PostMessage(this.entity, MT_PackProgressUpdate, { progress: this.elapsedTime });
};

Pack.prototype.PackProgress = function(data, lateness)
{
	if (this.elapsedTime >= this.GetPackTime())
	{
		this.CancelTimer();

		this.packed = !this.packed;
		Engine.PostMessage(this.entity, MT_PackFinished, { packed: this.packed });

		// Done un/packing, copy our parameters to the final entity
		var newEntity = Engine.AddEntity(this.template.Entity);
		if (newEntity == INVALID_ENTITY)
		{
			// Error (e.g. invalid template names)
			error("PackProgress: Error creating entity for '" + this.template.Entity + "'");
			return;
		}

		var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
		var cmpNewPosition = Engine.QueryInterface(newEntity, IID_Position);
		if (cmpPosition.IsInWorld())
		{
			var pos = cmpPosition.GetPosition2D();
			cmpNewPosition.JumpTo(pos.x, pos.y);
		}
		var rot = cmpPosition.GetRotation();
		cmpNewPosition.SetYRotation(rot.y);
		cmpNewPosition.SetXZRotation(rot.x, rot.z);
		cmpNewPosition.SetHeightOffset(cmpPosition.GetHeightOffset());

		var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
		var cmpNewOwnership = Engine.QueryInterface(newEntity, IID_Ownership);
		cmpNewOwnership.SetOwner(cmpOwnership.GetOwner());

		// Maintain current health level
		var cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
		var cmpNewHealth = Engine.QueryInterface(newEntity, IID_Health);
		var healthLevel = Math.max(0, Math.min(1, cmpHealth.GetHitpoints() / cmpHealth.GetMaxHitpoints()));
		cmpNewHealth.SetHitpoints(Math.round(cmpNewHealth.GetMaxHitpoints() * healthLevel));

		var cmpUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
		var cmpNewUnitAI = Engine.QueryInterface(newEntity, IID_UnitAI);
		if (cmpUnitAI && cmpNewUnitAI)
		{
			var pos = cmpUnitAI.GetHeldPosition();
			if (pos)
				cmpNewUnitAI.SetHeldPosition(pos.x, pos.z);
			if (cmpUnitAI.GetStanceName())
				cmpNewUnitAI.SwitchToStance(cmpUnitAI.GetStanceName());
			cmpNewUnitAI.AddOrders(cmpUnitAI.GetOrders());
			cmpNewUnitAI.SetGuardOf(cmpUnitAI.IsGuardOf());
		}

		// Maintain the list of guards
		var cmpGuard = Engine.QueryInterface(this.entity, IID_Guard);
		var cmpNewGuard = Engine.QueryInterface(newEntity, IID_Guard);
		if (cmpGuard && cmpNewGuard)
			cmpNewGuard.SetEntities(cmpGuard.GetEntities());

		Engine.BroadcastMessage(MT_EntityRenamed, { entity: this.entity, newentity: newEntity });

		// Play notification sound
		var sound = this.packed ? "packed" : "unpacked";
		PlaySound(sound, newEntity);

		// Destroy current entity
		Engine.DestroyEntity(this.entity);
	}
	else
	{
		this.SetElapsedTime(this.GetElapsedTime() + PACKING_INTERVAL + lateness);
	}
};

Engine.RegisterComponentType(IID_Pack, "Pack", Pack);
