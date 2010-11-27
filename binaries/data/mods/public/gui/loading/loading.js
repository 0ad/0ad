var g_Data;

function init(data)
{
	var mapName = "map";
	if (data && data.attribs)
		mapName = data.attribs.map;

	g_Data = data;
	
	// Set to "hourglass" cursor.
	setCursor("cursor-wait");

	// Get tip image and corresponding tip text
	var sprite = "";
	var tipText = "";
	var tipImageLoadingArray = buildDirEntList("art/textures/ui/loading/tips/", "*.png", false);
	var tipTextLoadingArray = buildDirEntList("gui/text/tips/", "*.txt", false);
	
	if (tipImageLoadingArray.length == 0)
		error("Failed to find any matching tip textures for the loading screen.");
	else if (tipTextLoadingArray.length == 0)
		error("Failed to find any matching tip text files for the loading screen.");
	else if (tipImageLoadingArray.length != tipTextLoadingArray.length)
		error("The there are different amounts of tip images and tip text files.");
	else
	{
		var randomIndex = getRandom (0, tipImageLoadingArray.length-1);

		sprite = "stretched:" + tipImageLoadingArray[randomIndex];
		sprite = sprite.replace("art/textures/ui/", "");
		sprite = sprite.replace(".cached.dds", ""); // cope with pre-cached textures
		
		tipText = readFile(tipTextLoadingArray[randomIndex]);
	}

	// Set tip image
	getGUIObjectByName("loadingTipImage").sprite = sprite;

	// Set tip text
	if (tipText)
	{
		var index = tipText.indexOf("\n");
		tipTextTitle = tipText.substring(0, index);
		tipTextMessage = tipText.substring(index);
		getGUIObjectByName("loadingTipTitle").caption = tipTextTitle? tipTextTitle : "";
		getGUIObjectByName("loadingTipText").caption = tipTextMessage? tipTextMessage : "";
	}

	// janwas: main loop now sets progress / description, but that won't
	// happen until the first timeslice completes, so set initial values.
	var loadingMapName = getGUIObjectByName ("loadingMapName");
	
	if (data)
	{
		switch (data.attribs.mapType)
		{
		case "scenario":
			//ldTitleBar.caption = "Loading Scenario";
			loadingMapName.caption = "Loading \"" + mapName + "\"";
			break;

		case "random":
			//ldTitleBar.caption = "Loading Random Map";
			loadingMapName.caption = "Generating \"" + mapName + "\"";
			break;

		default:
			error("Unkown map type: "+data.attribs.mapType);
		}
	}

	getGUIObjectByName("progressText").caption = "";
	getGUIObjectByName("progressBar").caption = 0;

	// Pick a random quote of the day (each line is a separate tip).
	var quoteArray = readFileLines("gui/text/quotes.txt");
	getGUIObjectByName("loadingQuoteText").caption = quoteArray[getRandom(0, quoteArray.length-1)];
}

// ====================================================================
function displayProgress()
{
	getGUIObjectByName("progressBar").caption = g_Progress; // display current progress
	getGUIObjectByName("progressText").caption = g_LoadDescription; // display current progess details
	
	// Keep curved right edge of progress bar in sync with the rest of the progress bar
	var middle = getGUIObjectByName("progressBar");
	var rightSide = getGUIObjectByName("loadingProgressbar_right");
	
	var middleLength = middle.size.right - middle.size.left;
	var increment = Math.round(g_Progress*middleLength/100);

	var size = rightSide.size;
	size.left = increment;
	size.right = increment+12;
	rightSide.size = size;
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
