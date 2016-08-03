Engine.RegisterInterface("BattleDetection");

/**
 * Message of the form { "player": number, "to": string }
 * sent from BattleDetection component whenever the battle state changes.
 */
Engine.RegisterMessageType("BattleStateChanged");
