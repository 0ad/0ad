Engine.RegisterInterface("Foundation");

// Message sent from Foundation to its own entity when construction
// has been completed.
// Units can watch for this and change task once it's complete.
// Data: { entity: 123, newentity: 234 }
Engine.RegisterMessageType("ConstructionFinished");

// Message of the form { "to", 59 }, as the percentage complete
// sent whenever the foundations progress changes.
Engine.RegisterMessageType("FoundationProgressChanged");
