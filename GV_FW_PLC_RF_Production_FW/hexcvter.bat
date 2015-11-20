@echo off
set REAL_PASS1=%1 
echo %REAL_PASS1%
set "str1=\" 
set "str2=%1" 
set "str3=%str1%%str2%" 
echo.%str3%
c:\Keil\C51\BIN\Ohx51.exe %str3% RANGE(B0:0 - B0:0xFFFF) HEXFILE (bridge.H00)
c:\Keil\C51\BIN\Ohx51.exe %str3% RANGE(B1:0 - B1:0xFFFF) HEXFILE (bridge.H01) OFFSET (-0x10000)
c:\Keil\C51\BIN\Ohx51.exe %str3% RANGE(B2:0 - B2:0xFFFF) HEXFILE (bridge.H02) OFFSET (-0x20000)
c:\Keil\C51\BIN\Ohx51.exe %str3% RANGE(B3:0 - B3:0xFFFF) HEXFILE (bridge.H03) OFFSET (-0x30000)
c:\Keil\C51\BIN\Ohx51.exe %str3% RANGE(B4:0 - B4:0xFFFF) HEXFILE (bridge.H04) OFFSET (-0x40000)
c:\Keil\C51\BIN\Ohx51.exe %str3% RANGE(B5:0 - B5:0xFFFF) HEXFILE (bridge.H05) OFFSET (-0x50000)
c:\Keil\C51\BIN\Ohx51.exe %str3% RANGE(B6:0 - B6:0xFFFF) HEXFILE (bridge.H06) OFFSET (-0x60000)
c:\Keil\C51\BIN\Ohx51.exe %str3% RANGE(B7:0 - B7:0xFFFF) HEXFILE (bridge.H07) OFFSET (-0x70000)
