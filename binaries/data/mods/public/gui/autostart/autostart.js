function init(initData)
{
	let settings = new GameSettings().init();
	settings.fromInitAttributes(initData);
	let assignments = {
		"local": {
			"player": 1,
			"name": Engine.ConfigDB_GetValue("user", "playername.singleplayer") || Engine.GetSystemUsername()
		}
	};
	settings.launchGame(assignments);

	Engine.SwitchGuiPage("page_loading.xml", {
		"attribs": settings.toInitAttributes(),
		"playerAssignments": assignments
	});
}
