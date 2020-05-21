let statusEffects = {
	"test_A": {
		"code": "test_a",
		"StatusName": "A",
		"StatusTooltip": "TTA"
	},
	"test_B": {
		"code": "test_b",
		"StatusName": "B",
		"StatusTooltip": "TTB"
	}
};

Engine.ListDirectoryFiles = () => Object.keys(statusEffects);
Engine.ReadJSONFile = (file) => statusEffects[file];

let sem = new StatusEffectsMetadata();

// Template data takes precedence over generic data.
TS_ASSERT_UNEVAL_EQUALS(sem.augment("test_a"), {
	"code": "test_a", "StatusName": "A", "StatusTooltip": "TTA"
});
TS_ASSERT_UNEVAL_EQUALS(sem.augment("test_b"), {
	"code": "test_b", "StatusName": "B", "StatusTooltip": "TTB"
});
TS_ASSERT_UNEVAL_EQUALS(sem.augment("test_a", { "StatusName": "test" }), {
	"code": "test_a", "StatusName": "test", "StatusTooltip": "TTA"
});
TS_ASSERT_UNEVAL_EQUALS(sem.augment("test_c", {	"StatusName": "test" }), {
	"StatusName": "test"
});
