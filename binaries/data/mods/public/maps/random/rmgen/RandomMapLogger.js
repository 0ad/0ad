function RandomMapLogger()
{
	this.lastTime = undefined;
	this.startTime = Date.now();
	this.prefix = ""; // seems noisy

	this.printDirectly(
		this.prefix +
		"Generating " + g_MapSettings.Name +
		" of size " + g_MapSettings.Size +
		" and "  + getNumPlayers() + " players.\n");
}

RandomMapLogger.prototype.printDirectly = function(string)
{
	log(string);
	print(string);
};

RandomMapLogger.prototype.print = function(string)
{
	this.printDuration();
	this.printDirectly(this.prefix + string + "...");
	this.lastTime = Date.now();
};

RandomMapLogger.prototype.printDuration = function()
{
	if (!this.lastTime)
		return;

	this.printDurationDirectly("", this.lastTime);
	this.lastTime = Date.now();
};

RandomMapLogger.prototype.close = function()
{
	this.printDuration();
	this.printDurationDirectly(this.prefix + "Total map generation time:", this.startTime);
};

RandomMapLogger.prototype.printDurationDirectly = function(text, startTime)
{
	this.printDirectly(text + " " + ((Date.now() - startTime) / 1000).toFixed(3) + "s.\n");
};
