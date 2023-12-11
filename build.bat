mkdir build
cd build
conan install .. --output-folder=build --build=missing
cmake .. -G "Visual Studio 16 2019" -DCMAKE_CONFIGURATION_TYPES="Release" -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
cmake --build . --config Release
cd ..

