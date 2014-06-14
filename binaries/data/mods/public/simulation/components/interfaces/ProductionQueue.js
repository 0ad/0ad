Engine.RegisterInterface("ProductionQueue");

// Message of the form { } (use GetQueue if you want the current details),
// sent to the current entity whenever the training queue changes.
Engine.RegisterMessageType("ProductionQueueChanged");

// Message of the form { entity: id }
// sent to the current entity whenever a unit is about to be trained.
Engine.RegisterMessageType("TrainingStarted");

// Message of the form { entities: [id, ...], metadata: ... }
// sent to the current entity whenever a unit has been trained.
Engine.RegisterMessageType("TrainingFinished");
