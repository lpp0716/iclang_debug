# IClang test suite

> Build target `clang` and `iclang-funcv` first.

> Now we only support Linux and WSL, and make sure your system has `/usr/bin/clang++`.

Just run `./test.sh` to test the entire suite.

* If all test cases pass, you will see 'All pass' in terminal and the return code of `test.sh` should be 0.

* If the test fails, please check the terminal log to determine which test case caused the issue and contact the IClang developer.

If you want to run a single test case, `cd` to the corresponding directory and run `./test`. 

For example, if you want to test the functionality of driver init, just run:

```shell
cd driver/init
./test.sh
```

The default LLVM bin path is `../../../llvm/cmake-build-release/bin`, 
you can change it by setting environment variable `MYBINPATH`, 
make sure to use absolute path, for example:

```shell
MYBINPATH="/home/hzy/hzydata/projects/llvm-project/llvm/cmake-build-debug/bin" ./test.sh
```

For WSL user, if you build the project with WSL toolchain on Windows 
(your compilation products are `clang.exe` and `iclang-funcv.exe`),
and test the project on WSL Linux environment,
you should set environment variable `WSL` to 1, for example:

```shell
WSL="1" ./test.sh
```