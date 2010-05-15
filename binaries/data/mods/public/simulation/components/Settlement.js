function Settlement() {}

Settlement.prototype.Schema =
	"<empty/>";

Engine.RegisterComponentType(IID_Settlement, "Settlement", Settlement);

/*
 * TODO: the vague plan is that this should keep track of who currently owns the settlement,
 * and some other code can detect this (or get notified of changes) when it needs to.
 * A civcenter's BuildRestrictions component will see that it's being built on this settlement,
 * call MoveOutOfWorld on us (so we're invisible and only the building is visible/selectable),
 * tell us that its player owns us, and move us back into our original position when the building
 * is destroyed. Don't know if that's a sensible plan, though.
 */
