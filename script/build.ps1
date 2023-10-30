cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE="A:\vcpkg\scripts\buildsystems\vcpkg.cmake" -G Ninja -B build -S .
Copy-Item -Path ./build/compile_commands.json -Destination ./compile_commands.json -Force
