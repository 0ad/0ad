Engine.GetTemplate = path => (
	{
		"Identity": {
			"GenericName": null,
			"Icon": null,
			"History": null
		}
	});

Engine.LoadLibrary("rmgen");

RandomMapLogger.prototype.printDirectly = function(string)
{
	log(string);
	// print(string);
};

function* GenerateMap(mapSettings)
{
	TS_ASSERT_DIFFER(mapSettings.Seed, undefined);
	// Phew... that assertion took a while. ;) Let's update the progress bar.
	yield 50;
	return new RandomMap(0, "blackness");
}
