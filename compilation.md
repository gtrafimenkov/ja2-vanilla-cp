## Build on Linux using GCC

On Ubuntu or Debian:

```
sudo apt-get install gcc g++ libsdl2.0-dev
make clean
make -j4
```

## Build Windows binary on Linux

```
sudo apt-get install gcc-mingw-w64 g++-mingw-w64
make clean
make build-win-on-linux
```

## Build on Mac OS

These instructions might be outdated.

Install Xcode and Xcode command line tools.

```
make clean
make build-on-mac
```
