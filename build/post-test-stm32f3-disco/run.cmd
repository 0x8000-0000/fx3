set OPENOCD_HOME=c:\Libs\Embedded\openocd-0.9.0

%OPENOCD_HOME%\bin\openocd.exe -s %OPENOCD_HOME%\share\openocd\scripts -f board\stm32f3discovery.cfg -l openocd.log -c "program obj.gcc/posttest.elf verify reset exit"
