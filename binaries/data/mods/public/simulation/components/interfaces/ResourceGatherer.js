Engine.RegisterInterface("ResourceGatherer");

/**
 * Message of the form { "to": [{ "type": string, "amount": number, "max":number }] }
 * sent from ResourceGatherer component whenever the amount of carried resources changes.
 */
Engine.RegisterMessageType("ResourceCarryingChanged");
