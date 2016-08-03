Engine.RegisterInterface("ProductionQueue");

/**
 * Message of the form {}
 * sent from ProductionQueue component to the current entity whenever the training queue changes.
 */
Engine.RegisterMessageType("ProductionQueueChanged");

/**
 * Message of the form { "entity": number }
 * sent from ProductionQueue component to the current entity whenever a unit is about to be trained.
 */
Engine.RegisterMessageType("TrainingStarted");

/**
 * Message of the form { "entities": number[], "owner": number, "metadata": object }
 * sent from ProductionQueue component to the current entity whenever a unit has been trained.
 */
Engine.RegisterMessageType("TrainingFinished");
