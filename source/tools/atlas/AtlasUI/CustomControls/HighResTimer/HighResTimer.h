class HighResTimer
{
public:
	HighResTimer();
	double GetTime(); // in seconds, relative to some arbitrary time

private:
	wxLongLong m_TickLength;
};
