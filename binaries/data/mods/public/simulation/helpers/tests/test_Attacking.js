Engine.LoadHelperScript("Attacking.js");
Engine.LoadComponentScript("interfaces/Attack.js");
Engine.LoadComponentScript("interfaces/Capturable.js");
Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("interfaces/Promotion.js");

// Unit tests for the Attacking helper.
// TODO: Some of it is tested in components/test_Damage.js, which should be spliced and moved.

class testHandleAttackEffects {
	constructor() {
		this.resultString = "";
		this.TESTED_ENTITY_ID = 5;

		this.attackData = {
			"Damage": "Uniquely Hashed Value",
			"Capture": "Something Else Entirely",
		};
	}

	/**
	 * This tests that we inflict multiple effect types.
	 */
	testMultipleEffects() {
		AddMock(this.TESTED_ENTITY_ID, IID_Health, {
			"TakeDamage": x => { this.resultString += x; },
		});

		AddMock(this.TESTED_ENTITY_ID, IID_Capturable, {
			"Capture": x => { this.resultString += x; },
		});

		Attacking.HandleAttackEffects("Test", this.attackData, this.TESTED_ENTITY_ID, INVALID_ENTITY, INVALID_PLAYER);

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

		Attacking.HandleAttackEffects("Test", this.attackData, this.TESTED_ENTITY_ID, INVALID_ENTITY, INVALID_PLAYER);
		TS_ASSERT(this.resultString.indexOf(this.attackData.Damage) === -1);
		TS_ASSERT(this.resultString.indexOf(this.attackData.Capture) !== -1);

		this.resultString = "";
		DeleteMock(this.TESTED_ENTITY_ID, IID_Capturable);
		AddMock(this.TESTED_ENTITY_ID, IID_Health, {
			"TakeDamage": x => { this.resultString += x; },
		});

		Attacking.HandleAttackEffects("Test", this.attackData, this.TESTED_ENTITY_ID, INVALID_ENTITY, INVALID_PLAYER);
		TS_ASSERT(this.resultString.indexOf(this.attackData.Damage) !== -1);
		TS_ASSERT(this.resultString.indexOf(this.attackData.Capture) === -1);
	}

	/**
	 * Check that the Attacked message is [not] sent if [no] receivers exist.
	 */
	testAttackedMessage() {
		Engine.PostMessage = () => TS_ASSERT(false);
		Attacking.HandleAttackEffects("Test", this.attackData, this.TESTED_ENTITY_ID, INVALID_ENTITY, INVALID_PLAYER);

		AddMock(this.TESTED_ENTITY_ID, IID_Capturable, {
			"Capture": () => ({ "captureChange": 0 }),
		});
		let count = 0;
		Engine.PostMessage = () => count++;
		Attacking.HandleAttackEffects("Test", this.attackData, this.TESTED_ENTITY_ID, INVALID_ENTITY, INVALID_PLAYER);
		TS_ASSERT_EQUALS(count, 1);

		AddMock(this.TESTED_ENTITY_ID, IID_Health, {
			"TakeDamage": () => ({ "HPchange": 0 }),
		});
		count = 0;
		Engine.PostMessage = () => count++;
		Attacking.HandleAttackEffects("Test", this.attackData, this.TESTED_ENTITY_ID, INVALID_ENTITY, INVALID_PLAYER);
		TS_ASSERT_EQUALS(count, 1);
	}

	/**
	 * Regression test that bonus multiplier is handled correctly.
	 */
	testBonusMultiplier() {
		AddMock(this.TESTED_ENTITY_ID, IID_Health, {
			"TakeDamage": (_, __, ___, mult) => { TS_ASSERT_EQUALS(mult, 2); },
		});
		AddMock(this.TESTED_ENTITY_ID, IID_Capturable, {
			"Capture": (_, __, ___, mult) => { TS_ASSERT_EQUALS(mult, 2); },
		});

		Attacking.HandleAttackEffects("Test", this.attackData, this.TESTED_ENTITY_ID, INVALID_ENTITY, INVALID_PLAYER, 2);
	}
}

new testHandleAttackEffects().testMultipleEffects();
new testHandleAttackEffects().testSkippedEffect();
new testHandleAttackEffects().testAttackedMessage();
new testHandleAttackEffects().testBonusMultiplier();
