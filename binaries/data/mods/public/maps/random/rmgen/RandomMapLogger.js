function RandomMapLogger()
{
	this.lastTime = undefined;
	this.startTime = Engine.GetMicroseconds ? Engine.GetMicroseconds() : 0;
	this.prefix = ""; // seems noisy

	// Don't print in test cases
	if (g_MapSettings.Name)
		this.printDirectly(
			this.prefix +
			"Generating " + g_MapSettings.Name +
			" of size " + g_MapSettings.Size +
			" and " + (g_MapSettings.PlayerData.length - 1) + " players.\n");
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
	this.lastTime = Engine.GetMicroseconds();
};

RandomMapLogger.prototype.printDuration = function()
{
	if (!this.lastTime)
		return;

	this.printDurationDirectly("", this.lastTime);
	this.lastTime = Engine.GetMicroseconds();
};

RandomMapLogger.prototype.close = function()
{
	this.printDuration();
	this.printDurationDirectly(this.prefix + "Total map generation time:", this.startTime);
};

RandomMapLogger.prototype.printDurationDirectly = function(text, startTime)
{
	this.printDirectly(text + " " + ((Engine.GetMicroseconds() - startTime) / 1000000).toFixed(6) + "s.\n");
};
