# > SYSTEM_LOG : GLITCHED_SERVER
> SECURITY_LEVEL : RED
> NODE_STATE : COMPROMISED
> EXECUTION_MODE : LIVE

Glitched Server is a FreeGLUT C++ game artifact engineered as a systems-focused graphics project.
It fuses low-level raster algorithms, real-time OpenGL rendering, and custom AI navigation into one executable loop.

## Core Technical Achievements

### 1) Custom 2D Rasterization Algorithms
- Bresenham line rasterization for deterministic integer-based line drawing.
- Midpoint circle rasterization for efficient circular primitives.
- Pixel-level rendering control without high-level shape helpers.

### 2) 3D OpenGL Rendering Pipeline
- Real-time rendering via FreeGLUT and OpenGL.
- Integrated update-render cycle for continuous simulation.
- C++ control over transforms, scene composition, and frame behavior.

### 3) Custom AI Pathfinding
- Hand-built pathfinding logic for autonomous movement.
- Real-time navigation decisions under map and gameplay constraints.
- AI behavior integrated directly into the main game loop.

## Engineering Value
Glitched Server demonstrates practical computer science depth:
- Algorithm implementation over black-box abstraction.
- Real-time loop design and state management.
- C++ architecture for graphics and gameplay coordination.

## How to Run

1. Clone the repository:
   git clone https://github.com/Mohammed-051/glitched-server.git
   cd "glitched-server"

2. Ensure your environment is ready:
   - Code::Blocks with MinGW installed.
   - FreeGLUT installed and configured in Code::Blocks include/lib settings.

3. Launch the project:
   - Double-click Glitched_Server.cbp
   - Or open it from inside Code::Blocks.

4. Build and execute:
   - Press Build and Run (F9).

## Operator Log
- Lead Developer: Mohammed Mahmud Parvez
- Contributors : Tahmidul Islam Mugdha, Rahul Roy Rik
