# A boiler-plate for OculusVR and VTK in C++

This is a Cmake-based OpenGL boilerplate for applications that use OculusVR and VTK. My intention was to allow the user the freedom to use modern OpenGL features and still be able to benefit from VTK features.

This framework uses GLEW to load OpenGL, GLFW to create OpenGL Window and loads GLM for your mathematical needs.

VTK is used to build vtkPolyData and do all kinds of operation on the vtkPolyData like decimation, smoothing, filling holes, etc. The vtkPolyData along with attached "arrays" (e.g. Normals) are then sent to a vanilla OpenGL shader (Version 4.0) along with the Oculus transformation matrices for rendering.

This boilerplate includes a file watacher on windows that detects changes to the shaders code and reloads them in realtime. Using the latest OpenGL version allow using GPU debugging tools like Nvidia NSight.
