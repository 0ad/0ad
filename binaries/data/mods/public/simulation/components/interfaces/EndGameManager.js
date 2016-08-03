Engine.RegisterInterface("EndGameManager");

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
 * Message of the form {}
 * sent from EndGameManager component.
 */
Engine.RegisterMessageType("GameTypeChanged");
