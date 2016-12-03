function exitCampaignMode(exitGame = false)
{
	// TODO: should this be here?
	saveCampaign();

	if (exitGame)
	{
		messageBox(
		400, 200,
		translate("Are you sure you want to quit 0 A.D.?"),
		translate("Confirmation"),
		[translate("No"), translate("Yes")],
		[null, Engine.Exit]
		);
		return;
	}
	Engine.SwitchGuiPage("page_pregame.xml", {});
}
