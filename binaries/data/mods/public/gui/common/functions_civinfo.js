/*
	DESCRIPTION	: Functions related to reading civ info
	NOTES		: 
*/

// ====================================================================


function loadCivData()
{	// Load all JSON files containing civ data
	var civData = {};
	var civFiles = buildDirEntList("civs/", "*.json", false);
	
	for each (var filename in civFiles)
	{	// Parse data if valid file
		var data = parseJSONData(filename);
		civData[data.Code] = data;
	}
	
	return civData;
}

// ====================================================================