# Ray

![Screenshot of the program in use](https://reverboi.com/archive/pic1.png)

A rasterizer for your terminal!
### Table of Contents
- [About this project](#about)
- [Compiling](#compiling)
- [Usage](#usage)

### About this Project

I built Ray without reallly knowing what a rasterizer was, one day i just convinced myself i probably knew enough geometry and programming to make a "game engine" type-thing. Not a fully fledged one of course, just something you could "take a walk" in, and why not make it render directly in the terminal using something like `ncurses`? I quickly realized if I wanted this thing to be half-decent I actually needed to learn a lot more, mainly about multi-threading, concurrency, sphere-projection, matrix manipulation, data sharing, data types, rotations in 3D space and of course get a bit more lost in the good old C++ manual pages and all its weirdness.
All of this to say i use this project as a playground to learn what I'm intrested in or what i need in order to build what I'm intrested in and generally get a deeper understanding of the C++ languager as a whole.

### Compiling

These instructions assume you're using a debian-based linux distribution and the apt package manager, if you're savy enough to be running anything else you probrably already know what to do here anyways.

First, clone the repository:

```bash
git clone https://github.com/reverboi/ray.git
cd ray
```
Now get the necessary dependencies:
```bash
sudo apt update
sudo apt install libevdev-dev libudev-dev libncurses-dev
```
Now we're ready to actually compile the project!
Using your preferred terminal-shell-emulator-command-line-interface-whatever-you-wanna-call-it, navigate to the directory where the compilation will take place, i usually make a dirctory named "build" (how original!) inside the root directory of the project (alongside tis README.md). Navigate to that directory, type ```cmake ..``` and them ```make```
Summarizing, starting from the root directory of the project, do this:

```bash
mkdir build
cd build
cmake ..
make
```
### Usage
You will find that the compilation process has spawned a file named ```Ray``` in your build directory.
Always from the terminal run it by typing:

```bash
sudo ./Ray
```
The sudo privilege is necessary for reading raw keyboard input.

You should now find yourself in what resembles a 3D room with a big ball made of triangles in front of you.

You can walk with the `wasd` keys and look around with the arrow keys. You can also jump by pressing the `Space` key.

You can exit the program by sending a kill signal, usually `Ctrl+C`.

That's all, have fun!
