Engine.RegisterInterface("Foundation");

/**
 * Message of the form { "entity": number, "newentity": number }
 * sent from Foundation and Repairable components to its own entity when a construction has been completed.
 * Units can watch for this and change the task once it's complete.
 */
Engine.RegisterMessageType("ConstructionFinished");

/**
 * Message of the form { "to": number }
 * as the percentage complete,
 * sent from Foundation component whenever the foundations progress changes.
 */
Engine.RegisterMessageType("FoundationProgressChanged");

/**
 * Message of the form { "to": number[] }
 * where "to" value is an array of builders entity ids,
 * sent from Foundation component whenever the foundation builders changes.
 */
Engine.RegisterMessageType("FoundationBuildersChanged");
