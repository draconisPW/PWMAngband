Compiling Instructions
======================

PWMAngband is usually built and run under Windows. If you're using Linux, you
can get the PWMAngband binaries directly and use WinE. Otherwise, a few
makefiles have been added to compile under Linux. Check the compiling.txt file
for instructions.

PWMAngband comes with various text files that explain how to compile the game
and several batch files that actually do the job.

PWMAngband uses Borland C++ 6.0 as compiler tool and IDE for coding and
debugging. For a free version, check Embarcadero C++ 7.20 from
https://www.embarcadero.com/free-tools/ccompiler.

How to build all libraries required to run the WIN client
---------------------------------------------------------

Just follow the instructions given in the WIN.txt file located in the /src
directory. This will generate the ZLIB and LIBPNG libraries using the WIN.bat
file in the same directory.
For Embarcadero C++ 7.20, follow the instructions given in the WIN.txt file
located in the /bcc32c subdirectory instead.

How to build all SDL libraries required to run the SDL client
-------------------------------------------------------------

Just follow the instructions given in the SDL.txt file located in the /src
directory. This will generate the SDL, FREETYPE, SDL_TTF, SDL_IMAGE, LIBMAD and
SDL_MIXER libraries using the SDL.bat file in the same directory.
For Embarcadero C++ 7.20, there is no easy way to compile the SDL libraries due
to incompatibility between the code and the compiler. There will be thousands
of compile errors that you'll have to fix manually.

How to build the PWMAngband client
----------------------------------

This is done by running the client.bat file in the /src directory. This will
generate the mangclient_gcu.exe, mangclient_sdl.exe and mangclient.exe
executable files corresponding to the GCU, SDL and WIN client.
For Embarcadero C++ 7.20, run the client.bat file located in the /bcc32c
subdirectory instead.

How to build the PWMAngband server
----------------------------------

This is done by running the server.bat file in the /src directory. This will
generate the mangband.exe executable file corresponding to the server.
For Embarcadero C++ 7.20, run the server.bat file located in the /bcc32c
subdirectory instead.

The clean.bat file will delete all generated binaries in case you want to
recompile everything from scratch.

How to build the HTML manual from the help files
------------------------------------------------

Just follow the instructions given in the make.txt file located in the /docs
directory. This will generate the Manual.html file using the make.bat file in
the same directory.

How to build the PWMAngband setup
---------------------------------

This is done by running the setup.bat file in the /src directory. This will
generate all the files required to run PWMAngband.

The cleansetup.bat file will delete all generated files in case you want to
regenerate everything from scratch.
