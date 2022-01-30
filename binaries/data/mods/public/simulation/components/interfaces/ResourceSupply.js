Engine.RegisterInterface("ResourceSupply");

/**
 * Message of the form { "from": number, "to": number }
 * sent from ResourceSupply component whenever the supply level changes.
 */
Engine.RegisterMessageType("ResourceSupplyChanged");
