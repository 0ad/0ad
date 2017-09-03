/**
 * Loads history and some gameplay data for civs.
 *
 *  @param selectableOnly {boolean} - Only load those which can be selected
 *         in the gamesetup. Scenario maps might set non-selectable civs.
 *  @param gaia {boolean} - Whether to include gaia as a mock civ.
 */
function loadCivData(selectableOnly, gaia)
{
	let civData = {};
	let civFiles = Engine.BuildDirEntList("simulation/data/civs/", "*.json", false);

	for (let filename of civFiles)
	{
		let data = Engine.ReadJSONFile(filename);
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

	if (gaia)
		civData.gaia = { "Code": "gaia", "Name": translate("Gaia") };

	return deepfreeze(civData);
}
