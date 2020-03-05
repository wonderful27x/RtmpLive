# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
file(GLOB RTMP_SRC *.c)

#纯c代码需要用CMAKE_C_FLAGS，否则用CMAKE_CXX_FLAGS
#-DNO_CRYPTO是添加“NO_CRYPTO”预编译宏的意思，-D是添加宏的选项
#这里添加这个宏是为了关闭openssl，当然也可以下载openssl源码一起编译就不会报错了
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_CRYPTO")

add_library( # Sets the name of the library.
        rtmp

        # Sets the library as a shared library.
        STATIC

        # Provides a relative path to your source file(s).
        ${RTMP_SRC})