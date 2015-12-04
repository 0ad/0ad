/**
 * Loads history and some gameplay data for all (or all playable) civs.
 *
 *  @param playableOnly {boolean}
 */
function loadCivData(playableOnly = false)
{
	var civData = {};
	var civFiles = Engine.BuildDirEntList("simulation/data/civs/", "*.json", false);

	for (let filename of civFiles)
	{
		var data = Engine.ReadJSONFile(filename);
		if (!data)
			continue;

		translateObjectKeys(data, ["Name", "Description", "History", "Special"]);
		if (!playableOnly || data.SelectableInGameSetup)
			civData[data.Code] = data;

		// Sanity check
		for (let prop of ["Code", "Culture", "Name", "Emblem", "History", "Music", "Factions", "CivBonuses",
		                  "TeamBonuses", "Structures", "StartEntities", "Formations", "AINames","SelectableInGameSetup"])
			if (!data[prop])
				error(filename + " doesn't contain " + prop);
	}

	return civData;
}
