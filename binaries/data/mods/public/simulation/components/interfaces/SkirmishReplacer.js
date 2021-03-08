Engine.RegisterInterface("SkirmishReplacer");

/**
 * Message of the form {}
 * sent from InitGame.
 */
Engine.RegisterMessageType("SkirmishReplace");

/**
 * Message of the form { "entity": {number}, "newentity": {number} }
 * sent from InitGame.
 */
Engine.RegisterMessageType("SkirmishReplacerReplaced");
