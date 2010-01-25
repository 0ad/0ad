var g_Selection = {}; // { id: 1, id: 1, ... } for each selected entity ID 'id'

var g_ActiveSelectionColour = { r:1, g:1, b:1, a:1 };
var g_InactiveSelectionColour = { r:0, g:0, b:0, a:0 };

function setHighlight(ent, colour)
{
	Engine.GuiInterfaceCall("SetSelectionHighlight", { "entity":ent, "colour":colour });
}

function toggleEntitySelection(ent)
{
	if (g_Selection[ent])
	{
		setHighlight(ent, g_InactiveSelectionColour);
		delete g_Selection[ent];
	}
	else
	{
		setHighlight(ent, g_ActiveSelectionColour);
		g_Selection[ent] = 1;
	}
}

function addEntitySelection(ents)
{
	for each (var ent in ents)
	{
		if (!g_Selection[ent])
		{
			setHighlight(ent, g_ActiveSelectionColour);
			g_Selection[ent] = 1;
		}
	}
}

function resetEntitySelection()
{
	for (var ent in g_Selection)
		setHighlight(ent, g_InactiveSelectionColour);

	g_Selection = {};
}

function getEntitySelection()
{
	var ents = [];
	for (var ent in g_Selection)
		ents.push(ent);
	return ents;
}
