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
	this.packed = this.template.State == "packed";
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
		let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
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
	if (this.IsPacked() || this.IsPacking())
		return;

	this.packing = true;

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.timer = cmpTimer.SetInterval(this.entity, IID_Pack, "PackProgress", 0, PACKING_INTERVAL, { "packing": true });

	let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (cmpVisual)
		cmpVisual.SelectAnimation("packing", true, 1.0, "packing");
};

Pack.prototype.Unpack = function()
{
	if (!this.IsPacked() || this.IsPacking())
		return;

	this.packing = true;

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.timer = cmpTimer.SetInterval(this.entity, IID_Pack, "PackProgress", 0, PACKING_INTERVAL, { "packing": false });

	let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (cmpVisual)
		cmpVisual.SelectAnimation("unpacking", true, 1.0);
};

Pack.prototype.CancelPack = function()
{
	if (!this.IsPacking())
		return;

	this.CancelTimer();
	this.packing = false;
	this.SetElapsedTime(0);

	// Clear animation
	let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (cmpVisual)
		cmpVisual.SelectAnimation("idle", false, 1.0);
};

Pack.prototype.GetPackTime = function()
{
	let cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);

	return ApplyValueModificationsToEntity("Pack/Time", +this.template.Time, this.entity) *
		cmpPlayer.GetCheatTimeMultiplier();
};

Pack.prototype.GetElapsedTime = function()
{
	return this.elapsedTime;
};

Pack.prototype.GetProgress = function()
{
	return Math.min(this.elapsedTime / this.GetPackTime(), 1);
};

Pack.prototype.SetElapsedTime = function(time)
{
	this.elapsedTime = time;
	Engine.PostMessage(this.entity, MT_PackProgressUpdate, { "progress": this.elapsedTime });
};

Pack.prototype.PackProgress = function(data, lateness)
{
	if (this.elapsedTime < this.GetPackTime())
	{
		this.SetElapsedTime(this.GetElapsedTime() + PACKING_INTERVAL + lateness);
		return;
	}

	this.CancelTimer();
	this.packed = !this.packed;
	this.packing = false;

	Engine.PostMessage(this.entity, MT_PackFinished, { "packed": this.packed });

	let newEntity = ChangeEntityTemplate(this.entity, this.template.Entity);

	if (newEntity)
		PlaySound(this.packed ? "packed" : "unpacked", newEntity);

};

Engine.RegisterComponentType(IID_Pack, "Pack", Pack);
