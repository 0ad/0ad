function getDisconnectReason(id)
{
	// Must be kept in sync with source/network/NetHost.h
	switch (id)
	{
	case 0: return "Unknown reason";
	case 1: return "Unexpected shutdown";
	case 2: return "Incorrect network protocol version";
	case 3: return "Game has already started";
	default: return "[Invalid value "+id+"]";
	}
}

function reportDisconnect(reason)
{
	var reasontext = (typeof reason == 'number' ? getDisconnectReason(reason) : reason);

	messageBox(400, 200,
		"Lost connection to the server.\n\nReason: " + reasontext + ".",
		"Disconnected", 2);
}
