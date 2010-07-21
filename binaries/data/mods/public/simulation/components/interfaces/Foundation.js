Engine.RegisterInterface("Foundation");

// Message sent from Foundation to its own entity when construction
// has been completed.
// Units can watch for this and change task once it's complete.
// Data: { entity: 123, newentity: 234 }
Engine.RegisterMessageType("ConstructionFinished");
