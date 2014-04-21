/*
	DESCRIPTION	: Functions related to reading civ info
	NOTES		: 
*/

// ====================================================================


function loadCivData()
{	// Load all JSON files containing civ data
	var civData = {};
	var civFiles = Engine.BuildDirEntList("civs/", "*.json", false);
	
	for each (var filename in civFiles)
	{	// Parse data if valid file
		var data = parseJSONData(filename);
		translateObjectKeys(data, ["Name", "Description", "History", "Special"]);
		civData[data.Code] = data;
	}
	
	return civData;
}

// ====================================================================
