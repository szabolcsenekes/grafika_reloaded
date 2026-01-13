#ifndef RENDERE_H
#define RENDERE_H

void renderer_init(int width, int height);

void renderer_resize(int width, int height);

void renderer_begin_frame(float r, float g, float b);

void renderer_end_frame(void* sdl_window);

#endif