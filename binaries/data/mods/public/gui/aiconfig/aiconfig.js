var g_AIs; // [ {"id": ..., "data": {"name": ..., "description": ..., ...} }, ... ]
var g_Callback; // for the OK button

function init(settings)
{
	g_Callback = settings.callback;

	g_AIs = [
		{id: "", data: {name: "None", description: "AI will be disabled for this player."}}
	].concat(settings.ais);

	var aiSelection = getGUIObjectByName("aiSelection");
	aiSelection.list = [ ai.data.name for each (ai in g_AIs) ];

	var selected = 0;
	for (var i = 0; i < g_AIs.length; ++i)
	{
		if (g_AIs[i].id == settings.id)
		{
			selected = i;
			break;
		}
	}
	aiSelection.selected = selected;
}

function selectAI(idx)
{
	var id = g_AIs[idx].id;
	var name = g_AIs[idx].data.name;
	var description = g_AIs[idx].data.description;

	getGUIObjectByName("aiDescription").caption = description;
}

function returnAI()
{
	var aiSelection = getGUIObjectByName("aiSelection");
	var idx = aiSelection.selected;
	var id = g_AIs[idx].id;
	var name = g_AIs[idx].data.name;

	// Pop the page before calling the callback, so the callback runs
	// in the parent GUI page's context
	Engine.PopGuiPage();

	g_Callback({"id": id, "name": name});
}
