# Requirements
Direct3D12 capable hardware and OS (Windows 10 20H2 or newer)<br>
Windows 10 SDK or newer<br>
CMake 3.22 or newer (can use Visual Studio, VSCode or any other CMake supported IDE)<br>
DirectX Raytracing capable GPU and compatible drivers<br>

# Features
- Deferred Rendering
- Physically Based shading
- Image Based Lighting
- Cook-Torrance specular BRDF (w/ lambert diffuse)
- Gamma Correction and Tonemapping

# Showcase
### PBR+IBL
![alt text](showcase/pbr+ibl.png)
### Textured PBR+IBL
![alt text](showcase/pbr+ibl+textures.png)
### Damaged Helmet PBR+IBL
![alt text](showcase/pbr+ibl+helmet.png)
### Cerberus PBR+IBL
![alt text](showcase/pbr+ibl+cerberus.png)
### Gamma Correction and ACES Tonemap
| With | Without  |
|------|------|
|![alt text](showcase/with_tonemap.png)|![alt text](showcase/without_tonemap.png)|

# Resources
[Real Shading in Unreal Engine 4](https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf)<br>
[LearnOpenGL-PBR](https://learnopengl.com/PBR/Theory)<br>
[PBR by Michał Siejak](https://github.com/Nadrin/PBR)

# Third-Party
[gfx by Guillaume Boissé](https://github.com/gboisse/gfx)
