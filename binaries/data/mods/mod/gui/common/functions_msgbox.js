// We want to pass callback functions for the different buttons in a convenient way.
// Because passing functions accross compartment boundaries is a pain, we just store them here together with some optional arguments.
// The messageBox page will return the code of the pressed button and the according function will be called.
var g_MessageBoxBtnFunctions = [];
var g_MessageBoxCallbackArgs = [];

function messageBoxCallbackFunction(btnCode)
{
	if (btnCode !== undefined && g_MessageBoxBtnFunctions[btnCode])
	{
		// Cache the variables to make it possible to call a messageBox from a callback function.
		let callbackFunction = g_MessageBoxBtnFunctions[btnCode];
		let callbackArgs = g_MessageBoxCallbackArgs[btnCode];

		g_MessageBoxBtnFunctions = [];
		g_MessageBoxCallbackArgs = [];

		if (callbackArgs !== undefined)
			callbackFunction(callbackArgs);
		else
			callbackFunction();
		return;
	}

	g_MessageBoxBtnFunctions = [];
	g_MessageBoxCallbackArgs = [];
}

function messageBox(mbWidth, mbHeight, mbMessage, mbTitle, mbButtonCaptions, mbBtnCode, mbCallbackArgs)
{
	if (g_MessageBoxBtnFunctions && g_MessageBoxBtnFunctions.length)
	{
		warn("A messagebox was called when a previous callback function is still set, aborting!");
		return;
	}

	g_MessageBoxBtnFunctions = mbBtnCode;
	g_MessageBoxCallbackArgs = mbCallbackArgs || g_MessageBoxCallbackArgs;

	Engine.PushGuiPage("page_msgbox.xml", {
		"width": mbWidth,
		"height": mbHeight,
		"message": mbMessage,
		"title": mbTitle,
		"buttonCaptions": mbButtonCaptions,
		"callback": mbBtnCode && "messageBoxCallbackFunction"
	});
}

function openURL(url)
{
	Engine.OpenURL(url);

	messageBox(
		600, 200,
		sprintf(
			translate("Opening %(url)s\n in default web browser. Please wait…"),
			{ "url": url }
		),
		translate("Opening page"));
}
