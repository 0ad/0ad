let test = 0;

function incrementTest()
{
	test += 1;
}

async function waitAndIncrement(promise)
{
	await promise;
	incrementTest();
}

{
	let resolve;
	const promise = new Promise(res => {
		incrementTest();
		resolve = res;
	});
	waitAndIncrement(promise);
	TS_ASSERT_EQUALS(test, 1);
	resolve();
	// At this point, waitAndIncrement is still not run, but is now free to run.
	TS_ASSERT_EQUALS(test, 1);
}

function endTest()
{
	TS_ASSERT_EQUALS(test, 2);
}
