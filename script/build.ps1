cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja -B build -S .
Copy-Item -Path ./build/compile_commands.json -Destination ./compile_commands.json -Force
