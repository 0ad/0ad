let damageTypes = {
	"test_A": {
		"code": "test_a",
		"name": "A",
		"order": 2
	},
	"test_B": {
		"code": "test_b",
		"name": "B",
		"order": 1
	}
};

Engine.ListDirectoryFiles = () => Object.keys(damageTypes);
Engine.ReadJSONFile = (file) => damageTypes[file];

let dtm = new DamageTypesMetadata();

TS_ASSERT_EQUALS(dtm.getName("test_a"), "A");
TS_ASSERT_EQUALS(dtm.getName("test_b"), "B");
TS_ASSERT_EQUALS(dtm.getName("test_c"), "test_c");

TS_ASSERT_UNEVAL_EQUALS(dtm.sort(["test_c", "test_a", "test_b"]), ["test_b", "test_a", "test_c"]);
