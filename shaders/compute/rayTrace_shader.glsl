#version 430 core

// Work group size of 16x16
layout(local_size_x = 16, local_size_y = 16) in;

layout(location = 0) uniform mat4 viewMatrix;   // View matrix
layout(location = 1) uniform mat4 projMatrix;   // Projection matrix
layout(location = 2) uniform vec3 position;   // Projection matrix
layout(location = 3) uniform uint frameNo;   // Projection matrix

// Texture input (bind to texture unit 0)
layout (rgba8, binding = 0) uniform image2D screenTexture;
layout (rgba8, binding = 1) uniform image2D normalTexture;

struct Ray {
    vec3 col;
    vec3 dir;
    vec3 point;
    float dist;
};

struct Material{
    int type;
    float opaqueness, smoothness, specularity;
    vec3 color;
};

struct Sphere{
    float radius;
    vec3 position;
    Material material;
};

struct Box{
    vec3 position, size, rotation;
    Material material;
};

const float aspectRatio = 4/3;
const float infinity = 99999999.;
const float PI = 3.14159;
float seed = 0;
int checks = 0;

//material: type, roughness, reflectance, color, emmision
Sphere balls[5] = Sphere[5](
    Sphere(1.0,vec3(1,1,0),Material(1, 1.0, 0.1, 0.0, vec3(0.2, 0.8, 0.5))),
    Sphere(1.0,vec3(-1,1,0),Material(1, 0.0, 1.0, 0.9, vec3(0.8,0.1,0.2))),
    Sphere(1.0,vec3(-6,1,0),Material(1, 1.0, 1.0, 0.5, vec3(0.2,0.1,0.9))),
    Sphere(5.0,vec3(15,1,0),Material(1, 0.3, 1.0, 0.9, vec3(0.2,0.1,0.9))),
    Sphere(1000.0,vec3(0,-1000,0),Material(1, 1.0, 0.0, 0.2, vec3(0.8,1.0,0.5)))
    //Sphere(1000.0,vec3(1000,0,0),Material(1, 0.0, 1.0, 1.0, vec3(0.8,1.0,0.5)))
);

Box boxes[1] = Box[1](
    Box(vec3(4,2,0),vec3(1),vec3(45.0,45.0,0.0),Material(1,1.0,1.0,0.3,vec3(0.8,0.2,1.0)))
);

Sphere sphereLightSources[3] = Sphere[3](
    Sphere(1.0,vec3(0,5,5),Material(0, 1.0, 0.1, 1.0, vec3(0.2,0.6,1.0))),
    Sphere(1.0,vec3(5,5,5),Material(0, 1.0, 0.1, 1.0, vec3(0.5,1.0,0.8))),
    Sphere(3.0,vec3(20,1,0),Material(0, 0.3, 1.0, 0.9, vec3(0.2,0.1,0.9)))
);

Box boxLightSources[2] = Box[2](
    Box(vec3(0,5,-5),vec3(1),vec3(45.0,45.0,0.0),Material(0, 1.0, 0.1, 1.0, vec3(0.8,0.4,0.8))),
    Box(vec3(5,5,-5),vec3(1),vec3(45.0,45.0,0.0),Material(0, 1.0, 0.1, 1.0, vec3(1.0,0.2,0.5)))
);

mat4 rotateY(float rotation){
    rotation = radians(rotation);
	float ys = sin(rotation);
	float yc = cos(rotation);
	float yoc = 1.0-yc;
	return mat4(yc,0.0,ys,0.0,
				0.0,yoc+yc,0.0,0.0,
				-ys,0.0,yc,0.0,
				0.0,0.0,0.0,1.0);
}

mat4 rotateX(float rotation){
    rotation = radians(rotation);
	float xs = sin(rotation);
	float xc = cos(rotation);
	float xoc = 1.0-xc;
	return mat4(xoc+xc,0.0,0.0,0.0,
			    0.0,xc,-xs,0.0,
				0.0,xs,xc,0.0,
				0.0,0.0,0.0,1.0);
}

mat4 rotateZ(float rotation){
    rotation = radians(rotation);
    float zs = sin(rotation);
    float zc = cos(rotation);
    float zoc = 1.0-zc;
	return mat4(zc,zs,0.0,0.0,
			    -zs,zc,0.0,0.0,
				0.0,0.0,zoc+zc,0.0,
				0.0,0.0,0.0,1.0);
}

/*
 * intersection functions
 * https://iquilezles.org/articles/intersectors
 */
 
vec4 boxHit(Ray ray,Box box) {
    mat4 translate = mat4(1.0,0.0,0.0,0.0,
                          0.0,1.0,0.0,0.0,
                          0.0,0.0,1.0,0.0,
                          box.position,1.0)*rotateX(box.rotation.x)*rotateY(box.rotation.y)*rotateZ(box.rotation.z);

    vec3 q = (inverse(translate)*vec4(ray.point,1.0)).xyz;
    vec3 m = 1.0/(inverse(translate)*vec4(ray.dir,0.0)).xyz; 
    vec3 n = m*q;  
    vec3 k = abs(m)*box.size;
    vec3 t1 = -n-k;
    vec3 t2 = -n+k;
    float tn = max(max(t1.x,t1.y),t1.z);
    float tf = min(min(t2.x,t2.y),t2.z);
    if(tn>tf||tf<0.0) return vec4(-1.0); //ray missed

    //vec3 normal = sign(q)*step(t1.yzx,t1.xyz)*step(t1.zxy,t1.xyz);

    vec3 normal = (tn>0.0) ? step(vec3(tn),t1) : // ro ouside the box
                           step(t2,vec3(tf));  // ro inside the box
    normal *= -sign(m);

        mat3 inverseRotation = inverse(transpose(mat3(rotateX(box.rotation.x) * rotateY(box.rotation.y) * rotateZ(box.rotation.z))));
    normal = inverseRotation * normal;

    return vec4(normal,(tn>0.0) ?tn:tf);
}

vec2 sphereHit(Ray Ray,Sphere sphere){
    // Vector from the ray origin to the sphere's center
    vec3 rc = Ray.point - sphere.position;

    // Compute the coefficients of the quadratic equation
    float b = dot(rc, Ray.dir);
    float c = dot(rc, rc) - pow(sphere.radius, 2.0);

    // Discriminant of the quadratic equation (b^2 - c)
    float t = pow(b, 2.0) - c;

    // If discriminant is positive, there are intersections
    if (t > 0.0) {
        // Calculate the two possible intersection distances (t1 and t2)
        float t1 = -b - sqrt(t);  // First intersection point
        float t2 = -b + sqrt(t);  // Second intersection point

        // If t1 is negative, the ray starts inside the sphere and t1 is the exit point
        // t2 is the entry point into the sphere
        if (t1 < 0.0) {
            return vec2(t2,-1.0);  // The ray is inside the sphere, t2 is the entry point
        }

        // If t1 is positive, return the nearest intersection in front of the ray
        return vec2(t1,1.0);  // Otherwise, t1 is the entry point into the sphere
    }

    // If no intersection, return a large negative value or other indication of no hit
    return vec2(-1.0,0);
}

vec3 triangleHit(Ray Ray, in vec3 v0, in vec3 v1, in vec3 v2, out vec3 n )
{
    vec3 v1v0 = v1 - v0;
    vec3 v2v0 = v2 - v0;
    vec3 rov0 = Ray.point - v0;
    n = cross( v1v0, v2v0 );
    vec3  q = cross( rov0, Ray.dir );
    float d = 1.0/dot( Ray.dir, n );
    float u = d*dot( -q, v2v0 );
    float v = d*dot(  q, v1v0 );
    float t = d*dot( -n, rov0 );
    if(min(u, min(v, (1-(u+v)))) < 0.0) t = -1.0;
    //t=min(u, min(v, (1-(u+v)))) * t;
    n = normalize(n);
    return vec3( t, u, v );
}

// Function to compute the distance and material at the intersection
vec2 rayDist(inout Ray rayTrace, inout vec3 normal, out Material material) {
    float dist = infinity;
    float isInside = 0;
    
    for(int i=0;i<balls.length();i++){
		Sphere ball = balls[i];
		vec2 hitResult = sphereHit(rayTrace,ball);
		float bd = hitResult.x;
		if(bd>0.0&&bd<dist){
            isInside = hitResult.y;
			dist = bd;
			vec3 position = rayTrace.point + rayTrace.dir * bd;
			normal = normalize(hitResult.y*(position-ball.position));
			material = ball.material;
		}
	}

    //test boxes
    for(int i=0;i<boxes.length();i++){
		Box block = boxes[i];
		vec4 hitResult = boxHit(rayTrace, block);
		float bd = hitResult.w;
		if(bd>0.0&&bd<dist){
			dist = bd;
			vec3 position = rayTrace.point + rayTrace.dir * bd;
			normal = hitResult.xyz;
			material = block.material;
		}
	}

    for(int i=0;i<1;i++){
        vec3 norm;
        vec3 hitResult = triangleHit(rayTrace, vec3(5,0,3), vec3(-5,0,3), vec3(0,6,3), norm);
		float bd = hitResult.x;
        if(bd>0.0&&bd<dist){
			dist = bd;
			vec3 position = rayTrace.point + rayTrace.dir * bd;
			normal = norm;
			material = Material(1, 1.0, 1.0, 1.0, vec3(1));
		}
    }

    for(int i=0;i<sphereLightSources.length();i++){
		Sphere ball = sphereLightSources[i];
		vec2 hitResult = sphereHit(rayTrace,ball);
		float bd = hitResult.x;
		if(bd>0.0&&bd<dist){
            isInside = hitResult.y;
			dist = bd;
			vec3 position = rayTrace.point + rayTrace.dir * bd;
			normal = normalize(hitResult.y*(position-ball.position));
			material = ball.material;
		}
	}

    for(int i=0;i<boxLightSources.length();i++){
		Box block = boxLightSources[i];
		vec4 hitResult = boxHit(rayTrace, block);
		float bd = hitResult.w;
		if(bd>0.0&&bd<dist){
			dist = bd;
			vec3 position = rayTrace.point + rayTrace.dir * bd;
			normal = hitResult.xyz;
			material = block.material;
		}
	}

    checks++;
    return vec2(dist, isInside);
}

float hash1() {
    return fract(sin(seed += 0.1)*43758.5453123);
}

vec2 hash2() {
    return fract(sin(vec2(seed+=0.1,seed+=0.1))*vec2(43758.5453123,22578.1459123));
}

vec3 hash3() {
    return fract(sin(vec3(seed+=0.1,seed+=0.1,seed+=0.1))*vec3(43758.5453123,22578.1459123,19642.3490423));
}

vec3 cosWeightedRandomHemisphereDirection( const vec3 n) {
  	vec2 r = hash2();
    
	vec3  uu = normalize( cross( n, vec3(0.0,1.0,1.0) ) );
	vec3  vv = cross( uu, n );
	
	float ra = sqrt(r.y);
	float rx = ra*cos(6.2831*r.x); 
	float ry = ra*sin(6.2831*r.x);
	float rz = sqrt( 1.0-r.y );
	vec3  rr = vec3( rx*uu + ry*vv + rz*n );
    
    return normalize( rr );
}

vec3 randomSphereDirection() {
    vec2 r = hash2()*6.2831;
	vec3 dr=vec3(sin(r.x)*vec2(sin(r.y),cos(r.y)),cos(r.x));
	return dr;
}

// Function to trace a Ray and set the normal
bool traceRayNorm(in Ray rayTrace, ivec2 fragCoord, inout Material material) {
    vec3 normal = vec3(0);
    float distance = rayDist(rayTrace, normal, material).x;
    if (distance < infinity) 
    {
        imageStore(normalTexture, fragCoord, vec4(normal, 1.0));
        return true;
    }else{
        imageStore(normalTexture, fragCoord, vec4(0,0,0, 1.0));
        return false;
    }

}

vec4 sampleLight(vec3 point, vec3 normal){
    vec3 lightness = vec3(0);
    float couldBeLit = infinity;
    Material mat;
    vec3 norm = vec3(0);
    Ray lightRay;

    for(int i=0;i<sphereLightSources.length();i++){
		Sphere ball = sphereLightSources[i];

        vec3 lightPos = ball.position + randomSphereDirection()*ball.radius;
        vec3 dirToLight = lightPos-point;
        lightRay.dir = normalize(dirToLight);
        lightRay.point = point+lightRay.dir*0.0001;
        float squareDist = dot(dirToLight, dirToLight);
        float distance = rayDist(lightRay, norm, mat).x;
        if(mat.type == 0){
            lightness += (clamp(dot(lightRay.dir, normal),0.0,1.0) * mat.color)/(distance);
        }else{
        couldBeLit = min(squareDist / (ball.material.color.x + ball.material.color.y + ball.material.color.z), couldBeLit);
        }
	}

    for(int i=0;i<boxLightSources.length();i++){
		Box block = boxLightSources[i];
        vec3 lightPos = block.position; // TODO add random box offset inside box (with rotation) efficiently

        vec3 dirToLight = lightPos-point;
        lightRay.dir = normalize(dirToLight);
        lightRay.point = point+lightRay.dir*0.0001;
        float squareDist = dot(dirToLight, dirToLight);
        float distance = rayDist(lightRay, norm, mat).x;
        if(mat.type == 0){
            lightness += (clamp(dot(lightRay.dir, normal),0.0,1.0) * mat.color)/(distance);
        }else{
        couldBeLit = min(squareDist / (block.material.color.x + block.material.color.y + block.material.color.z), couldBeLit);
        }
	}
    return vec4(lightness, couldBeLit);
}

// Function to trace a Ray and return the color at the intersection (or background color)
void traceRay(inout Ray rayTrace, ivec2 fragCoord, inout Material material) {

    traceRayNorm(rayTrace, fragCoord, material);

    vec3 normal = vec3(0);
    vec3 initPoint = rayTrace.point;
    vec3 initDir = rayTrace.dir;
    vec3 color = vec3(0);
    vec3 light = vec3(0);

    const int raySamples = 10;
    const int rayBounces = 5;
    const float uvCoord = (sin(fragCoord.x*0.141231) + sin(fragCoord.y*0.332512))+frameNo*0.001231432;
    seed = uvCoord;
    int realSamples = 0;
    // Find the closest distance from the Ray to the surface of the sphere
    for(int k = 0; k<raySamples; k++){
        realSamples++;
        rayTrace.point = initPoint+(randomSphereDirection()*0.001);
        rayTrace.dir = initDir;
        rayTrace.col = vec3(1);
        for(int i = 0; i<rayBounces; i++){
            vec2 hitResult = rayDist(rayTrace, normal, material);
            float distance = hitResult.x;
            if (distance < infinity) {
                rayTrace.point += rayTrace.dir * distance;

                if(material.type == 0){
                    rayTrace.col *= (material.color);
                    light += material.color;
                    //k -= min(i,1);
                    k += max(1-i,0)*raySamples;
                    break;
                }
                else{

                    float opaqueness = material.opaqueness;
                    float refractiveIndex = 0.1;
                    float specProbability = material.specularity;
                    int isSpec = specProbability > hash1()? 1 : 0;
                    float smoothness = material.smoothness * isSpec;
                    int isTransparent = opaqueness < hash1()? 1 : 0;
                    i -= isTransparent;
                    vec3 refractedDir = refract(rayTrace.dir, normal, refractiveIndex);
                    vec3 diffuseDir = cosWeightedRandomHemisphereDirection(normal*(1-(isTransparent*2)));
                    vec3 specDir = reflect(rayTrace.dir, normal);
                    vec3 opaqueDir = mix(specDir, refractedDir, isTransparent);
                    rayTrace.dir = mix(diffuseDir, opaqueDir, smoothness);
                    rayTrace.point += rayTrace.dir * 0.001;
                    rayTrace.col *= mix(material.color, vec3(1), min(isSpec+isTransparent,1));
                    vec4 directBrightness = sampleLight(rayTrace.point, normal);
                    //if((length(directBrightness.xyz) > 0.01)){
                    //    k += max(rayBounces-i,0);
                    //}
                    k += int(directBrightness.w*0.2); // how much to focus on priority sampling
                    vec3 directLighting = mix(directBrightness.xyz, vec3(0), min(smoothness+isTransparent,1));
                    light += directLighting;
                    //light += vec3(directBrightness.w*0.005);

                    continue;
                }
            } else {
                //light += vec3(0.1); // ambient light
                k += max(1-i,0)*raySamples;
                break;
            }
        }
        color += rayTrace.col;
    }
    rayTrace.col = (light*color)/float(realSamples*realSamples)*1.0;
    //rayTrace.col = (light)/float(realSamples);
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
    vec3 rayDir = normalize((inverse(viewMatrix) * viewSpacePos).xyz); // Normalize to get Ray direction

    vec3 rayOrigin = position; // Camera position (translation part of the inverse view matrix)
    
vec3 color;
    Material material;

    Ray rayTrace;
    rayTrace.point = rayOrigin;
    rayTrace.dir = rayDir;
    rayTrace.col = vec3(0);
    traceRay(rayTrace, fragCoord, material);

    color = rayTrace.col;

    imageStore(screenTexture, fragCoord, vec4(color, 1.0));
    //imageStore(screenTexture, fragCoord, vec4(float(checks)/100.0,0,0, 1.0));
}