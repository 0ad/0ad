/**
 * This class creates a new buttonhandler per entity owned by the currently viewed player,
 * or any player if the observer view is selected.
 * The buttons are animated when the according entities are attacked, hence they have to keep a state.
 */
class PanelEntityManager
{
	constructor(playerViewControl, selection, entityOrder)
	{
		this.panelEntityButtons = Engine.GetGUIObjectByName("panelEntityButtons").children;

		this.selection = selection;
		this.entityOrder = entityOrder;

		// One handler per panelEntity owned, sorted according to given order, limited to buttoncount
		this.handlers = [];

		let updater = this.update.bind(this);
		registerSimulationUpdateHandler(updater);
		playerViewControl.registerViewedPlayerChangeHandler(updater);
	}

	update()
	{
		// Obtain entity IDs to display
		let entityIDs =
			g_ViewedPlayer == -1 ?
				g_SimState.players.reduce((ents, pState) => ents.concat(pState.panelEntities), []) :
				g_SimState.players[g_ViewedPlayer].panelEntities;

		let reposition = false;

		// Delete handlers for entities not owned anymore
		for (let i = 0; i < this.handlers.length;)
			if (entityIDs.indexOf(this.handlers[i].entityID) == -1)
			{
				this.handlers[i].destroy();
				this.handlers.splice(i, 1);
				reposition = true;
			}
			else
				++i;

		// Construct new handlers
		for (let entityID of entityIDs)
			if (this.handlers.every(handler => entityID != handler.entityID))
				if (this.insertIfRelevant(entityID))
					reposition = true;

		for (let i = 0; i < this.handlers.length; ++i)
			this.handlers[i].update(i, reposition);
	}

	insertIfRelevant(entityID)
	{
		let entityState = GetEntityState(entityID);

		let orderKey = this.entityOrder.findIndex(entClass =>
			entityState.identity.classes.indexOf(entClass) != -1);

		// Sort depending on given order
		let insertPos = this.handlers.reduce(
			(pos, handler) => {
				if (handler.orderKey <= orderKey)
					++pos;
				return pos;
			},
			0);

		// Don't insert past the button limit
		if (insertPos >= this.panelEntityButtons.length)
			return false;

		// Delete last handler if the limit is reached
		if (this.handlers.length == this.panelEntityButtons.length)
		{
			this.handlers[this.handlers.length - 1].destroy();
			this.handlers.splice(this.handlers.length - 1);
		}

		this.handlers.splice(
			insertPos,
			0,
			new PanelEntity(
				this.selection,
				entityID,
				this.panelEntityButtons.findIndex(button => button.hidden),
				orderKey));

		return true;
	}
}
