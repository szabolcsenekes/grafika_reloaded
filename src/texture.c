#include "texture.h"

#include <GL/gl.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <stdio.h>

/*
 * Convert SDL surface pixel format to an OpenGL format.
 * (Currently always returns RGBA, since we convert everything to ABGR8888.)
 */
static GLenum surface_format_to_gl(const SDL_PixelFormat *fmt)
{
    (void)fmt;
    return GL_RGBA;
}

/*
 * Load an image from file and create an OpenGL texture.
 */
bool texture_load(Texture2D *out_tex, const char *file_path)
{
    if (!out_tex)
        return false;

    // Reset texture structure
    out_tex->id = 0;
    out_tex->width = 0;
    out_tex->height = 0;
    out_tex->valid = false;

    /*
     * Load image using SDL_image
     */
    SDL_Surface *loaded = IMG_Load(file_path);
    if (!loaded)
    {
        fprintf(stderr, "IMG_Load failed for: '%s' : %s\n", file_path, IMG_GetError());
        return false;
    }

    /*
     * Convert surface to a known pixel format (RGBA-like)
     */
    SDL_Surface *surf = SDL_ConvertSurfaceFormat(loaded, SDL_PIXELFORMAT_ABGR8888, 0);
    SDL_FreeSurface(loaded);
    if (!surf)
    {
        fprintf(stderr, "SDL_ConvertSurfaceFormat failed for '%s' : %s\n", file_path, SDL_GetError());
        return false;
    }

    /*
     * Generate and bind OpenGL texture
     */
    GLuint tex_id = 0;
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);

    /*
     * Set texture parameters (filtering and wrapping)
     */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    /*
     * Upload pixel data to GPU
     */
    GLenum gl_fmt = surface_format_to_gl(surf->format);
    (void)gl_fmt;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        surf->w,
        surf->h,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        surf->pixels);

    /*
     * Unbind texture
     */
    glBindTexture(GL_TEXTURE_2D, 0);

    /*
     * Fill output structure
     */
    out_tex->id = tex_id;
    out_tex->width = surf->w;
    out_tex->height = surf->h;
    out_tex->valid = true;

    SDL_FreeSurface(surf);
    return true;
}

/*
 * Delete the OpenGL texture and reset its data.
 */
void texture_free(Texture2D *tex)
{
    if (!tex)
        return;

    if (tex->valid && tex->id != 0)
    {
        glDeleteTextures(1, &tex->id);
    }

    tex->id = 0;
    tex->width = 0;
    tex->height = 0;
    tex->valid = false;
}