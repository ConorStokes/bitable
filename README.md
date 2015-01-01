## Bitables ##

Bitables are a data structure that is a hybrid of an SSTable and a b+ tree. They are an immutable, ordered, indexed data structure designed for fast range and point queries, being merged etc. They are described [here](http://conorstokes.github.io/data%20structures/2015/01/01/introducing-bitables/).

This library is the reference implementation, written in C and using memory mapped I/O. 

## Building ##

Premake4 is used for generating build files for different platforms (although, currently Windows and Visual Studio 2013 and Linux/GCC/Ubuntu are the only ones that have been testing). The Windows Premake4 binary used for testing is included. On Ubuntu, the Premake4 package from the apt-get repository was used.

There are 4 build configurations (combinations of debug/release and static library/shared library) and 2 platforms (x86 and x64). For gmake, these configurations are releaselib64, releasedll64, debuglib64, debugdll64, releaselib32, releasedll32, debuglib32 and debugdll32.

To build the static lib release x64 version with make on Linux (from the repository directory):

	premake4 gmake
	cd gmake
	make config=releaselib64

On Windows, you can produce a Visual Studio 2013 file (in the vs2013 subdirectory) using the below:

	premake4 vs2013

## Example ##

The library is C, but there is an example included in C++ which shows how to use the library.