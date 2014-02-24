@echo off
Set RegQry=HKLM\Hardware\Description\System\CentralProcessor\0
 
REG.exe Query %RegQry% > checkOS.txt
 
Find /i "x86" < CheckOS.txt > StringCheck.txt
 
If %ERRORLEVEL% == 0 (
	del StringCheck.txt
	Echo "32 Bit Operating system detected, installing 32 bit lighthaar"
	copy lighthaar.inf %windir%\system32\
	copy lighthaar32\lighthaar.dll %windir%\system32\

	cd /d %windir%\system32\
	rundll32 setupapi.dll,InstallHinfSection DefaultInstall 0 %windir%\system32\lighthaar.inf
) ELSE (
	del StringCheck.txt
	Echo "64 Bit Operating System detected, installing 64 bit and 32 bit lighthaar"
	copy lighthaar.inf %windir%\system32\
	copy lighthaar64\lighthaar.dll %windir%\system32\
	copy lighthaar.inf %windir%\SysWOW64\
	copy lighthaar32\lighthaar.dll %windir%\SysWOW64\

	cd /d %windir%\system32\
	rundll32 setupapi.dll,InstallHinfSection DefaultInstall 0 %windir%\system32\lighthaar.inf

	cd /d %windir%\SysWOW64\
	rundll32 setupapi.dll,InstallHinfSection DefaultInstall 0 %windir%\SYSWOW64\lighthaar.inf
)

