/**
 * The objective of this class is to displays all active counters (messages showing the remaining time)
 * for wonder-victory, ceasefire etc.
 */
class TimeNotificationOverlay
{
	constructor(playerViewControl)
	{
		this.notificationText = Engine.GetGUIObjectByName("notificationText");
		
		registerSimulationUpdateHandler(this.rebuild.bind(this));
		playerViewControl.registerViewedPlayerChangeHandler(this.rebuild.bind(this));
	}

	rebuild()
	{
		let notifications = Engine.GuiInterfaceCall("GetTimeNotifications", g_ViewedPlayer);

		let notificationText = "";
		for (let notification of notifications)
		{
			let message = notification.message;
			if (notification.translateMessage)
				message = translate(message);

			let parameters = notification.parameters || {};
			if (notification.translateParameters)
				translateObjectKeys(parameters, notification.translateParameters);

			parameters.time = timeToString(notification.endTime - g_SimState.timeElapsed);

			colorizePlayernameParameters(parameters);

			notificationText += sprintf(message, parameters) + "\n";
		}

		this.notificationText.caption = notificationText;
	}
}
