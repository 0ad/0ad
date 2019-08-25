/**
 * Message of the form { player": number, "from": number, "to": number }
 * sent from PlayerManager component to warn other components when a player changed entities.
 * This is also sent when the player gets created or destroyed.
 */
Engine.RegisterMessageType("PlayerEntityChanged");
