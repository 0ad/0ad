Engine.RegisterInterface("ProductionQueue");

/**
 * Message of the form {}
 * sent from ProductionQueue component to the current entity whenever the training queue changes.
 */
Engine.RegisterMessageType("ProductionQueueChanged");
