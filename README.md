# Voxellium
 Another Minecraft-like clone attempt, using Vulkan, Multithreading, and more.

## Info
 This is a voxel game. The intent is to mess around with different ideas I had
 about optimizing and organizing a voxel renderer. Some engine changes/improvements
 haven't been added to the base engine (Basalt) repo yet. Goes without saying, but
 without MoltenVK and other complexities, this won't run on Macs. And even moreso,
 your computer needs to support Vulkan in the first place.

## How to run
 The necessary things to install are:

 1. The Vulkan SDK [(From LunarG's website)](https://vulkan.lunarg.com).
 2. Conan Package Manager [Link](https://conan.io/downloads.html).
 3. CMake > you find the link lol.
 4. A C++ compiler that supports C++20, like VS or *maybe* g++ & clang.

### Recommended

 1. I personally use VSCode, with the CMake extension
 2. Use the Github Desktop App if you aren't familiar with git
 3. Update Graphics Drivers on your computer.
 
### Actually Running

 1. Setup CMake
 2. In the build directory (/build), run ```conan install <path_to_conanfile.txt>``` or ```conan install ..``` if just in the parent directory.
 3. Reconfigure CMake again so it detects the conan packages.
 4. Build! Hurrah!
 5. To run, make sure the executable can see the "res/" folder. Recommeded to have the cwd (working/environment directory) set for the launching.
 6. And that should be it! Little to no support will be provided, since this is in rapid / very active development. 

