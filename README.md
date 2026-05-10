# S0LARI5

An interactive 3D solar system sandbox written in C with OpenGL. Build your own star system by placing planets and moons around a central star, customize their textures and sizes, then turn on gravity and watch everything orbit.

![Language](https://img.shields.io/badge/language-C-blue)
![Graphics](https://img.shields.io/badge/graphics-OpenGL-5586A4)

## Features

- First-person free-flight camera through a fully 3D space environment
- Place satellites that orbit any selected parent body, building hierarchical systems (moons around planets, planets around stars)
- 31 textures to choose from, including real solar system bodies (Earth, Mars, Jupiter, Saturn...) and fictional worlds
- Per-fragment Phong shading with multiple colored point lights
- Mouse picking via ray-box intersection to select bodies in the scene
- Toggleable gravity that animates rotations and orbital motion with speed scaled by distance and size

## Controls

| Key            | Action                                  |
|----------------|-----------------------------------------|
| `W` `A` `S` `D` / Arrows | Move camera                   |
| Mouse          | Look around                             |
| Left click     | Select a planet                         |
| `F`            | Create a new satellite around selection |
| `R`            | Remove selected satellite (and children) |
| `T` / `G`      | Increase / decrease satellite size      |
| `U`            | Cycle through textures                  |
| `M`            | Enable gravity (start the simulation)   |
| `Esc`          | Exit                                    |

## Building

Requires:
- A C compiler (GCC / Clang)
- OpenGL
- [GLEW](http://glew.sourceforge.net/)
- [FreeGLUT](http://freeglut.sourceforge.net/)

Compile manually:

```bash
gcc ProyectoFinal.c Mat4.c Sphere.c Transforms.c Utils.c Vec3.c Vec4.c \
    -o solaris -lGL -lGLEW -lglut -lm
```

The project also includes Eclipse CDT files (`.cproject`, `.project`) — a prebuilt binary and makefile live in `Debug/`.

Run from the project root so the relative paths to `shaders/` and `textures/` resolve correctly:

```bash
./solaris
```

## Project structure

```
.
├── ProyectoFinal.c    Main program: window, input, scene graph, render loop
├── Sphere.{c,h}       Procedural UV sphere generation and drawing
├── Mat4.{c,h}         4x4 matrix type
├── Vec3.{c,h}         3-component vector type
├── Vec4.{c,h}         4-component vector type
├── Transforms.{c,h}   Matrix transforms (translate, rotate, scale, perspective)
├── Utils.{c,h}        Shader compilation and BMP texture loading
├── shaders/           GLSL vertex and fragment shaders (Phong lighting)
└── textures/          BMP planet and skybox textures
```

## How it works

Each placed body is a `Planet` struct that records its parent, local position, scale, and texture. On each frame, the render loop walks the array and composes each body's model matrix from its parent's matrix, producing nested orbits. Selection uses a ray cast from the camera through the mouse position, tested against each body's bounding box.

Lighting is uploaded as a uniform buffer of three colored spotlights and evaluated per-fragment in `shaders/phong.fsh`.
