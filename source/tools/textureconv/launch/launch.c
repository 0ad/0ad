#include <windows.h>

const char* ExeName = "textureconv.exe";
const char* ExtraParams = "REPLACEMEREPLACEMEREPLACEMEREPLACEMEREPLACEMEREPLACEMEREPLACEMEREPLACEME";

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR pCmdLine, int nCmdShow)
{
	static char Path[MAX_PATH];
	static char Prog[MAX_PATH];
	char *Srch;
	
	LPSTR NewCmdLine;
	if (pCmdLine)
	{
		NewCmdLine = malloc(strlen(pCmdLine) + strlen(ExtraParams) + 2);
		if (!NewCmdLine)
			return 3;
		strcpy(NewCmdLine, ExtraParams);
		strcat(NewCmdLine, " ");
		strcat(NewCmdLine, pCmdLine);
	}
	else
	{
		NewCmdLine = (char*)ExtraParams;
	}
	
	if (!GetModuleFileName(hInstance, Path, sizeof(Path)))
		return 1;
	
	Srch = strrchr(Path, '\\');
	if (!Srch)
		return 2;
	Srch[0] = '\0';
	
	strcpy(Prog, Path);
	strcat(Prog, "\\");
	strcat(Prog, ExeName);
	
	ShellExecute(NULL, "open", Prog, NewCmdLine, Path, SW_SHOWNORMAL);
	
	return 0;
}