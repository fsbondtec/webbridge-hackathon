@ECHO OFF

:: remove old build files
if exist CMakeUserPresets.json (
	del CMakeUserPresets.json
)
if exist build (
	rmdir /s /q build
)
if exist frontend\dist (
	rmdir /s /q frontend\dist
)
if exist frontend\node_modules (
	rmdir /s /q frontend\node_modules
)

CALL conda activate webbridge_hackathon
if "%CONDA_DEFAULT_ENV%" NEQ "webbridge_hackathon" (
	GOTO END
)


conan install . --build=missing ^
    -s compiler=msvc -s compiler.version=194 -s compiler.cppstd=20 -s compiler.runtime=dynamic ^
    -s arch=x86_64 -s os=Windows -s build_type=Release
if ERRORLEVEL 1 (
    GOTO END
)

conan install . --build=missing ^
    -s compiler=msvc -s compiler.version=194 -s compiler.cppstd=20 -s compiler.runtime=dynamic ^
    -s arch=x86_64 -s os=Windows -s build_type=Debug
if ERRORLEVEL 1 (
    GOTO END
)

conan install . --build=missing ^
    -s compiler=msvc -s compiler.version=194 -s compiler.cppstd=20 -s compiler.runtime=dynamic ^
    -s arch=x86_64 -s os=Windows -s "&:build_type=RelWithDebInfo"
if ERRORLEVEL 1 (
    GOTO END
)

conan install . --build=missing ^
    -s compiler=msvc -s compiler.version=194 -s compiler.cppstd=20 -s compiler.runtime=dynamic ^
    -s arch=x86_64 -s os=Windows -s "&:build_type=MinSizeRel"
if ERRORLEVEL 1 (
    GOTO END
)

:: Use Conan's generated environment
CALL build\generators\conanbuild.bat

cmake --preset conan-default

CALL conda deactivate

:END
