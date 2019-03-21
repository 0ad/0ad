/**
 * Path to a file containing quotes of historical figures.
 */
var g_QuotesFile = "gui/text/quotes.txt";

/**
 * Directory storing txt files containing the gameplay tips.
 */
var g_TipsTextPath = "gui/text/tips/";

/**
 * Directory storing the PNG images with filenames corresponding to the tip text files.
 */
var g_TipsImagePath = "loading/tips/";

var g_Data;
var g_EndPieceWidth = 16;

function init(data)
{
	g_Data = data;

	Engine.SetCursor("cursor-wait");

	let tipFile = pickRandom(listFiles(g_TipsTextPath, ".txt", false));

	if (tipFile)
	{
			let tipText = Engine.TranslateLines(Engine.ReadFile(g_TipsTextPath + tipFile + ".txt")).split("\n");
			Engine.GetGUIObjectByName("tipTitle").caption = tipText.shift();
			Engine.GetGUIObjectByName("tipText").caption = tipText.map(
				// Translation: A bullet point used before every item of list of tips displayed on loading screen
				text => text && sprintf(translate("• %(tiptext)s"), { "tiptext": text })
			).join("\n\n");
			Engine.GetGUIObjectByName("tipImage").sprite = "stretched:" + g_TipsImagePath + tipFile + ".png";
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
		}
	}

	Engine.GetGUIObjectByName("progressText").caption = "";
	Engine.GetGUIObjectByName("progressbar").caption = 0;

	Engine.GetGUIObjectByName("quoteText").caption = translate(pickRandom(Engine.ReadFileLines(g_QuotesFile).filter(line => line)));
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
