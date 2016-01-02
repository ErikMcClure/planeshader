@ECHO OFF
md "..\Packages\PlaneShader"

XCOPY "*.sln" "..\Packages\PlaneShader" /S /C /I /R /Y
XCOPY "*.lib" "..\Packages\PlaneShader" /S /C /I /R /Y
XCOPY "*.dll" "..\Packages\PlaneShader" /S /C /I /R /Y
XCOPY "*.vcxproj" "..\Packages\PlaneShader" /S /C /I /R /Y
XCOPY "examples\*.cpp" "..\Packages\PlaneShader\examples" /S /C /I /R /Y
XCOPY "examples\*.h" "..\Packages\PlaneShader\examples" /S /C /I /R /Y
XCOPY "examples\*.filters" "..\Packages\PlaneShader\examples" /S /C /I /R /Y
XCOPY "PlaneShader_net\*.cpp" "..\Packages\PlaneShader\PlaneShader_net" /S /C /I /R /Y
XCOPY "PlaneShader_net\*.h" "..\Packages\PlaneShader\PlaneShader_net" /S /C /I /R /Y
XCOPY "PlaneShader_net\*.filters" "..\Packages\PlaneShader\PlaneShader_net" /S /C /I /R /Y

md "..\Packages\PlaneShader\include"
md "..\Packages\PlaneShader\doc"

XCOPY "include\*.h" "..\Packages\PlaneShader\include" /S /C /I /R /Y
XCOPY "include\*.inl" "..\Packages\PlaneShader\include" /S /C /I /R /Y
XCOPY "doc\*.txt" "..\Packages\PlaneShader\doc" /S /C /I /R /Y
XCOPY "examples\bin\*.exe" "..\Packages\PlaneShader\examples\bin" /C /I /R /Y
XCOPY "examples\bin64\*.exe" "..\Packages\PlaneShader\examples\bin64" /C /I /R /Y
XCOPY "bin\*.pdb" "..\Packages\PlaneShader\bin" /C /I /R /Y
XCOPY "bin64\*.pdb" "..\Packages\PlaneShader\bin64" /C /I /R /Y

del "..\Packages\PlaneShader\examples\bin\*.lib"
del "..\Packages\PlaneShader\examples\bin\*_d.exe"
del "..\Packages\PlaneShader\examples\bin64\*.lib"
del "..\Packages\PlaneShader\examples\bin64\*_d.exe"

md "..\Packages\PlaneShader\examples\media"

XCOPY "examples\media\*.png" "..\Packages\PlaneShader\examples\media" /C /I /R /Y
XCOPY "examples\media\*.hlsl" "..\Packages\PlaneShader\examples\media" /C /I /R /Y
XCOPY "examples\media\*.ico" "..\Packages\PlaneShader\examples\media" /C /I /R /Y
XCOPY "examples\media\*.rc" "..\Packages\PlaneShader\examples\media" /C /I /R /Y