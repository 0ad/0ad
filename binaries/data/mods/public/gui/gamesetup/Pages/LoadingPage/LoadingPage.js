/**
 * The purpose of this page is to display a placeholder in multiplayer until the settings from the server have been received.
 * This is not technically necessary, but only performed to avoid confusion or irritation when showing the clients first the
 * default settings and then switching to the server settings quickly thereafter.
 */
SetupWindowPages.LoadingPage = class
{
	constructor(setupWindow)
	{
		if (g_IsNetworked)
			setupWindow.controls.netMessages.registerNetMessageHandler("gamesetup", this.hideLoadingPage.bind(this));
		else
			this.hideLoadingPage();
	}

	hideLoadingPage()
	{
		let loadingPage = Engine.GetGUIObjectByName("loadingPage");
		if (loadingPage.hidden)
			return;

		loadingPage.hidden = true;
		Engine.GetGUIObjectByName("setupWindow").hidden = false;
	}
}
