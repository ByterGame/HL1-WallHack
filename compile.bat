@echo off
chcp 65001 > nul
echo Компиляция
g++ -static -m32 test.cpp -o test.exe -lpsapi
if %errorlevel% == 0 (
    echo Успешно скомпилировано
    test.exe
) else (
    echo Ошибка компиляции
    echo Попробуй с правами админа
)
pause