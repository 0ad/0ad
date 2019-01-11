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
			result += setStringTags(object.LangName + "\n", { "font": "sans-bold-stroke-14" });

		if (object.Title)
			result += setStringTags(object.Title + "\n", { "font": "sans-bold-stroke-14" });

		if (object.Subtitle)
			result += setStringTags(object.Subtitle + "\n", { "font": "sans-bold-14" });

		if (object.List)
		{
			for (let element of object.List)
			{
				let credit;
				if (element.nick && element.name)
					credit = sprintf(translate("%(nick)s - %(name)s"), { "nick": element.nick, "name": element.name });
				else if (element.nick)
					credit = element.nick;
				else if (element.name)
					credit = element.name;

				if (credit)
					result += setStringTags(credit + "\n", { "font": "sans-14" });
			}

			result += "\n";
		}

		if (object.Content)
			result += "\n" + parseHelper(object.Content) + "\n";
	}

	return result;
}
