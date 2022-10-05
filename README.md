# Aurora

Aurora is a visualisation system for the [looking glass](https://lookingglassfactory.com/) that I wrote 2020 for my bachelors thesis.
It is able to render 4-dimensional signed distance fields using projection onto the autostereoscopic display.
While doing that, it incorporates lights, shadows and surface texture using a novell rendering algorithm. 

![Image of hypercube rendering](https://www.jan-gihr.com/img/aurora_hypercube.png)

## Code samples
[Rendering/ApplicationCUDA.cu](/Rendering/ApplicationCUDA.cu): rendering with CUDA\
[MarchingFunctions.h](/Marching/MarchingFunctions.h) (bi-)raymarching algorithm implementation\
[Rendering/Application.cpp](/Rendering/Application.cpp): multithreading architecture, interface to looking glass\
[MathLib/SignedDistanceFields](MathLib/SignedDistanceFields): SDF implementations used to represent 4D structures\

## About the project
For my bachelor's thesis, I investigated how shading and texturing of 4D structures could be implemented in a renderer.
The goal was to test the impact of these spatial vision cues on the understanding of the perceived 4D structures. 

### Rendering for an autostereoscopic 3D Display
The display I used was the Looking Glass. 
To achieve a 3D image that can be seen from multiple angles, I needed to render the scene from multiple viewpoints, mimicking the spots the observers can stand in.
From that, I could create a big texture and pass it to the looking glass API, which makes a dithered result image from the big texture. 
Depending on the view angle, differently ionised glass panes in the display will redirect your view so that you only see the right parts of the dithered texture to create a holographic effect.

### Rendering a 4D structure
4D structures are represented with signed distance fields.
Existing approaches took just one slice of a 4D structure and rendered that in 3D. 
As the idea was to determine the ability to perceive a 4D structure in total, I wanted to project the 4D sturcture onto the 3D display, with only one "compressed" dimension.
To achieve this, I still needed to create multiple 2D images from different view points from the 4D scene. 
This is achieved by a modified version of raymarching, but instead of travelling along a ray, we travel along a plane (_biray_). 
Each pixel from the resulting image sends one _biray_, so it covers two dimensions worth of data. 
This approach was chosen over using two projection matrices as raymarching allows for the light and shadow calculation needed for the thesis. 

### Performance with CUDA
In the end, I needed to render a scene 45 times per frame, from 45 different angles, all while using the new and more expensive biraymarching algorithm.
To still achieve an interactive application, I used CUDA for the rendering algorithm. This caused a speedup of ~500% over the previous software thread approach. 
