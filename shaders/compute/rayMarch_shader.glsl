#version 430 core

// Work group size of 16x16
layout(local_size_x = 16, local_size_y = 16) in;

layout(location = 0) uniform mat4 viewMatrix;   // View matrix
layout(location = 1) uniform mat4 projMatrix;   // Projection matrix
layout(location = 2) uniform vec3 position;   // Projection matrix

// Texture input (bind to texture unit 0)
layout (rgba8, binding = 0) uniform image2D screenTexture;  // Binding point 0

// Constants and precision
const int MAX_MARCHING_STEPS = 256;
const float MIN_DIST = 0.0;
const float MAX_DIST = 100.0;
const float PRECISION = 0.001;
const float infty = 9999999.0;
const float aspectRatio = 4/3;

int iterCounter = 0;

// Structure to hold surface data
struct surf {
    float sd; // Signed distance
    int mat;  // Material ID
    vec3 col;
};

struct ray {
    vec3 col;
    vec3 dir;
    vec3 point;
};

// Function to return the minimum of two surfaces
surf minSurf(surf s1, surf s2) {
    if (s2.sd < s1.sd) return s2;
    return s1;
}

// SDF Definitions

// Sphere SDF
float sdSphere(vec3 p, float r, vec3 offset) {
    return length(p - offset) - r;
}

// Plane SDF
float sdPlane(vec3 p, vec3 n, float h) {
    return dot(p, n) + h; // n must be normalized
}

// Box SDF
float sdBox(vec3 p, vec3 b, vec3 offset) {
    p = p - offset;
    vec3 q = abs(p) - b;
    float d = length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
    return d;
}

// Scene SDF
surf sdScene(vec3 p) {
    surf scene = surf(sdPlane(p, vec3(0.0, 1.0, 0.0), 2.5), 1, vec3(1));
    scene = minSurf(scene, surf(sdSphere(p, 1.0, vec3(-0.8, 0.0, 3.0)), 0, vec3(1,0,0)));
    scene = minSurf(scene, surf(sdBox(p, vec3(1.0), vec3(1.5, 0.0, -3.0)), 3, vec3(0,0,1)));
    iterCounter++;
    return scene;
}

float GetDistanceToNearestSurface(vec3 p){
    return sdScene(p).sd;
}

vec3 GetSurfaceNormal(vec3 p)
{
    float d0 = GetDistanceToNearestSurface(p);
    const vec2 epsilon = vec2(.0001,0);
    vec3 d1 = vec3(
        GetDistanceToNearestSurface(p-epsilon.xyy),
        GetDistanceToNearestSurface(p-epsilon.yxy),
        GetDistanceToNearestSurface(p-epsilon.yyx));
    return normalize(d0 - d1);
}

surf fireRay(inout ray marchRay, float end, float w){
    float rp = 0.0; // previous r
    float ri = 0.0; // current r
    float rn = infty; // next r
    float d = 0.0;  // accumulated distance
    float t = 0.0; // current position along the ray
    surf co;  // closest object
    for (int i = 0; i < MAX_MARCHING_STEPS; i++) {
        co = sdScene(marchRay.point + (t + d) * marchRay.dir);  // Sample the scene
        rn = co.sd;  // Get the signed distance at the current position
        
        // Acceleration condition
        if (d > ri + rn) {
            d = ri;
            co = sdScene(marchRay.point + (t + d) * marchRay.dir);
            rn = co.sd;
        }
        
        t += d;  // Move the ray forward by the distance d
        rp = ri;  // Store previous distance
        ri = rn;  // Update current distance
        d = ri + w * ri * (d - rp + ri) / (d + rp - ri);  // Update distance using acceleration factor
        
        if (rn < PRECISION && i > 1) {
            marchRay.point += (t) * marchRay.dir;
            return co;
        }
        if (t > end){
            co.sd = end;
            co.col = vec3(0);
            marchRay.point += (t) * marchRay.dir;
            return co;
        }
    }
}

// Accelerated Ray Marching
surf rayMarch(vec3 ro, vec3 rd, float start, float end, float w) {
    surf co;  // closest object
    ray marchRay;
    marchRay.point = ro;
    marchRay.dir = rd;
    vec3 col = vec3(0);
    //co = fireRay(marchRay, end, w);
    for(int i = 0; i < 1; i++){

        co = fireRay(marchRay, end, w);
        col += (1.0/(float(i)+1.0))*co.col;
        if (co.sd >= end){
            return co;
        }
        vec3 norm = GetSurfaceNormal(marchRay.point);
        marchRay.dir = reflect(marchRay.dir, norm);
    }
    co.col = col;
    //co.sd = marchRay.dist;
    //co.col = marchRay.dir;
    return co;
}

void main() {
    // Get the fragment coordinates (pixel location in the image)
    ivec2 fragCoord = ivec2(gl_GlobalInvocationID.xy);
    
    // Convert screen coordinates to NDC (Normalized Device Coordinates)
    vec2 texSize = imageSize(screenTexture);
    vec2 ndc = (fragCoord / texSize) * 2.0 - 1.0;  // Convert from pixel to NDC (-1 to 1)
    // Apply aspect ratio correction
    ndc.x *= aspectRatio; // Correct for aspect ratio

    // Convert from NDC to clip space
    // NDC -> Clip Space (Z=-1, W=1)
    vec4 clipSpacePos = vec4(ndc, -1.0, 1.0); // Setting Z=-1 for the near plane and W=1

    // Convert from clip space to view space by multiplying with the inverse projection matrix
    vec4 viewSpacePos = inverse(projMatrix) * clipSpacePos; // Apply inverse projection matrix
    viewSpacePos /= viewSpacePos.w; // Perspective divide to get normalized coordinates in view space

    // Convert from view space to world space by multiplying with the inverse view matrix
    vec3 rayDir = normalize((inverse(viewMatrix) * viewSpacePos).xyz); // Normalize to get ray direction

    vec3 rayOrigin = position; // Camera position (translation part of the inverse view matrix)
    
    surf co = rayMarch(rayOrigin, rayDir, MIN_DIST, MAX_DIST, 0.6);

    // Write the color to the screenTexture
    //imageStore(screenTexture, fragCoord, vec4(float(co.iter-1)/float(MAX_MARCHING_STEPS),0,0, 1.0));
    //imageStore(screenTexture, fragCoord, vec4(float(iterCounter)/float(MAX_MARCHING_STEPS),0,0, 1.0));
    imageStore(screenTexture, fragCoord, vec4(co.col, 1.0));
    //imageStore(screenTexture, fragCoord, vec4(co.sd/25.0,0,0, 1.0));
}