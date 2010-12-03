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
	var tipTextLoadingArray = buildDirEntList("gui/text/tips/", "*.txt", false);
	var tipTextFilePath = tipTextLoadingArray[getRandom (0, tipTextLoadingArray.length-1)];
	var fileName = tipTextFilePath.substring(tipTextFilePath.lastIndexOf("/")+1).replace(".txt", ".png");
	var tipImageFilePath = "art/textures/ui/loading/tips/" + fileName;

	if (tipTextLoadingArray.length > 0)
	{
		// Set tip text
		var tipText = readFile(tipTextFilePath);
		if (tipText)
		{
			var index = tipText.indexOf("\n");
			tipTextTitle = tipText.substring(0, index);
			tipTextMessage = tipText.substring(index);
			getGUIObjectByName("tipTitle").caption = tipTextTitle? tipTextTitle : "";
			getGUIObjectByName("tipText").caption = tipTextMessage? tipTextMessage : "";
		}

		// Set tip image
		var sprite = "stretched:" + tipImageFilePath;
		sprite = sprite.replace("art/textures/ui/", "");
		sprite = sprite.replace(".cached.dds", ""); // cope with pre-cached textures
		getGUIObjectByName("tipImage").sprite = sprite? sprite : "";
	}
	else
	{
		error("Failed to find any matching tip textures for the loading screen.")
	}

	// janwas: main loop now sets progress / description, but that won't
	// happen until the first timeslice completes, so set initial values.
	var loadingMapName = getGUIObjectByName ("loadingMapName");
	
	if (data)
	{
		switch (data.attribs.mapType)
		{
		case "scenario":
			loadingMapName.caption = "Loading \"" + mapName + "\"";
			break;

		case "random":
			loadingMapName.caption = "Generating \"" + mapName + "\"";
			break;

		default:
			error("Unkown map type: " + data.attribs.mapType);
		}
	}

	getGUIObjectByName("progressText").caption = "";
	getGUIObjectByName("progressbar").caption = 0;

	// Pick a random quote of the day (each line is a separate tip).
	var quoteArray = readFileLines("gui/text/quotes.txt");
	getGUIObjectByName("quoteText").caption = quoteArray[getRandom(0, quoteArray.length-1)];
}

// ====================================================================
function displayProgress()
{
	getGUIObjectByName("progressbar").caption = g_Progress; // display current progress
	getGUIObjectByName("progressText").caption = g_LoadDescription; // display current progess details
	
	// Keep curved right edge of progress bar in sync with the rest of the progress bar
	var middle = getGUIObjectByName("progressbar");
	var rightSide = getGUIObjectByName("progressbar_right");
	
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
