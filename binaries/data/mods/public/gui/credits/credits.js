var g_PanelNames = ["special", "programming", "art", "translators", "misc", "donators"];
var g_ButtonNames = {};
var g_PanelTexts = {};

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

	for (let i = 0; i < g_PanelNames.length; ++i)
	{
		let button = Engine.GetGUIObjectByName("creditsPanelButton[" + i + "]");
		if (!button)
		{
			warn("Could not display some credits.");
			break;
		}
		button.hidden = false;
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
	Engine.GetGUIObjectByName("creditsPanelButtons").children.forEach((button, j) => {
		button.sprite = i == j ? "ModernTabVerticalForeground" : "ModernTabVerticalBackground";
	});

	Engine.GetGUIObjectByName("creditsText").caption = g_PanelTexts[g_PanelNames[i]];
}
