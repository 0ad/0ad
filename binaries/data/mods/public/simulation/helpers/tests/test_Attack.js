AttackEffects = class AttackEffects
{
	constructor() {}
	Receivers()
	{
		return [{
			"type": "Damage",
			"IID": "IID_Health",
			"method": "TakeDamage"
		},
		{
			"type": "Capture",
			"IID": "IID_Capturable",
			"method": "Capture"
		},
		{
			"type": "ApplyStatus",
			"IID": "IID_StatusEffectsReceiver",
			"method": "ApplyStatus"
		}];
	}
};

Engine.LoadHelperScript("Attack.js");
Engine.LoadComponentScript("interfaces/Capturable.js");
Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("interfaces/Promotion.js");
Engine.LoadComponentScript("interfaces/Resistance.js");
Engine.LoadComponentScript("interfaces/StatusEffectsReceiver.js");

// TODO: Some of it is tested in components/test_Damage.js, which should be spliced and moved.

class testHandleAttackEffects {
	constructor() {
		this.resultString = "";
		this.TESTED_ENTITY_ID = 5;

		this.attackData = {
			"Damage": "1",
			"Capture": "2",
			"ApplyStatus": {
				"statusName": {}
			}
		};
	}

	/**
	 * This tests that we inflict multiple effect types.
	 */
	testMultipleEffects() {
		AddMock(this.TESTED_ENTITY_ID, IID_Health, {
			"TakeDamage": x => { this.resultString += x; },
			"GetHitpoints": () => 1,
			"GetMaxHitpoints": () => 1,
		});

		AddMock(this.TESTED_ENTITY_ID, IID_Capturable, {
			"Capture": x => { this.resultString += x; },
		});

		AttackHelper.HandleAttackEffects(this.TESTED_ENTITY_ID, {
			"type": "Test",
			"attackData": this.attackData,
			"attacker": INVALID_ENTITY,
			"attackerOwner": INVALID_PLAYER
		});

		TS_ASSERT(this.resultString.indexOf(this.attackData.Damage) !== -1);
		TS_ASSERT(this.resultString.indexOf(this.attackData.Capture) !== -1);
	}

	/**
	 * This tests that we correctly handle effect types if one is not received.
	 */
	testSkippedEffect() {
		AddMock(this.TESTED_ENTITY_ID, IID_Capturable, {
			"Capture": x => { this.resultString += x; },
		});

		AttackHelper.HandleAttackEffects(this.TESTED_ENTITY_ID, {
			"type": "Test",
			"attackData": this.attackData,
			"attacker": INVALID_ENTITY,
			"attackerOwner": INVALID_PLAYER
		});
		TS_ASSERT(this.resultString.indexOf(this.attackData.Damage) === -1);
		TS_ASSERT(this.resultString.indexOf(this.attackData.Capture) !== -1);

		this.resultString = "";
		DeleteMock(this.TESTED_ENTITY_ID, IID_Capturable);
		AddMock(this.TESTED_ENTITY_ID, IID_Health, {
			"TakeDamage": x => { this.resultString += x; },
			"GetHitpoints": () => 1,
			"GetMaxHitpoints": () => 1,
		});

		AttackHelper.HandleAttackEffects(this.TESTED_ENTITY_ID, {
			"type": "Test",
			"attackData": this.attackData,
			"attacker": INVALID_ENTITY,
			"attackerOwner": INVALID_PLAYER
		});
		TS_ASSERT(this.resultString.indexOf(this.attackData.Damage) !== -1);
		TS_ASSERT(this.resultString.indexOf(this.attackData.Capture) === -1);
	}

	/**
	 * Check that the Attacked message is [not] sent if [no] receivers exist.
	 */
	testAttackedMessage() {
		Engine.PostMessage = () => TS_ASSERT(false);
		AttackHelper.HandleAttackEffects(this.TESTED_ENTITY_ID, {
			"type": "Test",
			"attackData": this.attackData,
			"attacker": INVALID_ENTITY,
			"attackerOwner": INVALID_PLAYER
		});

		AddMock(this.TESTED_ENTITY_ID, IID_Capturable, {
			"Capture": () => ({ "captureChange": 0 }),
		});
		let count = 0;
		Engine.PostMessage = () => count++;
		AttackHelper.HandleAttackEffects(this.TESTED_ENTITY_ID, {
			"type": "Test",
			"attackData": this.attackData,
			"attacker": INVALID_ENTITY,
			"attackerOwner": INVALID_PLAYER
		});
		TS_ASSERT_EQUALS(count, 1);

		AddMock(this.TESTED_ENTITY_ID, IID_Health, {
			"TakeDamage": () => ({ "healthChange": 0 }),
			"GetHitpoints": () => 1,
			"GetMaxHitpoints": () => 1,
		});
		count = 0;
		Engine.PostMessage = () => count++;
		AttackHelper.HandleAttackEffects(this.TESTED_ENTITY_ID, {
			"type": "Test",
			"attackData": this.attackData,
			"attacker": INVALID_ENTITY,
			"attackerOwner": INVALID_PLAYER
		});
		TS_ASSERT_EQUALS(count, 1);
	}

	/**
	 * Regression test that StatusEffects are handled correctly.
	 */
	testStatusEffects() {
		let cmpStatusEffectsReceiver = AddMock(this.TESTED_ENTITY_ID, IID_StatusEffectsReceiver, {
			"ApplyStatus": (effectData, __, ___) => {
				TS_ASSERT_UNEVAL_EQUALS(effectData, this.attackData.ApplyStatus);
			}
		});
		let spy = new Spy(cmpStatusEffectsReceiver, "ApplyStatus");

		AttackHelper.HandleAttackEffects(this.TESTED_ENTITY_ID, {
			"type": "Test",
			"attackData": this.attackData,
			"attacker": INVALID_ENTITY,
			"attackerOwner": INVALID_PLAYER
		}, 2);
		TS_ASSERT_EQUALS(spy._called, 1);
	}

	/**
	 * Regression test that bonus multiplier is handled correctly.
	 */
	testBonusMultiplier() {
		AddMock(this.TESTED_ENTITY_ID, IID_Health, {
			"TakeDamage": (amount, __, ___) => {
				TS_ASSERT_EQUALS(amount, this.attackData.Damage * 2);
			},
			"GetHitpoints": () => 1,
			"GetMaxHitpoints": () => 1,
		});
		AddMock(this.TESTED_ENTITY_ID, IID_Capturable, {
			"Capture": (amount, __, ___) => {
				TS_ASSERT_EQUALS(amount, this.attackData.Capture * 2);
			},
		});

		AttackHelper.HandleAttackEffects(this.TESTED_ENTITY_ID, {
			"type": "Test",
			"attackData": this.attackData,
			"attacker": INVALID_ENTITY,
			"attackerOwner": INVALID_PLAYER
		}, 2);
	}
}

new testHandleAttackEffects().testMultipleEffects();
new testHandleAttackEffects().testSkippedEffect();
new testHandleAttackEffects().testAttackedMessage();
new testHandleAttackEffects().testStatusEffects();
new testHandleAttackEffects().testBonusMultiplier();
