#!/bin/bash
#-------------------------------------------------------------------------------
#This file is part of nsg-library.
#http://github.com/woodjazz/nsg-library
#
#Copyright (c) 2014-2016 Néstor Silveira Gorski
#
#-------------------------------------------------------------------------------
#This software is provided 'as-is', without any express or implied
#warranty. In no event will the authors be held liable for any damages
#arising from the use of this software.
#
#Permission is granted to anyone to use this software for any purpose,
#including commercial applications, and to alter it and redistribute it
#freely, subject to the following restrictions:
#
#1. The origin of this software must not be misrepresented; you must not
#claim that you wrote the original software. If you use this software
#in a product, an acknowledgment in the product documentation would be
#appreciated but is not required.
#2. Altered source versions must be plainly marked as such, and must not be
#misrepresented as being the original software.
#3. This notice may not be removed or altered from any source distribution.
#-------------------------------------------------------------------------------

source /opt/poky/2.2/environment-setup-i586-poky-linux
cd $( dirname $0 ) # Ensure we are in project root directory
set -e
SOURCE_FOLDER="$PWD"

if [ ! -n "$1" ]; then
	echo "Enter build target directory. (directory will be created on the parent directory of the current one"
	exit 0
fi

cd ..
cmake -E make_directory $1
cd $1

#cmake $SOURCE_FOLDER -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="Debug"

#In order for find_package to be successful, Qt 5 must be found below the 
#CMAKE_PREFIX_PATH, or the Qt5<Module>_DIR must be set in the CMake cache 
#to the location of the Qt5WidgetsConfig.cmake file. The easiest way to use 
#CMake is to set the CMAKE_PREFIX_PATH environment variable to the install 
#prefix of Qt 5.
#(see http://doc.qt.io/qt-5/cmake-manual.html)

cmake $SOURCE_FOLDER -G "Unix Makefiles" \
-DCMAKE_SYSROOT="$SDKTARGETSYSROOT" \
-DCMAKE_BUILD_TYPE="Debug" \
-DOE_QMAKE_PATH_EXTERNAL_HOST_BINS="/opt/poky/qt5/sysroots/x86_64-pokysdk-linux/usr/bin/qt5" \
-DCMAKE_PREFIX_PATH="$HOME/Qt/5.7/gcc_64/lib/cmake/Qt5Widgets;$HOME/Qt/5.7/gcc_64/lib/cmake/Qt5Quick"

#cmake $SOURCE_FOLDER -G "Unix Makefiles" \
#-DCMAKE_SYSROOT="$SDKTARGETSYSROOT" \
#-DOE_QMAKE_PATH_EXTERNAL_HOST_BINS="/opt/poky/qt5/sysroots/x86_64-pokysdk-linux/usr/bin/qt5" \
#-DCMAKE_PREFIX_PATH="$HOME/Qt/5.7/gcc_64/lib/cmake/Qt5Widgets;$HOME/Qt/5.7/gcc_64/lib/cmake/Qt5Quick"


#cmake $SOURCE_FOLDER -G "Unix Makefiles"
