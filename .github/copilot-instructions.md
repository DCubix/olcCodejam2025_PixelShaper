<!-- Use this file to provide workspace-specific custom instructions to Copilot. For more details, visit https://code.visualstudio.com/docs/copilot/copilot-customization#_use-a-githubcopilotinstructionsmd-file -->
- [x] Verify that the copilot-instructions.md file in the .github directory is created.

- [x] Clarify Project Requirements
	<!-- C++ cross-platform CMake project for olcPixelGameEngine specified. -->

- [x] Scaffold the Project
	<!--
	Ensure that the previous step has been marked as completed.
	Call project setup tool with projectType parameter.
	Run scaffolding command to create project files and folders.
	Use '.' as the working directory.
	If no appropriate projectType is available, search documentation using available tools.
	Otherwise, create the project structure manually using available file creation tools.
	-->

- [x] Customize the Project
	<!--
	Cross-platform CMake configuration completed for Windows/Linux/macOS.
	Example application with basic olcPixelGameEngine features created.
	Project structure established with proper platform detection.
	-->

- [x] Install Required Extensions
	<!-- No specific extensions required for this CMake project. -->

- [x] Compile the Project
	<!--
	Project successfully compiled with CMake.
	Fixed C++ standard to C++17 for olcPixelGameEngine compatibility.
	Executable built at build/bin/Debug/olcPixelGameEngine_Project.exe
	-->

- [x] Create and Run Task
	<!--
	Created VS Code tasks.json with CMake configuration, build, clean, and run tasks.
	Default build task configured for easy building with Ctrl+Shift+P > Tasks: Run Build Task.
	-->

- [x] Launch the Project
	<!--
	Project successfully launches. Executable tested and runs properly.
	Application creates window and displays animated graphics as expected.
	-->

- [x] Ensure Documentation is Complete

## Project Summary
This is a complete cross-platform CMake project for olcPixelGameEngine development. The project includes:

### Core Components
- **CMakeLists.txt**: Cross-platform build configuration with automatic platform detection
- **src/main.cpp**: Example application demonstrating olcPixelGameEngine features
- **src/olcPixelGameEngine.h**: Complete graphics engine header (277KB)
- **README.md**: Comprehensive documentation and build instructions
- **.vscode/tasks.json**: VS Code development tasks

### Platform Support
- **Windows**: WinAPI integration, MSVC/MinGW support
- **Linux**: X11 libraries, GCC/Clang support  
- **macOS**: GLUT framework support

### Development Workflow
1. Use VS Code Command Palette (Ctrl+Shift+P)
2. Run "Tasks: Run Build Task" to build
3. Use "Tasks: Run Task" → "Run Application" to test
4. Modify `src/main.cpp` for your own applications

### Build Requirements
- CMake 3.16+
- C++17 compatible compiler
- OpenGL support
- Platform-specific libraries (automatically detected)

The project is fully functional and ready for olcPixelGameEngine development. The example application creates an animated graphics window demonstrating basic rendering capabilities.
	<!--
	README.md created with comprehensive build instructions for all platforms.
	CMakeLists.txt includes detailed comments and platform detection.
	VS Code tasks configured for easy development workflow.
	Project structure documented and example application provided.
	-->
