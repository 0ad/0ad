let statusEffects = {
	"test_A": {
		"code": "test_a",
		"statusName": "A",
		"applierTooltip": "TTA"
	},
	"test_B": {
		"code": "test_b",
		"statusName": "B",
		"applierTooltip": "TTB"
	}
};

Engine.ListDirectoryFiles = () => Object.keys(statusEffects);
Engine.ReadJSONFile = (file) => statusEffects[file];

let sem = new StatusEffectsMetadata();

TS_ASSERT_UNEVAL_EQUALS(sem.getData("test_a"), {
	"applierTooltip": "TTA",
	"code": "test_a",
	"icon": "default",
	"statusName": "A",
	"receiverTooltip": ""
});
TS_ASSERT_UNEVAL_EQUALS(sem.getData("test_b"), {
	"applierTooltip": "TTB",
	"code": "test_b",
	"icon": "default",
	"statusName": "B",
	"receiverTooltip": ""
});
TS_ASSERT_UNEVAL_EQUALS(sem.getApplierTooltip("test_a"), "TTA");
TS_ASSERT_UNEVAL_EQUALS(sem.getIcon("test_b"), "default");
TS_ASSERT_UNEVAL_EQUALS(sem.getName("test_a"), "A");
TS_ASSERT_UNEVAL_EQUALS(sem.getReceiverTooltip("test_b"), "");
