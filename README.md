# franks-drawing-library

I wrote two projects that showcase the bare essentials with getting graphics applications up and running using GCC/MinGW. In my case specifically, all code was written in the freeware Dev-C++ IDE. To compile, the following flags must be used for the linker on the command-line:
```
  -lopengl32
  -lglu32
  -lSDL2main
  -lSDL2
  -lSDL2_image
  -static-libgcc
  ```
The compiler must also be told to compile with the ISO C++11 standard (-std=C++11) this has been enabled within the project file.

(In Dev-C++ under the Project->Project Options menu, and ensure the ISO C++11 standard is enabled: Project Options->Compiler->Code Generation->Language Standard (-std)=ISO C++11 ).

The Makefile(s) should have this information contained, but I have since updated the Dev-CPP project files to include these settings, as well.

Possibly more to come later, but this version is more-or-less complete. -- Frank, May 12, 2020
