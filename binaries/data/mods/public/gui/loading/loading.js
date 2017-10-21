var g_Data;
var g_EndPieceWidth = 16;

function init(data)
{
	g_Data = data;

	Engine.SetCursor("cursor-wait");

	// Get tip image and corresponding tip text
	let tipTextLoadingArray = Engine.BuildDirEntList("gui/text/tips/", "*.txt", false);

	if (tipTextLoadingArray.length > 0)
	{
		// Set tip text
		let tipTextFilePath = pickRandom(tipTextLoadingArray);
		let tipText = Engine.TranslateLines(Engine.ReadFile(tipTextFilePath));

		if (tipText)
		{
			let index = tipText.indexOf("\n");
			let tipTextTitle = tipText.substring(0, index);
			let tipTextMessage = tipText.substring(index);
			Engine.GetGUIObjectByName("tipTitle").caption = tipTextTitle ? tipTextTitle : "";
			Engine.GetGUIObjectByName("tipText").caption = tipTextMessage ? tipTextMessage : "";
		}

		// Set tip image
		let fileName = tipTextFilePath.substring(tipTextFilePath.lastIndexOf("/") + 1).replace(".txt", ".png");
		let tipImageFilePath = "loading/tips/" + fileName;
		let sprite = "stretched:" + tipImageFilePath;
		Engine.GetGUIObjectByName("tipImage").sprite = sprite ? sprite : "";
	}
	else
		error("Failed to find any matching tips for the loading screen.");

	// janwas: main loop now sets progress / description, but that won't
	// happen until the first timeslice completes, so set initial values.
	let loadingMapName = Engine.GetGUIObjectByName("loadingMapName");

	if (data)
	{
		let mapName = translate(data.attribs.settings.Name);
		switch (data.attribs.mapType)
		{
		case "skirmish":
		case "scenario":
			loadingMapName.caption = sprintf(translate("Loading “%(map)s”"), { "map": mapName });
			break;

		case "random":
			loadingMapName.caption = sprintf(translate("Generating “%(map)s”"), { "map": mapName });
			break;

		default:
			error("Unknown map type: " + data.attribs.mapType);
		}
	}

	Engine.GetGUIObjectByName("progressText").caption = "";
	Engine.GetGUIObjectByName("progressbar").caption = 0;

	// Pick a random quote of the day (each line is a separate tip).
	let quoteArray = Engine.ReadFileLines("gui/text/quotes.txt").filter(line => line);
	Engine.GetGUIObjectByName("quoteText").caption = translate(pickRandom(quoteArray));
}

function displayProgress()
{
	// Make the progessbar finish a little early so that the user can actually see it finish
	if (g_Progress >= 100)
		return;

	// Show 100 when it is really 99
	let progress = g_Progress + 1;

	Engine.GetGUIObjectByName("progressbar").caption = progress; // display current progress
	Engine.GetGUIObjectByName("progressText").caption = progress + "%";

	// Displays detailed loading info rather than a percent
	// Engine.GetGUIObjectByName("progressText").caption = g_LoadDescription; // display current progess details

	// Keep curved right edge of progress bar in sync with the rest of the progress bar
	let middle = Engine.GetGUIObjectByName("progressbar");
	let rightSide = Engine.GetGUIObjectByName("progressbar_right");

	let middleLength = (middle.size.right - middle.size.left) - (g_EndPieceWidth / 2);
	let increment = Math.round(progress * middleLength / 100);

	let size = rightSide.size;
	size.left = increment;
	size.right = increment + g_EndPieceWidth;
	rightSide.size = size;
}

/**
 * This is a reserved function name that is executed by the engine when it is ready
 * to start the game (i.e. loading progress has reached 100%).
 */
function reallyStartGame()
{
	Engine.SwitchGuiPage("page_session.xml", g_Data);

	Engine.ResetCursor();
}
