class MapFilters
{
	constructor(mapCache)
	{
		this.mapCache = mapCache;
	}

	/**
	 * Some map filters may reject every map of a particular mapType.
	 * This function allows identifying which map filters have any matches for that maptype.
	 */
	getAvailableMapFilters(mapTypeName)
	{
		return this.Filters.filter(filter =>
			this.getFilteredMaps(mapTypeName, filter.Name, true));
	}

	/**
	 * This function identifies all maps matching the given mapType and mapFilter.
	 * If existence is true, it will only test if there is at least one file for that mapType and mapFilter.
	 * Otherwise it returns an array with filename, translated map title and map description.
	 */
	getFilteredMaps(mapTypeName, filterName, existence)
	{
		let index = g_MapTypes.Name.findIndex(name => name == mapTypeName);
		if (index == -1)
		{
			error("Can't get filtered maps for invalid maptype: " + mapTypeName);
			return undefined;
		}

		let mapFilter = this.Filters.find(filter => filter.Name == filterName);
		if (!mapFilter)
		{
			error("Invalid mapfilter name: " + filterName);
			return undefined;
		}

		Engine.ProfileStart("getFilteredMaps");

		let maps = [];
		let mapTypePath = g_MapTypes.Path[index];
		for (let filename of listFiles(mapTypePath, g_MapTypes.Suffix[index], false))
		{
			if (filename.startsWith(this.HiddenFilesPrefix))
				continue;

			let mapPath = mapTypePath + filename;
			let mapData = this.mapCache.getMapData(mapTypeName, mapPath);

			// Map files may come with custom json files
			if (!mapData || !mapData.settings)
				continue;

			if (MatchesClassList(mapData.settings.Keywords || [], mapFilter.Match))
			{
				if (existence)
				{
					Engine.ProfileStop();
					return true;
				}

				maps.push({
					"file": mapPath,
					"name": translate(mapData.settings.Name),
					"description": translate(mapData.settings.Description)
				});
			}
		}

		Engine.ProfileStop();
		return existence ? false : maps;
	}
}

/**
 * When maps start with this prefix, they will not appear in the maplist.
 * Used for the Atlas _default.xml for instance.
 */
MapFilters.prototype.HiddenFilesPrefix = "_";

MapFilters.prototype.Filters = [
	{
		"Name": "default",
		"Title": translate("Default"),
		"Description": translate("All maps except naval and demo maps."),
		"Match": ["!naval !demo !hidden"]
	},
	{
		"Name": "naval",
		"Title": translate("Naval Maps"),
		"Description": translate("Maps where ships are needed to reach the enemy."),
		"Match": ["naval"]
	},
	{
		"Name": "demo",
		"Title": translate("Demo Maps"),
		"Description": translate("These maps are not playable but for demonstration purposes only."),
		"Match": ["demo"]
	},
	{
		"Name": "new",
		"Title": translate("New Maps"),
		"Description": translate("Maps that are brand new in this release of the game."),
		"Match": ["new"]
	},
	{
		"Name": "trigger",
		"Title": translate("Trigger Maps"),
		"Description": translate("Maps that come with scripted events and potentially spawn enemy units."),
		"Match": ["trigger"]
	},
	{
		"Name": "all",
		"Title": translate("All Maps"),
		"Description": translate("Every map of the chosen maptype."),
		"Match": "!"
	}
];
