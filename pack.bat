@ECHO OFF
md "..\Packages\PlaneShader"

RMDIR /S /Q ".\Debug\"
RMDIR /S /Q ".\Release\"
RMDIR /S /Q ".\x64\"
RMDIR /S /Q ".\Win32\"
RMDIR /S /Q ".\examples\00 Testbed\x64\"
RMDIR /S /Q ".\examples\00 Testbed\Win32\"
RMDIR /S /Q ".\PlaneShader\x64\"
RMDIR /S /Q ".\PlaneShader\Win32\"
DEL /F /Q .\PlaneShader\psDirectX11_*.h
DEL /F /S /Q *.log *.exp *.idb *.ilk *.iobj *.ipdb *.metagen *.pch
DEL /F /Q ".\bin\*.txt"
DEL /F /Q ".\bin32\*.txt"
DEL /F /Q ".\bin\00 Testbed*.lib"
DEL /F /Q ".\bin32\00 Testbed*.lib"
DEL /F /Q ".\bin\00 Testbed*.pdb"
DEL /F /Q ".\bin32\00 Testbed*.pdb"
XCOPY *.* "..\Packages\PlaneShader" /S /C /I /R /Y
RMDIR /S /Q "..\Packages\PlaneShader\lib\"
DEL /F /Q "..\Packages\PlaneShader\bin\D3D*.dll"
DEL /F /Q "..\Packages\PlaneShader\bin\D3D*.pdb"
DEL /F /Q "..\Packages\PlaneShader\bin32\D3D*.dll"
DEL /F /Q "..\Packages\PlaneShader\bin32\D3D*.pdb"
DEL /F /S /Q "..\Packages\PlaneShader\*.zip"
