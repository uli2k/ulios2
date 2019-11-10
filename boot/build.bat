@ECHO OFF

:编译汇编代码
nasm -o ..\out\setup\SETUP -f bin setup.asm
nasm -o ..\out\setup\SETUP386 -f bin setup386.asm
nasm -o ..\out\setup\F32BOOT -f bin fat32\f32boot.asm
nasm -o ..\out\setup\ULIBOOT -f bin ulifs\uliboot.asm

:用TASM把c0.asm编译为C0T.OBJ，微型模式
C:\TC\TASM C0,C0T /D__TINY__ /MX;
:暂存原本的C0T.OBJ，以防写坏
ren C:\TC\LIB\C0T.OBJ C0T.TMP
:拷贝新的C0T.OBJ
copy C0T.OBJ C:\TC\LIB
del C0T.OBJ
C:
cd \TC
:用TCC编译成16位裸机指令
TCC -lt -mt -ef32ldr c:\ulios2\boot\fat32\f32ldr.c
copy OUT\F32LDR.COM c:\ulios2\out\setup\F32LDR
del OUT\F32LDR.*
TCC -lt -mt -eulildr c:\ulios2\boot\ulifs\ulildr.c
copy OUT\ULILDR.COM c:\ulios2\out\setup\ULILDR
del OUT\ULILDR.*
:删除生成的C0T.OBJ，换回原本的
del LIB\C0T.OBJ
ren LIB\C0T.TMP C0T.OBJ
cd c:\ulios2\boot
