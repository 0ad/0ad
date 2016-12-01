var g_Campaign = null;

function init(data)
{
	// TODO: load up existing campaign in the dropdown above
	Engine.GetGUIObjectByName("saveGameDesc").caption = data.campaign;
	g_Campaign = data.campaign;
}

function startCampaign()
{
	// TODO: handle overwrite
	realStartCampaign(Engine.GetGUIObjectByName("saveGameDesc").caption, g_Campaign);
}

function realStartCampaign(name, campaign)
{
	Engine.SaveCampaign(name, {"name" : name, "campaign" : campaign});
}