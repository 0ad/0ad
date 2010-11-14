var g_Data;

function init(data)
{
	var mapName = "map";
	if (data && data.attribs)
		mapName = data.attribs.map;

	g_Data = data;
	
	// Set to "hourglass" cursor.
	setCursor("cursor-wait");

	// Choose random concept art for loading screen background (should depend on the selected civ later when this is specified).
	var sprite = "";
	var loadingBkgArray = buildDirEntList("art/textures/ui/loading/", "*.dds", false);
	if (loadingBkgArray.length == 0)
		error("Failed to find any matching textures for the loading screen background.");
	else
	{
		// Get a random index from the list of loading screen backgrounds.
		sprite = "stretched:" + loadingBkgArray[getRandom (0, loadingBkgArray.length-1)];
		sprite = sprite.replace("art/textures/ui/", "");
		sprite = sprite.replace(".cached.dds", ""); // cope with pre-cached textures
	}
	getGUIObjectByName ("ldConcept").sprite = sprite;

	// janwas: main loop now sets progress / description, but that won't
	// happen until the first timeslice completes, so set initial values.
	
	var ldTitleBar = getGUIObjectByName ("ldTitleBar");
	var ldText = getGUIObjectByName ("ldText");
	
	if (data)
	{
		switch (data.attribs.mapType)
		{
		case "scenario":
			ldTitleBar.caption = "Loading Scenario";
			ldText.caption = "Loading " + mapName + "\nPlease wait...";
			break;

		case "random":
			ldTitleBar.caption = "Loading Random Map";
			ldText.caption = "Generating " + mapName + "\nPlease wait...";
			break;

		default:
			error("Unkown map type: "+data.attribs.mapType);
		}
	}

	getGUIObjectByName("ldProgressBarText").caption = "";
	getGUIObjectByName("ldProgressBar").caption = 0;

	// Pick a random tip of the day (each line is a separate tip).
	var tipArray = readFileLines("gui/text/tips.txt");
	// Set tip string.
	getGUIObjectByName("ldTip").caption = tipArray[getRandom(0, tipArray.length-1)];
}

// ====================================================================

function reallyStartGame()
{
	// Stop the music
	if (global.curr_music)
		global.curr_music.fade(-1, 0.0, 5.0); // fade to 0 over 5 seconds

	// This is a reserved function name that is executed by the engine when it is ready
	// to start the game (i.e. loading progress has reached 100%).

	// Switch GUI from loading screen to game session.
	Engine.SwitchGuiPage("page_session.xml", g_Data);
	
	// Restore default cursor.
	setCursor("arrow-default");
}
