Engine.LoadHelperScript("MultiKeyMap.js");

function setup_keys(map)
{
	map.AddItem("prim_a", "item_a", null, "sec_a");
	map.AddItem("prim_a", "item_b", null, "sec_a");
	map.AddItem("prim_a", "item_c", null, "sec_a");
	map.AddItem("prim_a", "item_a", null, "sec_b");
	map.AddItem("prim_b", "item_a", null, "sec_a");
	map.AddItem("prim_c", "item_a", null, "sec_a");
	map.AddItem("prim_c", "item_a", null, 5);
}

// Check that key-related operations are correct.
function test_keys(map)
{
	TS_ASSERT(map.items.has("prim_a"));
	TS_ASSERT(map.items.has("prim_b"));
	TS_ASSERT(map.items.has("prim_c"));

	TS_ASSERT(map.items.get("prim_a").has("sec_a"));
	TS_ASSERT(map.items.get("prim_a").has("sec_b"));
	TS_ASSERT(!map.items.get("prim_a").has("sec_c"));
	TS_ASSERT(map.items.get("prim_b").has("sec_a"));
	TS_ASSERT(map.items.get("prim_c").has("sec_a"));
	TS_ASSERT(map.items.get("prim_c").has(5));

	TS_ASSERT(map.items.get("prim_a").get("sec_a").length == 3);
	TS_ASSERT(map.items.get("prim_a").get("sec_b").length == 1);
	TS_ASSERT(map.items.get("prim_b").get("sec_a").length == 1);
	TS_ASSERT(map.items.get("prim_c").get("sec_a").length == 1);
	TS_ASSERT(map.items.get("prim_c").get(5).length == 1);

	TS_ASSERT(map.GetItems("prim_a", "sec_a").length == 3);
	TS_ASSERT(map.GetItems("prim_a", "sec_b").length == 1);
	TS_ASSERT(map.GetItems("prim_b", "sec_a").length == 1);
	TS_ASSERT(map.GetItems("prim_c", "sec_a").length == 1);
	TS_ASSERT(map.GetItems("prim_c", 5).length == 1);

	TS_ASSERT(map.HasItem("prim_a", "item_a", "sec_a"));
	TS_ASSERT(map.HasItem("prim_a", "item_b", "sec_a"));
	TS_ASSERT(map.HasItem("prim_a", "item_c", "sec_a"));
	TS_ASSERT(!map.HasItem("prim_a", "item_d", "sec_a"));
	TS_ASSERT(map.HasItem("prim_a", "item_a", "sec_b"));
	TS_ASSERT(!map.HasItem("prim_a", "item_b", "sec_b"));
	TS_ASSERT(!map.HasItem("prim_a", "item_c", "sec_b"));
	TS_ASSERT(map.HasItem("prim_b", "item_a", "sec_a"));
	TS_ASSERT(map.HasItem("prim_c", "item_a", "sec_a"));
	TS_ASSERT(map.HasAnyItem("item_a", "sec_b"));
	TS_ASSERT(map.HasAnyItem("item_b", "sec_a"));
	TS_ASSERT(!map.HasAnyItem("item_d", "sec_a"));
	TS_ASSERT(!map.HasAnyItem("item_b", "sec_b"));

	// Adding the same item increases its count.
	map.AddItem("prim_a", "item_b", 0, "sec_a");
	TS_ASSERT_EQUALS(map.items.get("prim_a").get("sec_a").length, 3);
	TS_ASSERT_EQUALS(map.items.get("prim_a").get("sec_a").filter(item => item._ID == "item_b")[0]._count, 2);
	TS_ASSERT_EQUALS(map.GetItems("prim_a", "sec_a").length, 3);

	// Adding without stackable doesn't invalidate caches, adding with does.
	TS_ASSERT(!map.AddItem("prim_a", "item_b", 0, "sec_a"));
	TS_ASSERT(map.AddItem("prim_a", "item_b", 0, "sec_a", true));

	TS_ASSERT(map.items.get("prim_a").get("sec_a").filter(item => item._ID == "item_b")[0]._count == 4);

	// Likewise removing, unless we now reach 0
	TS_ASSERT(!map.RemoveItem("prim_a", "item_b", "sec_a"));
	TS_ASSERT(map.RemoveItem("prim_a", "item_b", "sec_a", true));
	TS_ASSERT(!map.RemoveItem("prim_a", "item_b", "sec_a"));
	TS_ASSERT(map.RemoveItem("prim_a", "item_b", "sec_a"));

	// Check that cleanup is done
	TS_ASSERT(map.items.get("prim_a").get("sec_a").length == 2);
	TS_ASSERT(map.RemoveItem("prim_a", "item_a", "sec_a"));
	TS_ASSERT(map.RemoveItem("prim_a", "item_c", "sec_a"));
	TS_ASSERT(!map.items.get("prim_a").has("sec_a"));
	TS_ASSERT(map.items.get("prim_a").has("sec_b"));
	TS_ASSERT(map.RemoveItem("prim_a", "item_a", "sec_b"));
	TS_ASSERT(!map.items.has("prim_a"));
}

function setup_items(map)
{
	map.AddItem("prim_a", "item_a", { "value": 1 }, "sec_a");
	map.AddItem("prim_a", "item_b", { "value": 2 }, "sec_a");
	map.AddItem("prim_a", "item_c", { "value": 3 }, "sec_a");
	map.AddItem("prim_a", "item_c", { "value": 1000 }, "sec_a");
	map.AddItem("prim_a", "item_a", { "value": 5 }, "sec_b");
	map.AddItem("prim_b", "item_a", { "value": 6 }, "sec_a");
	map.AddItem("prim_c", "item_a", { "value": 7 }, "sec_a");
}

// Check that items returned are correct.
function test_items(map)
{
	let items = map.GetAllItems("sec_a");
	TS_ASSERT("prim_a" in items);
	TS_ASSERT("prim_b" in items);
	TS_ASSERT("prim_c" in items);
	let sum = 0;
	for (let key in items)
		items[key].forEach(item => (sum += item.value * item._count));
	TS_ASSERT(sum == 22);
}

// Test items, and test that deserialised versions still pass test (i.e. test serialisation).
let map = new MultiKeyMap();
setup_keys(map);
test_keys(map);

map = new MultiKeyMap();
let map2 = new MultiKeyMap();
setup_keys(map);
map2.Deserialize(map.Serialize());
test_keys(map2);

map = new MultiKeyMap();
setup_items(map);
test_items(map);
map = new MultiKeyMap();
map2 = new MultiKeyMap();
setup_items(map);
map2.Deserialize(map.Serialize());
test_items(map2);
