function ResourceSupply() {}

ResourceSupply.prototype.Init = function()
{
	// Current resource amount (non-negative; can be a fractional amount)
	this.amount = this.GetMaxAmount();
};

ResourceSupply.prototype.GetMaxAmount = function()
{
	return +this.template.Amount;
};

ResourceSupply.prototype.GetCurrentAmount = function()
{
	return this.amount;
};

ResourceSupply.prototype.TakeResources = function(rate)
{
	// Internally we handle fractional resource amounts (to be accurate
	// over long periods of time), but want to return integers (so players
	// have a nice simple integer amount of resources). So return the
	// difference between rounded values:

	var old = this.amount;
	this.amount = Math.max(0, old - rate/1000);
	var change = Math.ceil(old) - Math.ceil(this.amount);
	// (use ceil instead of floor so that we continue returning non-zero values even if
	// 0 < amount < 1)
	return { "amount": change, "exhausted": (old == 0) };
};

ResourceSupply.prototype.GetType = function()
{
	if (this.template.Subtype)
		return { "generic": this.template.Type, "specific": this.template.Subtype };
	else
		return { "generic": this.template.Type };
};

Engine.RegisterComponentType(IID_ResourceSupply, "ResourceSupply", ResourceSupply);
