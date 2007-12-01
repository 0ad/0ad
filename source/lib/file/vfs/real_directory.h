
class RealDirectory
{
public:
	RealDirectory(const char* path, unsigned priority, unsigned flags)
		: m_path(path), m_priority(priority), m_flags(flags)
	{
	}

	const char* Path() const
	{
		return m_path;
	}

	unsigned Priority() const
	{
		return m_priority;
	}

	unsigned Flags() const
	{
		return m_flags;
	}

private:
	// note: paths are relative to the root directory, so storing the
	// entire path instead of just the portion relative to the mount point
	// is not all too wasteful.
	const char* m_path;

	unsigned m_priority;

	unsigned m_flags;
};
