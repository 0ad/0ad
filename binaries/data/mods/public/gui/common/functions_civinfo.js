/*
	DESCRIPTION	: Functions related to reading civ info
	NOTES		: 
*/

function loadCivData(playableOnly = false)
{
	// Load all JSON files containing civ data
	var civData = {};
	var civFiles = Engine.BuildDirEntList("simulation/data/civs/", "*.json", false);

	for each (var filename in civFiles)
	{
		// Parse data if valid file
		var data = Engine.ReadJSONFile(filename);
		if (!data)
			continue;

		translateObjectKeys(data, ["Name", "Description", "History", "Special"]);
		if (!playableOnly || data.SelectableInGameSetup)
			civData[data.Code] = data;
	}

	return civData;
}
