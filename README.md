# franks-drawing-library

Dev-Cpp project with C++ source files, headers, SDL2 dlls, all showcasing a feature-rich and straight-forward way to rendering 2D and 3D graphics as quickly as possible. Don't forget to add the following linker flags directly in the "compiler options" submenu:

```
  -lopengl32
  -lglu32
  -lSDL2main
  -lSDL2
  -lSDL2_image
  -static-libgcc
  ```
  
Also, don't forget to double-check the Project Options and ensure the C++11 standard is enabled (Project Options->Compiler->Code Generation->Language Standard (-std)=ISO C++11

Possibly more to come later, but this version is more-or-less complete. -- Frank, May 12, 2020
