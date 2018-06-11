var g_LobbyMessages = {
	"error": message => {
		setFeedback(message.text ||
			translate("Unknown error. This usually occurs because the same IP address is not allowed to register more than one account within one hour."));
		Engine.StopXmppClient();
	},
	"disconnected": message => {
		setFeedback(message.reason);
		Engine.StopXmppClient();
	}
};

function onTick()
{
	while (true)
	{
		let message = Engine.LobbyGuiPollNewMessage();
		if (!message)
			break;

		if (message.type == "system" && message.level)
			g_LobbyMessages[message.level](message);
		else
			warn("Unknown prelobby message: " + uneval(message));
	}
}

function setFeedback(feedbackText)
{
	Engine.GetGUIObjectByName("feedback").caption = feedbackText;
	Engine.GetGUIObjectByName("continue").enabled = !feedbackText;
}

function cancelButton()
{
	if (Engine.HasXmppClient())
		Engine.StopXmppClient();
	Engine.PopGuiPage();
}
