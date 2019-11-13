var g_LoadingPage;

function init(data)
{
	g_LoadingPage = {
		"initData": data,
		"progressBar": new ProgressBar(),
		"quoteDisplay": new QuoteDisplay(),
		"tipDisplay": new TipDisplay(),
		"titleDisplay": new TitleDisplay(data)
	};

	Engine.SetCursor("cursor-wait");
}

/**
 * This is a reserved function name that is executed by the engine when it is ready
 * to start the game (i.e. loading progress has reached 100%).
 */
function reallyStartGame()
{
	Engine.SwitchGuiPage("page_session.xml", g_LoadingPage.initData);
	Engine.ResetCursor();
}
