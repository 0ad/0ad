/**
 * This class provides information on the current merchant ships.
 */
TraderStatusText.prototype.Components.prototype.ShipText = class
{
	getText(traderNumber, idleTags)
	{
		let active = traderNumber.shipTrader.trading;
		let inactive = traderNumber.shipTrader.total - active;

		let message = this.IdleShipTraderText[active ? "active" : "no-active"][inactive ? "inactive" : "no-inactive"](inactive);

		let activeString = sprintf(
			translatePlural(
				"There is %(numberTrading)s merchant ship trading",
				"There are %(numberTrading)s merchant ships trading",
				active
			),
			{ "numberTrading": active }
		);

		let inactiveString = sprintf(
			active ?
				translatePlural(
					"%(numberOfShipTraders)s inactive",
					"%(numberOfShipTraders)s inactive",
					inactive
				) :
				translatePlural(
					"%(numberOfShipTraders)s merchant ship inactive",
					"%(numberOfShipTraders)s merchant ships inactive",
					inactive
				),
			{ "numberOfShipTraders": inactive }
		);

		return sprintf(message, {
			"openingTradingString": activeString,
			"inactiveString": setStringTags(inactiveString, idleTags)
		});
	}
};

TraderStatusText.prototype.Components.prototype.ShipText.prototype.IdleShipTraderText = {
	"active": {
		"inactive": () => translate("%(openingTradingString)s, and %(inactiveString)s."),
		"no-inactive": () => translate("%(openingTradingString)s.")
	},
	"no-active": {
		"inactive": inactive => translatePlural("There is %(inactiveString)s.", "There are %(inactiveString)s.", inactive),
		"no-inactive": () => translate("There are no merchant ships.")
	}
};
