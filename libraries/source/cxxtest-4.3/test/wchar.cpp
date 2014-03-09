//
// This program is used to check if the compiler supports basic_string<wchar_t>
//
#include <string>

int main()
{
    std::basic_string<wchar_t> s(L"s");
    return 0;
}

