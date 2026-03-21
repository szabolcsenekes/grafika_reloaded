#ifndef TEXTURE_H
#define TEXTURE_H

#include <stdbool.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <GL/gl.h>

/*
 * Represents a 2D OpenGL texture.
 *
 * id     - OpenGL texture identifier
 * width  - texture width in pixels
 * height - texture height in pixels
 * valid  - indicates whether the texture was successfully loaded
 */
typedef struct Texture2D
{
    GLuint id;
    int width;
    int height;
    bool valid;
} Texture2D;

/*
 * Load an image file and create an OpenGL texture from it.
 * Returns true on success.
 */
bool texture_load(Texture2D *out_tex, const char *file_path);

/*
 * Free the OpenGL texture and reset the structure.
 */
void texture_free(Texture2D *tex);

#endif // TEXTURE_H