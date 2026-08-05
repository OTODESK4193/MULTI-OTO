@echo off
echo Cleaning build output directory...
if exist "out" (
    rmdir /s /q "out"
    echo Clean completed!
) else (
    echo "out" directory does not exist. Already clean.
)
pause
