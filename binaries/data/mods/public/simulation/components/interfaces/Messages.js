/**
 * Broadcast message 
 * sent when one entity is changed to other:
 * - from Foundation component when building construction is done
 * - from Promotion component when unit is promoted
 * - from Mirage component when a fogged entity is re-discovered
 * Data: { entity: <integer>, newentity: <integer> }
 */
Engine.RegisterMessageType("EntityRenamed");

Engine.RegisterMessageType("DiplomacyChanged");

// Message of the form { "to": receiver, "from": sender, "amounts": amounts }.
// sent whenever a tribute is sent
Engine.RegisterMessageType("TributeExchanged");
