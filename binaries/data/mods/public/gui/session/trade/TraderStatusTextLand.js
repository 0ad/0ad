/**
 * This class provides information on the current trade carts.
 */
TraderStatusText.prototype.Components.prototype.LandText = class
{
	getText(traderNumber, idleTags)
	{
		let active = traderNumber.landTrader.trading;
		let garrisoned = traderNumber.landTrader.garrisoned;
		let inactive = traderNumber.landTrader.total - active - garrisoned;

		let message = this.IdleLandTraderText[active ? "active" : "no-active"][garrisoned ? "garrisoned" : "no-garrisoned"][inactive ? "inactive" : "no-inactive"](inactive);

		let activeString = sprintf(
			translatePlural(
				"There is %(numberTrading)s land trader trading",
				"There are %(numberTrading)s land traders trading",
				active
			),
			{ "numberTrading": active }
		);

		let inactiveString = sprintf(
			active || garrisoned ?
				translatePlural(
					"%(numberOfLandTraders)s inactive",
					"%(numberOfLandTraders)s inactive",
					inactive
				) :
				translatePlural(
					"%(numberOfLandTraders)s land trader inactive",
					"%(numberOfLandTraders)s land traders inactive",
					inactive
				),
			{ "numberOfLandTraders": inactive }
		);

		let garrisonedString = sprintf(
			active || inactive ?
				translatePlural(
					"%(numberGarrisoned)s garrisoned on a trading merchant ship",
					"%(numberGarrisoned)s garrisoned on a trading merchant ship",
					garrisoned
				) :
				translatePlural(
					"There is %(numberGarrisoned)s land trader garrisoned on a trading merchant ship",
					"There are %(numberGarrisoned)s land traders garrisoned on a trading merchant ship",
					garrisoned
				),
			{ "numberGarrisoned": garrisoned }
		);

		return sprintf(message, {
			"openingTradingString": activeString,
			"openingGarrisonedString": garrisonedString,
			"garrisonedString": garrisonedString,
			"inactiveString": setStringTags(inactiveString, idleTags)
		});
	}
};

TraderStatusText.prototype.Components.prototype.LandText.prototype.IdleLandTraderText = {
	"active": {
		"garrisoned": {
			"no-inactive": () => translate("%(openingTradingString)s, and %(garrisonedString)s."),
			"inactive": () => translate("%(openingTradingString)s, %(garrisonedString)s, and %(inactiveString)s.")
		},
		"no-garrisoned": {
			"no-inactive": () => translate("%(openingTradingString)s."),
			"inactive": () => translate("%(openingTradingString)s, and %(inactiveString)s.")
		}
	},
	"no-active": {
		"garrisoned": {
			"no-inactive": () => translate("%(openingGarrisonedString)s."),
			"inactive": () => translate("%(openingGarrisonedString)s, and %(inactiveString)s.")
		},
		"no-garrisoned": {
			"inactive": inactive => translatePlural("There is %(inactiveString)s.", "There are %(inactiveString)s.", inactive),
			"no-inactive": () => translate("There are no land traders.")
		}
	}
};
