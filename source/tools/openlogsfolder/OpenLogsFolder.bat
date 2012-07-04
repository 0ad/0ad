@REM We prefer to use the %localappdata% variable when available
@REM but it's not on XP/2000, so we use a VBScript as an alternative
(cd /D "%localappdata%\0ad\logs\" && start .) || cscript //Nologo OpenLogsFolder.vbs
