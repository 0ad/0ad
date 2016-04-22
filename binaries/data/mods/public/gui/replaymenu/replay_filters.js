/**
 * Allow to filter replays by duration in 15min / 30min intervals.
 */
const g_DurationFilterIntervals = [
	{ "min":  -1, "max":  -1 },
	{ "min":  -1, "max":  15 },
	{ "min":  15, "max":  30 },
	{ "min":  30, "max":  45 },
	{ "min":  45, "max":  60 },
	{ "min":  60, "max":  90 },
	{ "min":  90, "max": 120 },
	{ "min": 120, "max":  -1 }
];

/**
 * Allow to filter by population capacity.
 */
const g_PopulationCapacities = prepareForDropdown(g_Settings && g_Settings.PopulationCapacities);

/**
 * Reloads the selectable values in the filters. The filters depend on g_Settings and g_Replays
 * (including its derivatives g_MapSizes, g_MapNames).
 */
function initFilters(filters)
{
	Engine.GetGUIObjectByName("compabilityFilter").checked = !filters || filters.compatibility;

	if (filters && filters.playernames)
		Engine.GetGUIObjectByName("playersFilter").caption = filters.playernames;

	initDateFilter(filters && filters.date);
	initMapSizeFilter(filters && filters.mapSize);
	initMapNameFilter(filters && filters.mapName);
	initPopCapFilter(filters && filters.popCap);
	initDurationFilter(filters && filters.duration);
}

/**
 * Allow to filter by month. Uses g_Replays.
 */
function initDateFilter(date)
{
	var months = g_Replays.map(replay => getReplayMonth(replay));
	months = months.filter((month, index) => months.indexOf(month) == index).sort();

	var dateTimeFilter = Engine.GetGUIObjectByName("dateTimeFilter");
	dateTimeFilter.list = [translateWithContext("datetime", "Any")].concat(months);
	dateTimeFilter.list_data = [""].concat(months);

	if (date)
		dateTimeFilter.selected = dateTimeFilter.list_data.indexOf(date);

	if (dateTimeFilter.selected == -1 || dateTimeFilter.selected >= dateTimeFilter.list.length)
		dateTimeFilter.selected = 0;
}

/**
 * Allow to filter by mapsize. Uses g_MapSizes.
 */
function initMapSizeFilter(mapSize)
{
	var mapSizeFilter = Engine.GetGUIObjectByName("mapSizeFilter");
	mapSizeFilter.list = [translateWithContext("map size", "Any")].concat(g_MapSizes.Name);
	mapSizeFilter.list_data = [-1].concat(g_MapSizes.Tiles);

	if (mapSize)
		mapSizeFilter.selected = mapSizeFilter.list_data.indexOf(mapSize);

	if (mapSizeFilter.selected == -1 || mapSizeFilter.selected >= mapSizeFilter.list.length)
		mapSizeFilter.selected = 0;
}

/**
 * Allow to filter by mapname. Uses g_MapNames.
 */
function initMapNameFilter(mapName)
{
	var mapNameFilter = Engine.GetGUIObjectByName("mapNameFilter");
	mapNameFilter.list = [translateWithContext("map name", "Any")].concat(g_MapNames.map(mapName => translate(mapName)));
	mapNameFilter.list_data = [""].concat(g_MapNames);

	if (mapName)
		mapNameFilter.selected = mapNameFilter.list_data.indexOf(mapName);

	if (mapNameFilter.selected == -1 || mapNameFilter.selected >= mapNameFilter.list.length)
		mapNameFilter.selected = 0;
}

/**
 * Allow to filter by population capacity.
 */
function initPopCapFilter(popCap)
{
	var populationFilter = Engine.GetGUIObjectByName("populationFilter");
	populationFilter.list = [translateWithContext("population capacity", "Any")].concat(g_PopulationCapacities.Title);
	populationFilter.list_data = [""].concat(g_PopulationCapacities.Population);

	if (popCap)
		populationFilter.selected = populationFilter.list_data.indexOf(popCap);

	if (populationFilter.selected == -1 || populationFilter.selected >= populationFilter.list.length)
		populationFilter.selected = 0;
}

/**
 * Allow to filter by game duration. Uses g_DurationFilterIntervals.
 */
function initDurationFilter(duration)
{
	var durationFilter = Engine.GetGUIObjectByName("durationFilter");
	durationFilter.list = g_DurationFilterIntervals.map((interval, index) => {

		if (index == 0)
			return translateWithContext("duration", "Any");

		if (index == 1)
			// Translation: Shorter duration than max minutes.
			return sprintf(translatePluralWithContext("duration filter", "< %(max)s min", "< %(max)s min", interval.max), interval);

		if (index == g_DurationFilterIntervals.length - 1)
			// Translation: Longer duration than min minutes.
			return sprintf(translatePluralWithContext("duration filter", "> %(min)s min", "> %(min)s min", interval.min), interval);

		// Translation: Duration between min and max minutes.
		return sprintf(translateWithContext("duration filter", "%(min)s - %(max)s min"), interval);
	});
	durationFilter.list_data = g_DurationFilterIntervals.map((interval, index) => index);

	if (duration)
		durationFilter.selected = durationFilter.list_data.indexOf(duration);

	if (durationFilter.selected == -1 || durationFilter.selected >= g_DurationFilterIntervals.length)
		durationFilter.selected = 0;
}

/**
 * Initializes g_ReplaysFiltered with replays that are not filtered out and sort it.
 */
function filterReplays()
{
	const sortKey = Engine.GetGUIObjectByName("replaySelection").selected_column;
	const sortOrder = Engine.GetGUIObjectByName("replaySelection").selected_column_order;

	g_ReplaysFiltered = g_Replays.filter(replay => filterReplay(replay)).sort((a, b) =>
	{
		let cmpA, cmpB;
		switch (sortKey)
		{
		case 'name':
			cmpA = +a.timestamp;
			cmpB = +b.timestamp;
			break;
		case 'duration':
			cmpA = +a.duration;
			cmpB = +b.duration;
			break;
		case 'players':
			cmpA = +a.attribs.settings.PlayerData.length;
			cmpB = +b.attribs.settings.PlayerData.length;
			break;
		case 'mapName':
			cmpA = getReplayMapName(a);
			cmpB = getReplayMapName(b);
			break;
		case 'mapSize':
			cmpA = +a.attribs.settings.Size;
			cmpB = +b.attribs.settings.Size;
			break;
		case 'popCapacity':
			cmpA = +a.attribs.settings.PopulationCap;
			cmpB = +b.attribs.settings.PopulationCap;
			break;
		}

		if (cmpA < cmpB)
			return -sortOrder;
		else if (cmpA > cmpB)
			return +sortOrder;

		return 0;
	});
}

/**
 * Decides whether the replay should be listed.
 *
 * @returns {bool} - true if replay should be visible
 */
function filterReplay(replay)
{
	// Check for compability first (most likely to filter)
	var compabilityFilter = Engine.GetGUIObjectByName("compabilityFilter");
	if (compabilityFilter.checked && !isReplayCompatible(replay))
		return false;

	// Filter date/time (select a month)
	var dateTimeFilter = Engine.GetGUIObjectByName("dateTimeFilter");
	if (dateTimeFilter.selected > 0 && getReplayMonth(replay) != dateTimeFilter.list_data[dateTimeFilter.selected])
		return false;

	// Filter by playernames
	var playersFilter = Engine.GetGUIObjectByName("playersFilter");
	var keywords = playersFilter.caption.toLowerCase().split(" ");
	if (keywords.length)
	{
		// We just check if all typed words are somewhere in the playerlist of that replay
		let playerList = replay.attribs.settings.PlayerData.map(player => player ? player.Name : "").join(" ").toLowerCase();
		if (!keywords.every(keyword => playerList.indexOf(keyword) != -1))
			return false;
	}

	// Filter by map name
	var mapNameFilter = Engine.GetGUIObjectByName("mapNameFilter");
	if (mapNameFilter.selected > 0 && getReplayMapName(replay) != mapNameFilter.list_data[mapNameFilter.selected])
		return false;

	// Filter by map size
	var mapSizeFilter = Engine.GetGUIObjectByName("mapSizeFilter");
	if (mapSizeFilter.selected > 0 && replay.attribs.settings.Size != mapSizeFilter.list_data[mapSizeFilter.selected])
		return false;

	// Filter by population capacity
	var populationFilter = Engine.GetGUIObjectByName("populationFilter");
	if (populationFilter.selected > 0 && replay.attribs.settings.PopulationCap != populationFilter.list_data[populationFilter.selected])
		return false;

	// Filter by game duration
	var durationFilter = Engine.GetGUIObjectByName("durationFilter");
	if (durationFilter.selected > 0)
	{
		let interval = g_DurationFilterIntervals[durationFilter.selected];

		if ((interval.min > -1 && replay.duration < interval.min * 60) ||
			(interval.max > -1 && replay.duration > interval.max * 60))
			return false;
	}

	return true;
}
