# NSEMU
Experimental Nintendo Switch emulator.

## Status
Still WIP project. You can get update via [My twitter](https://twitter.com/RKX1209).  
TODO list is [here](https://github.com/RKX1209/nsemu/wiki/TODO). 
## Building
```
git clone https://github.com/RKX1209/nsemu.git
cd nsemu
make -j
```

## Running
```
./nsemu <NSO file>
```
You can try some homebrew application like [libtransistor](https://github.com/reswitched/libtransistor).  

Network sample.Sending the string to localhost:5555.
```
./nsemu /path/to/libtransistor/build/test/test_bsd.nso
``` 
```
$ nc -l localhost 5555
Hello from libtransistor over a socket!
```

## Debugging
GDB mode.
```
./nsemu -s /path/to/nsofile
```
```
gdb-multiarch -q
gdb$ set architecture aarch64
gdb$ target remote localhost:1234
```
Debug logs.
```
./nsemu -d /path/to/nsofile
```
