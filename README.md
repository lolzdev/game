This is a work-in-progress voxel engine

## Building
First you need to initialize the submodules to pull external dependencies with
```sh
git submodule init
```
then you also need to pull the dependencies for `shaderc`, you can do so with
```sh
./ext/shaderc/utils/git-sync-deps
```

Now you can create a new directory to build the project in using CMake.
```sh
mkdir build
cd build
cmake ..
make
```
