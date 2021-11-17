Engine.RegisterInterface("Trainer");

/**
 * Message of the form { "entity": number }
 * sent from Trainer component to the current entity whenever a unit is about to be trained.
 */
Engine.RegisterMessageType("TrainingStarted");

/**
 * Message of the form { "entities": number[], "owner": number, "metadata": object }
 * sent from Trainer component to the current entity whenever a unit has been trained.
 */
Engine.RegisterMessageType("TrainingFinished");
