@ECHO OFF
SET PATH=C:\DJGPP\BIN;%PATH%
SET DJGPP=C:\DJGPP\DJGPP.ENV
nasm -o ..\out\setup\SETUP -f bin setup.asm
nasm -o ..\out\setup\SETUP386 -f bin setup386.asm
nasm -o ..\out\setup\F32BOOT -f bin fat32\f32boot.asm
nasm -o ..\out\setup\ULIBOOT -f bin ulifs\uliboot.asm

C:\TC\TASM C0,C0T /D__TINY__ /MX;
ren C:\TC\LIB\C0T.OBJ C0T.TMP
move /y C0T.OBJ C:\TC\LIB
C:
cd \TC
TCC -lt -mt -ef32ldr c:\progra~1\micros~2\myproj~1\ulios2\boot\fat32\f32ldr.c
del OUT\F32LDR.OBJ
move /y OUT\F32LDR.COM c:\progra~1\micros~2\myproj~1\ulios2\out\setup\F32LDR
TCC -lt -mt -eulildr c:\progra~1\micros~2\myproj~1\ulios2\boot\ulifs\ulildr.c
del OUT\ULILDR.OBJ
move /y OUT\ULILDR.COM c:\progra~1\micros~2\myproj~1\ulios2\out\setup\ULILDR
del LIB\C0T.OBJ
ren LIB\C0T.TMP C0T.OBJ
cd c:\progra~1\micros~2\myproj~1\ulios2\boot
