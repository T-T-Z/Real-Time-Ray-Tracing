# Real-Time-Ray-Tracing
Simple implementation of importance sampling for real time ray tracing

![rayTraceVision](https://github.com/user-attachments/assets/be63549b-8c1b-44fe-9926-605185754d2a)

10 samples, 5 bounces and denoising ^

The goal of this implementation is to have a realtime implementation of pathtracing with as little overhead as possible.
Realistic light interactions are not the main aim so this won't look as good as other implementations due to the aproximations and shortcuts (for now).
The main optimisation is the importance sampling used. The concept is to identify what areas will be harder to be lit using a simple direct lighting implementation and prioritising the darker areas.
This creats a map that looks like this:

![importanceMap](https://github.com/user-attachments/assets/a537483f-eb47-46f7-8fba-658084ec0843)

The more red an area is the more rays will be sent there. The darker areas on the other hand will have less.
The reason that it still looks well lit is because the direct lighting is used to light the low priority areas for the most part meaning that far fewer rays are needed to have accurate looking lighting.
This optimisation keeps most benefits like light distortion through transparent materials as well as more realistic diffuse lighting while needing far fewer samples to light up the full scene.

Note: The lighting effect under the triangle mirror is not a bug it's because the floor is not level with it (intentonally) to test thin slit lighting behaviour
