# Debug Build Issues


## Run cmake under debugger

```
mkdir build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE --no-warn-unused-cli -S . -B build/ -G "Visual Studio 17 2022" -T host=x64 -A x64 --debugger
```

> You could use --debugger-pipe command line argument as appropriate.
> VS Code uses something like `--debugger-pipe \\.\\pipe\\cmake-debugger-pipe\\12345678-1234-1234-1234-123456789012`