function* GenerateMap()
{
	try
	{
		yield;
	}
	catch (error)
	{
		TS_ASSERT(error instanceof Error);
		TS_ASSERT_EQUALS(error.message, "Failed to convert the yielded value to an integer.");
		yield 50;
		return;
	}

	TS_FAIL("The yield statement didn't throw.");
}
