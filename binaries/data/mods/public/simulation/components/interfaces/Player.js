/**
 * Message of the form { "player": number, "from": string, "to": string }
 * sent from Player component to warn other components when a player changed civs.
 * This should only happen in Atlas.
 */
Engine.RegisterMessageType("CivChanged");

/**
 * Message of the form { "player": number, "otherPlayer": number }
 * sent from Player component when diplomacy changed for one player or between two players.
 */
Engine.RegisterMessageType("DiplomacyChanged");

/**
 * Message of the form {}
 * sent from Player component.
 */
Engine.RegisterMessageType("DisabledTechnologiesChanged");

/**
 * Message of the form {}
 * sent from Player component.
 */
Engine.RegisterMessageType("DisabledTemplatesChanged");

/**
 * Message of the form { "playerID": number }
 * sent from Player component when a player is defeated.
 */
Engine.RegisterMessageType("PlayerDefeated");

/**
 * Message of the form { "playerID": number }
 * sent from Player component when a player has won.
 */
Engine.RegisterMessageType("PlayerWon");

/**
 * Message of the form { "to": number, "from": number, "amounts": object }
 * sent from Player component whenever a tribute is sent.
 */
Engine.RegisterMessageType("TributeExchanged");

/**
 * Message of the form { "player": player, "type": "cheat" }
 * sent from Player when some multiplier of that player has changed
 */
Engine.RegisterMessageType("MultiplierChanged");
