var g_Data;
const END_PIECE_WIDTH = 16;

function init(data)
{
	g_Data = data;

	// Set to "hourglass" cursor.
	Engine.SetCursor("cursor-wait");

	// Get tip image and corresponding tip text
	var tipTextLoadingArray = Engine.BuildDirEntList("gui/text/tips/", "*.txt", false);

	if (tipTextLoadingArray.length > 0)
	{
		// Set tip text
		var tipTextFilePath = tipTextLoadingArray[getRandom (0, tipTextLoadingArray.length-1)];
		var tipText = Engine.TranslateLines(Engine.ReadFile(tipTextFilePath));

		if (tipText)
		{
			var index = tipText.indexOf("\n");
			var tipTextTitle = tipText.substring(0, index);
			var tipTextMessage = tipText.substring(index);
			Engine.GetGUIObjectByName("tipTitle").caption = tipTextTitle? tipTextTitle : "";
			Engine.GetGUIObjectByName("tipText").caption = tipTextMessage? tipTextMessage : "";
		}

		// Set tip image
		var fileName = tipTextFilePath.substring(tipTextFilePath.lastIndexOf("/")+1).replace(".txt", ".png");
		var tipImageFilePath = "loading/tips/" + fileName;
		var sprite = "stretched:" + tipImageFilePath;
		Engine.GetGUIObjectByName("tipImage").sprite = sprite? sprite : "";
	}
	else
	{
		error("Failed to find any matching tips for the loading screen.")
	}

	// janwas: main loop now sets progress / description, but that won't
	// happen until the first timeslice completes, so set initial values.
	var loadingMapName = Engine.GetGUIObjectByName ("loadingMapName");

	if (data)
	{
		var mapName = translate(data.attribs.settings.Name);
		switch (data.attribs.mapType)
		{
		case "skirmish":
		case "scenario":
			loadingMapName.caption = sprintf(translate("Loading \"%(map)s\""), {map: mapName});
			break;

		case "random":
			loadingMapName.caption = sprintf(translate("Generating \"%(map)s\""), {map: mapName});
			break;

		default:
			error(sprintf("Unknown map type: %(mapType)s", { mapType: data.attribs.mapType }));
		}
	}

	Engine.GetGUIObjectByName("progressText").caption = "";
	Engine.GetGUIObjectByName("progressbar").caption = 0;

	// Pick a random quote of the day (each line is a separate tip).
	var quoteArray = Engine.ReadFileLines("gui/text/quotes.txt");
	Engine.GetGUIObjectByName("quoteText").caption = translate(quoteArray[getRandom(0, quoteArray.length-1)]);
}

// ====================================================================
function displayProgress()
{
	// Make the progessbar finish a little early so that the user can actually see it finish
	if (g_Progress < 100)
	{
		// Show 100 when it is really 99
		var progress = g_Progress + 1;

		Engine.GetGUIObjectByName("progressbar").caption = progress; // display current progress
		Engine.GetGUIObjectByName("progressText").caption = progress + "%";

		// Displays detailed loading info rather than a percent
		// Engine.GetGUIObjectByName("progressText").caption = g_LoadDescription; // display current progess details

		// Keep curved right edge of progress bar in sync with the rest of the progress bar
		var middle = Engine.GetGUIObjectByName("progressbar");
		var rightSide = Engine.GetGUIObjectByName("progressbar_right");

		var middleLength = (middle.size.right - middle.size.left) - (END_PIECE_WIDTH / 2);
		var increment = Math.round(progress * middleLength / 100);

		var size = rightSide.size;
		size.left = increment;
		size.right = increment + END_PIECE_WIDTH;
		rightSide.size = size;
	}
}

// ====================================================================
function reallyStartGame()
{
	// Stop the music
//	if (global.curr_music)
//		global.curr_music.fade(-1, 0.0, 5.0); // fade to 0 over 5 seconds


	// This is a reserved function name that is executed by the engine when it is ready
	// to start the game (i.e. loading progress has reached 100%).

	// Switch GUI from loading screen to game session.
	Engine.SwitchGuiPage("page_session.xml", g_Data);

	// Restore default cursor.
	Engine.SetCursor("arrow-default");
}
