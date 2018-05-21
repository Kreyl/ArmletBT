:start

"C:\Program Files (x86)\STMicroelectronics\STM32 ST-LINK Utility\ST-LINK Utility\ST-LINK_CLI.exe" -c SWD -P ArmletDT.hex 0x08000000 -V  -Rst

@if %ERRORLEVEL% EQU 0 echo Done

@REM pause
@ping 10.25.11.254 -n 1 -w 2000 > nul

goto start