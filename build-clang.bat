@echo off
REM Build PetusMod with Ninja + clang-cl inside the MSVC environment.
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
set "GEODE_SDK=C:\Users\dimsk\geode-sdk"
set "PATH=C:\Program Files\LLVM\bin;C:\Users\dimsk\AppData\Local\Microsoft\WinGet\Links;%PATH%"
set "CPM_SOURCE_CACHE=C:\Users\dimsk\.cpmcache"

cd /d "C:\Users\dimsk\Downloads\Petus\PetusMod"
rmdir /s /q build 2>nul

cmake -B build -G Ninja ^
  -DGEODE_BINDINGS_REPO_PATH=C:/Users/dimsk/geode-bindings ^
  -DGEODE_CLI="C:/Users/dimsk/AppData/Local/Microsoft/WinGet/Packages/GeodeSDK.GeodeCLI_Microsoft.Winget.Source_8wekyb3d8bbwe/geode.exe" ^
  -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
  -DCMAKE_C_COMPILER=clang-cl ^
  -DCMAKE_CXX_COMPILER=clang-cl ^
  -DCMAKE_LINKER=lld-link ^
  || exit /b 1

cmake --build build --config RelWithDebInfo || exit /b 1
echo BUILD_OK
