function getDisconnectReason(id)
{
	// Must be kept in sync with source/network/NetHost.h
	switch (id)
	{
	case 0: return translate("Unknown reason");
	case 1: return translate("Unexpected shutdown");
	case 2: return translate("Incorrect network protocol version");
	case 3: return translate("Game has already started");
	default: return sprintf(translate("[Invalid value %(id)s]"), { id: id });
	}
}

function reportDisconnect(reason)
{
	var reasontext = getDisconnectReason(reason);

	messageBox(400, 200,
		translate("Lost connection to the server.") + "\n\n" + sprintf(translate("Reason: %(reason)s."), { reason: reasontext }),
		translate("Disconnected"), 2);
}
