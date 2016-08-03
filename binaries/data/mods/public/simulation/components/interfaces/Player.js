/**
 * Message of the form { "player": number, "from": string, "to": string }
 * sent from Player component to warn other components when a player changed civs.
 * This should only happen in Atlas
 */
Engine.RegisterMessageType("CivChanged");
