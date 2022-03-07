function init(initData)
{
	let settings = new GameSettings().init();
	settings.fromInitAttributes(initData.attribs);

	settings.launchGame(initData.playerAssignments, initData.storeReplay);

	Engine.SwitchGuiPage("page_loading.xml", {
		"attribs": settings.finalizedAttributes,
		"playerAssignments": initData.playerAssignments
	});
}
