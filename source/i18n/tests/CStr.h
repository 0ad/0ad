class CStr : public std::string {};
class CStrW : public std::wstring
{
public:
	CStrW(const std::wstring &s) : std::wstring(s) {}
};
