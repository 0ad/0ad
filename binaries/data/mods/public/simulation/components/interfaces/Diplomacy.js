Engine.RegisterInterface("Diplomacy");

/**
 * Message of the form { "player": number, "otherPlayer": number }
 * sent from Diplomacy component when diplomacy changed for one player or between two players.
 */
Engine.RegisterMessageType("DiplomacyChanged");
