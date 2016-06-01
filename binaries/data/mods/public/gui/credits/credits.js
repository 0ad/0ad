var g_PanelNames = ["special", "programming", "art", "translators", "misc", "donators"];
var g_ButtonNames = {};
var g_PanelTexts = {};
var g_ActivePanel = -1;

function init()
{
	// Load credits list from the disk and parse them
	for (let name of g_PanelNames)
	{
		let json = Engine.ReadJSONFile("gui/credits/texts/" + name + ".json");
		if (!json || !json.Content)
		{
			error("Could not load credits for " + name + "!");
			continue;
		}
		g_ButtonNames[name] = json.Title || name;
		g_PanelTexts[name] = parseHelper(json.Content);
	}

	placeButtons();
	selectPanel(0);
}

function placeButtons()
{
	const numButtons = 20;
	if (g_PanelNames.length > numButtons)
		warn("Could not display some credits.");

	for (let i = 0; i < numButtons; ++i)
	{
		let button = Engine.GetGUIObjectByName("creditsPanelButton[" + i + "]");
		if (i >= g_PanelNames.length)
		{
			button.hidden = true;
			continue;
		}
		let size = button.size;
		size.top = i * 35;
		size.bottom = size.top + 30;
		button.size = size;

		button.onPress = (i => function() {selectPanel(i);})(i);
		let buttonText = Engine.GetGUIObjectByName("creditsPanelButtonText[" + i + "]");
		buttonText.caption = translate(g_ButtonNames[g_PanelNames[i]]);
	}
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

function selectPanel(i)
{
	if (g_ActivePanel != -1)
	{
		let oldPanelButton = Engine.GetGUIObjectByName("creditsPanelButton[" + g_ActivePanel+ "]");
		oldPanelButton.sprite = "BackgroundBox";
	}

	g_ActivePanel = i;
	Engine.GetGUIObjectByName("creditsPanelButton[" + i + "]").sprite = "ForegroundBox";
	Engine.GetGUIObjectByName("creditsText").caption = g_PanelTexts[g_PanelNames[i]];
}
