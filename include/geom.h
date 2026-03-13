#ifndef GEOM_H
#define GEOM_H

typedef struct AABB {
    float minx, miny, minz;
    float maxx, maxy, maxz;
} AABB;

typedef struct Color3 {
    float r, g, b;
} Color3;

#endif //GEOM_H