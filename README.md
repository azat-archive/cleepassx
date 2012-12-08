keepassx 0.4.3 src with CLI added. CLI uses GNU readline and allows to navigate over database imitating regular filesystem, though not literally.

You can launch resulting binary as usually, getting ordinary keepassx. You may launch it with «-cli» option, then it will not use X-server.

As for now files Cli.h and Cli.cpp should cause compilation errors in Visual Studio, they've been compiled only with gcc in linux.

Build & run instructions:
========================

go to the root of the source tree, then

`$ mkdir build && cd build`

and

`$ cmake .. && make && ./src/keepassx -cli ~/passwords.kdb`

or

`$ qmake .. && make && ./build/keepassx -cli ~/passwords.kdb`

(replace `~/passwords.kdb` with anything you need)

The cli uses GNU readline, so its devel package has to be installed. I use libreadline6-dev from ubuntu 12.04. And certainly you need cmake or qmake, make and gcc :) This project uses no more gcc-specific than keepassx as well as it does not use C++11 or any other newest features, so at least any gcc4 up to 4.6 is suitable.

Usage
=====
You can press Tab twice or enter «?» to get help from the interface.
