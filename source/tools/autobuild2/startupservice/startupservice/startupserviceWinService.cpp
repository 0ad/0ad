// startupservice.cpp : main Windows Service project file.

#include "stdafx.h"
#include <string.h>
#include "startupserviceWinService.h"

using namespace startupservice;
using namespace System::Text;
using namespace System::Security::Policy;
using namespace System::Reflection;

//To install/uninstall the service, type: "startupservice.exe -Install [-u]"
int _tmain(int argc, _TCHAR* argv[])
{
	if (argc >= 2)
	{
		if (argv[1][0] == _T('/'))
		{
			argv[1][0] = _T('-');
		}

		if (_tcsicmp(argv[1], _T("-Install")) == 0)
		{
			array<String^>^ myargs = System::Environment::GetCommandLineArgs();
			array<String^>^ args = gcnew array<String^>(myargs->Length - 1);

			// Set args[0] with the full path to the assembly,
			Assembly^ assem = Assembly::GetExecutingAssembly();
			args[0] = assem->Location;

			Array::Copy(myargs, 2, args, 1, args->Length - 1);
			AppDomain^ dom = AppDomain::CreateDomain(L"execDom");
			Type^ type = System::Object::typeid;
			String^ path = type->Assembly->Location;
			StringBuilder^ sb = gcnew StringBuilder(path->Substring(0, path->LastIndexOf(L"\\")));
			sb->Append(L"\\InstallUtil.exe");
			Evidence^ evidence = gcnew Evidence();
			dom->ExecuteAssembly(sb->ToString(), evidence, args);
		}
	}
	else
	{
		ServiceBase::Run(gcnew startupserviceWinService());
	}
}
