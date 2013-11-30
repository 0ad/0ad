Engine.RegisterInterface("Guard");

// Message of the form { "guarded": entity, "data": msg },
// sent whenever a guarded/escorted unit is attacked
Engine.RegisterMessageType("GuardedAttacked");
