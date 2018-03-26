# NSEMU
NSEMU is a Nintendo Switch Emulator. It allows you to play games on PC.
## Building
```
git clone https://github.com/RKX1209/nsemu.git
cd nsemu
make -j4
```

## Running
```
./nsemu <NSO file>
```
You can try some homebrew application like [libtransistor](https://github.com/reswitched/libtransistor).  

```
./nsemu /path/to/libtransistor/build/test/test_bsd.nso

``` 

## Debugging
Currently you should modify a run level flag to debugging mode.  
include/Util.hpp
```cpp
static RunLevel curlevel = RUN_LEVEL_DEBUG;
//static RunLevel curlevel = RUN_LEVEL_RELEASE;
```

