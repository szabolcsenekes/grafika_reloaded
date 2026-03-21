#ifndef GEOM_H
#define GEOM_H

/*
 * Axis-Aligned Bounding Box.
 * Stores the minimum and maximum coordinates of a 3D box.
 */
typedef struct AABB
{
    float minx, miny, minz;
    float maxx, maxy, maxz;
} AABB;

/*
 * Simple RGB color structure.
 */
typedef struct Color3
{
    float r, g, b;
} Color3;

#endif // GEOM_H