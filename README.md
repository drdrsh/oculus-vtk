# A boiler-plate for OculusVR and VTK in C++

This is a Cmake-based OpenGL boilerplate for applications that use OculusVR and VTK. My intention was to allow the user the freedom to use modern OpenGL features and still be able to benefit from VTK features.

This framework uses GLEW to load OpenGL, GLFW to create OpenGL Window and loads GLM for your mathematical needs.

VTK is used to build vtkPolyData and do all kinds of operation on these polyData (decimation, smoothing, etc). These vtkPolyData are then sent to a vanilla OpenGL shader (Version 4.0) along with the Oculus transformation matrices for rendering.
