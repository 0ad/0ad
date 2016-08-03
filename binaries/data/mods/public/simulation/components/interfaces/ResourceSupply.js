Engine.RegisterInterface("ResourceSupply");

/**
 * Message of the form { "from": number, "to": number }
 * sent from ResourceSupply component whenever the supply level changes.
 */
Engine.RegisterMessageType("ResourceSupplyChanged");

/**
 * Message of the form { "to": number }
 * sent from ResourceSupply component whenever the number of gatherer changes.
 */
Engine.RegisterMessageType("ResourceSupplyNumGatherersChanged");
