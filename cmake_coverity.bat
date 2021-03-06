::-------------------------------------------------------------------------------
::This file is part of nsg-library.
::http://github.com/woodjazz/nsg-library
::
::Copyright (c) 2014-2017 Néstor Silveira Gorski
::
::-------------------------------------------------------------------------------
::This software is provided 'as-is', without any express or implied
::warranty. In no event will the authors be held liable for any damages
::arising from the use of this software.
::
::Permission is granted to anyone to use this software for any purpose,
::including commercial applications, and to alter it and redistribute it
::freely, subject to the following restrictions:
::
::1. The origin of this software must not be misrepresented; you must not
::claim that you wrote the original software. If you use this software
::in a product, an acknowledgment in the product documentation would be
::appreciated but is not required.
::2. Altered source versions must be plainly marked as such, and must not be
::misrepresented as being the original software.
::3. This notice may not be removed or altered from any source distribution.
::-------------------------------------------------------------------------------

@echo off
pushd %~dp0
set SOURCE_FOLDER=%CD%

if "%1" == "" (
	echo "Enter build target directory. (directory will be created on the parent directory of the current one"
	@exit /b 1
)

call "%VS120COMNTOOLS%/vsvars32.bat"

cd ..
cmake -E make_directory %1
cd %1

cmake %SOURCE_FOLDER% -G "Visual Studio 12 Win64" -DCOVERITY="1"

::coverity build
cov-build --dir cov-int msbuild nsg-library.sln
