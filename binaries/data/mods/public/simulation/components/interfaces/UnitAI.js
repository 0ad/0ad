Engine.RegisterInterface("UnitAI");

/**
 * Message of the form { "idle": boolean }
 * sent from UnitAI whenever the unit's idle status changes.
 */
Engine.RegisterMessageType("UnitIdleChanged");

/**
 * Message of the form { "to": string }
 * where "to" value is a UnitAI stance,
 * sent from UnitAI whenever the unit's stance changes.
 */
Engine.RegisterMessageType("UnitStanceChanged");

/**
 * Message of the form { "to": string }
 * where "to" value is a UnitAI state,
 * sent from UnitAI whenever the unit changes state.
 */
Engine.RegisterMessageType("UnitAIStateChanged");

/**
 * Message of the form { "to": number[] }
 * where "to" value is an array of data orders given by GetOrderData,
 * sent from UnitAI whenever the unit order data changes.
 */
Engine.RegisterMessageType("UnitAIOrderDataChanged");

/**
 * Message of the form { "entity": number }
 * sent from UnitAI whenever a pickup is requested.
 */
Engine.RegisterMessageType("PickupRequested");

/**
 * Message of the form { "entity": number }
 * sent from UnitAI whenever a pickup is aborted.
 */
Engine.RegisterMessageType("PickupCanceled");
