<p align="center"><img src="Assets/Textures/ShiftIcon.svg" align="center" width=256></p>

<h1 align="center">Shift: A Rendering Sandbox for Computer Graphics Research</h1>

Shift is a modern rendering engine based on Vulkan API
that was designed for the graphics programmers to learn, practice, research, and optimize various rendering techniques.

You can learn watch Shift rendering demo videos and images on the [webpage](https://shift.notionlinker.com/)!

---

### Prerequisites

1. Have a Windows 10/11.
2. Download the [MSVC 2022 compiler](https://visualstudio.microsoft.com/vs/features/cplusplus/) (is the only one supported as of Shift 1.0).
2. Download the [LunarG Vulkan SDK](https://www.lunarg.com/vulkan-sdk/).
3. Set the VULKAN_SDK_PATH to your SDK install directory.

---

### Building
**Using Clion:**
1. Open Shift as a new project and set the compiler to MSVC 2022 and build. Shift's CMake will do the rest.

**Using MSVC 2022:**
1. Generate the .sln with `mkdir Solution; cmake ..`.
2. Open the .sln with the MSVC 2022.
3. Set **Shift** as a startup project and build.
4. _Pray, I Guess..._
---

*Note: There will be a lot of warnings when building Assimp so don't get too scared:D*

After running Shift you should get a skeleton:

![img.png](./Assets/Renderings/Skull.png)

If that is so, congrats! You are now running Shift!

### Screenshots

![](./Assets/Renderings/SponzaaaaaaaImage1.png)
![](./Assets/Renderings/CameraRendering.png)
![](./Assets/Renderings/SkeletonCropped1.png)
![](./Assets/Renderings/Deagle2.png)
![](./Assets/Renderings/TacticalHelmetImage2.png)
![](./Assets/Renderings/SkullTrophy1Conf.png)

*More demos, screenshots, additional documentation, and credits to 3D modelers are on the Shift [webpage](https://shift.notionlinker.com/)!*

### Credits
- [Ostap "Ade0n" Trush](https://github.com/Adeon18): Development.
- [Danylo Rudenko](https://github.com/danylorudenko): Mentorship.