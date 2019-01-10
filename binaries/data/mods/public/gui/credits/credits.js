/**
 * Order in which the tabs should show up.
 */
var g_OrderTabNames = ["special", "programming", "art", "translators", "misc", "donators"];

/**
 * Array of Objects containg all relevant data per tab.
 */
var g_PanelData = [];

/**
 * Vertical size of a tab button.
 */
var g_TabButtonHeight = 30;

/**
 * Vertical space between two tab buttons.
 */
var g_TabButtonDist = 5;

function init()
{
	// Load credits list from the disk and parse them
	for (let category of g_OrderTabNames)
	{
		let json = Engine.ReadJSONFile("gui/credits/texts/" + category + ".json");
		if (!json || !json.Content)
		{
			error("Could not load credits for " + category + "!");
			continue;
		}
		translateObjectKeys(json, ["Title", "Subtitle"]);
		g_PanelData.push({
			"label": json.Title || category,
			"content": parseHelper(json.Content)
		});
	}

	placeTabButtons(
		g_PanelData,
		g_TabButtonHeight,
		g_TabButtonDist,
		selectPanel,
		category => {
			Engine.GetGUIObjectByName("creditsText").caption = g_PanelData[category].content;
		});
}

// Run through a "Content" list and parse elements for formatting and translation
function parseHelper(list)
{
	let result = "";

	for (let object of list)
	{
		if (object.LangName)
			result += "[font=\"sans-bold-stroke-14\"]" + object.LangName + "\n";

		if (object.Title)
			result += "[font=\"sans-bold-stroke-14\"]" + object.Title + "\n";

		if (object.Subtitle)
			result += "[font=\"sans-bold-14\"]" + object.Subtitle + "\n";

		if (object.List)
		{
			for (let element of object.List)
			{
				if (element.nick && element.name)
					result += "[font=\"sans-14\"]" + sprintf(translate("%(nick)s - %(name)s"), { "nick": element.nick, "name": element.name }) + "\n";
				else if (element.nick)
					result += "[font=\"sans-14\"]" + element.nick + "\n";
				else if (element.name)
					result += "[font=\"sans-14\"]" + element.name + "\n";
			}

			result += "\n";
		}

		if (object.Content)
			result += "\n" + parseHelper(object.Content) + "\n";
	}

	return result;
}
