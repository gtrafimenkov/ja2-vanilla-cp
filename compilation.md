## Build on Linux using GCC

On Ubuntu or Debian:

```
sudo apt-get install gcc g++ libsdl2.0-dev
make clean
make -j4
```

## Build on Linux binary for Windows

```
sudo apt-get install gcc-mingw-w64 g++-mingw-w64
make clean
make build-win-on-linux
```

## Build on Windows

Open file _build\solution-vs\ja2.sln with Visual Studio Community edition and build the project.

or:
- install Cygwin enviroment (www.cygwin.com)
- install MinGW from http://sourceforge.net/projects/mingw to folder c:\MinGW
- from the Cygwin shell, execute: ```$ make build-on-win```


## Build on Mac OS

These instructions might be outdated.

Install Xcode and Xcode command line tools.

```
make clean
make build-on-mac
```
