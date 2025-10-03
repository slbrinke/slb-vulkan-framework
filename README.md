# slb-vulkan-framework

This simple framework is set up to render small scenes in vulkan.

The rendering library started out based on the vulkan tutorial by Alexander Overvoorde: https://vulkan-tutorial.com/
and is successively extended to test out different rendering techniques.

Currently a scene containing simple meshes, materials and light sources, arranged in a scene graph hierarchy, can be defined. The assets displayed in the default scene are by Charles Nderitu on Polyhaven.
A simple forward renderer or a deferred renderer can be used to visualize the scene.
The shaders implement physically-based shading based on:
Burley, Brent, and Walt Disney Animation Studios. "Physically-based shading at disney." Acm siggraph. Vol. 2012. No. 2012.