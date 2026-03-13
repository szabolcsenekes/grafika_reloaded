#ifndef TEXTURE_H
#define TEXTURE_H

#include <stdbool.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <GL/gl.h>

typedef struct Texture2D {
    GLuint id;
    int width;
    int height;
    bool valid;
} Texture2D;

bool texture_load(Texture2D* out_tex, const char* file_path);

void texture_free(Texture2D* tex);

#endif //TEXTURE_H