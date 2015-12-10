/**
 * Loads history and some gameplay data for civs.
 *
 *  @param selectableOnly {boolean} - Only load those which can be selected
 *         in the gamesetup. Scenario maps might set non-selectable civs.
 */
function loadCivData(selectableOnly = false)
{
	var civData = {};
	var civFiles = Engine.BuildDirEntList("simulation/data/civs/", "*.json", false);

	for (let filename of civFiles)
	{
		var data = Engine.ReadJSONFile(filename);
		if (!data)
			continue;

		translateObjectKeys(data, ["Name", "Description", "History", "Special"]);
		if (!selectableOnly || data.SelectableInGameSetup)
			civData[data.Code] = data;

		// Sanity check
		for (let prop of ["Code", "Culture", "Name", "Emblem", "History", "Music", "Factions", "CivBonuses",
		                  "TeamBonuses", "Structures", "StartEntities", "Formations", "AINames","SelectableInGameSetup"])
			if (data[prop] == undefined)
				error(filename + " doesn't contain " + prop);
	}

	return civData;
}
