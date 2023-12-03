' Simple VBScript to open the 0 A.D. logs folder
' the path varies on different versions of Windows
Set objShell = CreateObject("Shell.Application")
' 0x1C is equivalent to the ssfLOCALAPPDATA constant in VB
objShell.Explore objShell.Namespace(&H1C&).Self.Path & "\0ad\logs"
