Engine.RegisterInterface("Garrisonable");

/**
 * Message of the form { "holderID": number }
 * sent from the Garrisonable component whenever the garrisoned state changes.
 */
Engine.RegisterMessageType("GarrisonedStateChanged");
