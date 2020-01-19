/**
 * This class enables other classes to subscribe to specific CNetMessage types (see NetMessage.h, NetMessages.h) sent by the CNetServer.
 */
class NetMessages
{
	constructor(setupWindow)
	{
		this.netMessageHandlers = {};

		for (let messageType of this.MessageTypes)
			this.netMessageHandlers[messageType] = new Set();

		this.registerNetMessageHandler("netwarn", addNetworkWarning);

		Engine.GetGUIObjectByName("netMessages").onTick = this.onTick.bind(this);
		setupWindow.registerClosePageHandler(this.onClosePage.bind(this));
	}

	registerNetMessageHandler(messageType, handler)
	{
		if (this.netMessageHandlers[messageType])
			this.netMessageHandlers[messageType].add(handler);
		else
			error("Unknown net message type: " + uneval(messageType));
	}

	unregisterNetMessageHandler(messageType, handler)
	{
		if (this.netMessageHandlers[messageType])
			this.netMessageHandlers[messageType].delete(handler);
		else
			error("Unknown net message type: " + uneval(messageType));
	}

	onTick()
	{
		while (true)
		{
			let message = Engine.PollNetworkClient();
			if (!message)
				break;

			log("Net message: " + uneval(message));

			if (this.netMessageHandlers[message.type])
				for (let handler of this.netMessageHandlers[message.type])
					handler(message);
			else
				error("Unrecognized net message type " + message.type);
		}
	}

	onClosePage()
	{
		Engine.DisconnectNetworkGame();
	}
}

/**
 * Messages types are present here if and only if they are sent by NetClient.cpp.
 */
NetMessages.prototype.MessageTypes = [
	"chat",
	"ready",
	"gamesetup",
	"kicked",
	"netstatus",
	"netwarn",
	"players",
	"start"
];
