var g_PanelNames = ["special", "programming", "art", "translators", "misc", "donators"];
var g_PanelTexts = [];
var g_ActivePanel = -1;

function init()
{
	// Load credits list from the disk and parse them
	for (let name of g_PanelNames)
		g_PanelTexts.push(parseJSONCredits(name));

	selectPanel(0);
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
			result += "[font=\"sans-bold-stroke-14\"]" + translate(object.Title) + "\n";

		if (object.Subtitle)
			result += "[font=\"sans-bold-14\"]" + translate(object.Subtitle) + "\n";

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

function parseJSONCredits(panelName)
{
	let json = Engine.ReadJSONFile("gui/credits/texts/" + panelName + ".json");
	if (!json || !json.Content)
	{
		error("Could not load credits for " + panelName + "!");
		return "";
	}

	return parseHelper(json.Content);
}

function selectPanel(i)
{
	if (g_ActivePanel != -1)
	{
		let oldPanelButton = Engine.GetGUIObjectByName(g_PanelNames[g_ActivePanel] + "PanelButton");
		oldPanelButton.sprite = "BackgroundBox";
	}

	g_ActivePanel = i;
	let newPanelButton = Engine.GetGUIObjectByName(g_PanelNames[g_ActivePanel] + "PanelButton");
	newPanelButton.sprite = "ForegroundBox";

	let creditsText = Engine.GetGUIObjectByName("creditsText");
	creditsText.caption = g_PanelTexts[i];
}
