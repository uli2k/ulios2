@ECHO OFF

C:
cd \TC
TCC -lt -mt -efmtboot c:\ulios2\tools\fmtboot.c
del OUT\FMTBOOT.OBJ
move /y OUT\FMTBOOT.COM c:\ulios2\out\setup
TCC -lt -mt -eulifsfmt c:\ulios2\tools\ulifsfmt.c
del OUT\ulifsfmt.OBJ
move /y OUT\ULILFSFMT.COM c:\ulios2\out\setup
cd c:\ulios2\tools
