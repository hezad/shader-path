


/* Defining constants */

#define PATH_MAX_LIGHTS 10
#define PATH_MAX_OBJECTS 10

#define PATH_FLOAT_EPSILON 0.0001
#define PATH_INFINITY 9999999.0

#define PATH_OBJECT_SPHERE 1



/* Defining uniforms */

const int sceneNumLights = {{js:num_lights}};
const int sceneNumObjects = 1;

uniform vec2 u_screen_size;
uniform int u_frame_count;
uniform vec3 u_ambiant_color;

uniform vec3 u_lights_origin[ sceneNumLights ];
uniform vec3 u_lights_color[ sceneNumLights ];
uniform float u_lights_intensity[ sceneNumLights ];


/* Defining Structs */

struct MaterialEntity {
    vec3 baseColor;
    float diffuse;
    float specular;
    int shininess;
};

struct ObjectEntity {
    int objectType;
    vec3 origin;
    float radius;
    MaterialEntity material;
};

struct RayEntity {
    vec3 origin;
    vec3 direction;
};

struct CameraEntity {
    vec3 origin;
    mat3 coordinateSystem;
    vec2 fov;
    float nearClipPlaneDist;
};

struct Intersection {
    bool intersect;
    float distance;
    ObjectEntity object;
    RayEntity ray;
    vec3 intersectionPoint;
    vec3 normal;
};



/* Defining raytracing functions */

vec3 sphereNormal(vec3 intersectPoint, ObjectEntity sphere) {
    return normalize(intersectPoint - sphere.origin);
}

vec3 getIntersectPoint(RayEntity ray, float dist) {
    return ray.origin + ray.direction * dist;
}

Intersection dontIntersect() {
    Intersection intersect;
    intersect.intersect = false;
    return intersect;
}

Intersection doesIntersect(RayEntity ray, ObjectEntity object, float distance) {
    Intersection intersect;
    intersect.intersect = true;
    intersect.object = object;
    intersect.distance = distance;
    intersect.ray = ray;
    intersect.intersectionPoint = getIntersectPoint(ray, distance);

    if( intersect.object.objectType == PATH_OBJECT_SPHERE ) {
        intersect.normal = sphereNormal(intersect.intersectionPoint, intersect.object);
    }

    return intersect;
}

Intersection sphereIntersect(RayEntity ray, ObjectEntity sphere) {
    float A = dot(ray.direction, ray.direction);
    vec3 originToCenterRay = ray.origin - sphere.origin;
    float B = dot(originToCenterRay, ray.direction) * 2.0;
    float C = dot(originToCenterRay, originToCenterRay) - sphere.radius * sphere.radius;

    float delta = B * B - 4.0 * A * C;

    if(delta > 0.0) {
        delta = sqrt(delta);
        float distance1 = (-B - delta) / (2.0 * A);
        float distance2 = (-B + delta) / (2.0 * A);
        
        if(distance2 > PATH_FLOAT_EPSILON) {
            if(distance1 < PATH_FLOAT_EPSILON) {
                if(distance2 < distance1) {
                    return doesIntersect(ray, sphere, distance2);
                }
            } else {
                if(distance1 < distance2) {
                    return doesIntersect(ray, sphere, distance1);
                }
            }
        }
    }

    return dontIntersect();
}



/* Global Scene variables */
ObjectEntity sceneObjects[ 1 ]; 



/* Global Scene calc functions (Lighting/Shading mainly) */

bool lightPositionIsVisibleFrom(vec3 lightPos, vec3 point) {
    return true;
}

vec3 getLightContribution(Intersection intersection) {

    vec3 ambiantColor = u_ambiant_color;
    vec3 diffuseColor = vec3(0.0, 0.0, 0.0);
    vec3 specularColor = vec3(0.0, 0.0, 0.0);

    vec3 finalColor = ambiantColor;

    for(int i = 0; i < sceneNumLights; i++) {
        vec3 o = u_lights_origin[i];
        vec3 c = u_lights_color[i];
        float intensity = u_lights_intensity[i];

        if( lightPositionIsVisibleFrom(o, intersection.intersectionPoint) ) {
            finalColor.r += 0.2;
            finalColor.g += 0.2;
            finalColor.b += 0.2;
        }
    }

    return finalColor;
}



/* And here we go ! */

CameraEntity camera;
RayEntity baseRay;

void main() {

    /* Those settings and calculations should be done only once
       and not for each pixel. They should be out of main() but how ? */
    sceneObjects[0].objectType = PATH_OBJECT_SPHERE;
    sceneObjects[0].origin = vec3(0.0, 0.0, 2.0);
    sceneObjects[0].radius = 1.0;
    sceneObjects[0].material.baseColor = vec3(0.8, 0.8, 1.0);
    sceneObjects[0].material.diffuse = 0.8;
    sceneObjects[0].material.specular = 1.0;
    sceneObjects[0].material.shininess = 80;

    camera.origin = vec3(0.0, 0.0, 0.0);
    camera.coordinateSystem[0] = vec3(1.0, 0.0, 0.0);
    camera.coordinateSystem[1] = vec3(0.0, 1.0, 0.0);
    camera.coordinateSystem[2] = vec3(0.0, 0.0, 1.0);
    camera.fov.x = 90.0;
    camera.fov.y = camera.fov.x / (u_screen_size.x / u_screen_size.y);
    camera.nearClipPlaneDist = 1.0;
    /* ---------------------------------------------------- */

    vec2 homogenousPixelPosition;
    homogenousPixelPosition.x = 2.0 * float(gl_FragCoord.x) / u_screen_size.x - 1.0;
    homogenousPixelPosition.y = 2.0 * float(gl_FragCoord.y) / u_screen_size.y - 1.0;

    vec3 viewVector;
    viewVector.x = camera.nearClipPlaneDist * homogenousPixelPosition.x * tan(camera.fov.x / 2.0);
    viewVector.y = camera.nearClipPlaneDist * homogenousPixelPosition.y * tan(camera.fov.y / 2.0);
    viewVector.z = 1.0;

    baseRay.origin = camera.origin;
    baseRay.direction = normalize(vec3(
        dot(viewVector, camera.coordinateSystem[0]),
        dot(viewVector, camera.coordinateSystem[1]),
        dot(viewVector, camera.coordinateSystem[2])
    ));

    float r = u_ambiant_color.r; 
    float g = u_ambiant_color.g; 
    float b = u_ambiant_color.b;

    Intersection closestIntersection;
    closestIntersection.distance = PATH_INFINITY;
    closestIntersection.intersect = false;

    Intersection actualIntersection;
    for(int i = 0; i < sceneNumObjects; i++) {
        if( sceneObjects[i].objectType == PATH_OBJECT_SPHERE ) {
            actualIntersection = sphereIntersect(baseRay, sceneObjects[i]);
        }

        if( actualIntersection.intersect && actualIntersection.distance < closestIntersection.distance ) {
            closestIntersection = actualIntersection;
        }
    }

    if( closestIntersection.intersect ) {

        vec3 color = getLightContribution(closestIntersection);

        r = color.r;
        g = color.g;
        b = color.b;
    }
    
    gl_FragColor = vec4(r, g, b, 1.0);
}