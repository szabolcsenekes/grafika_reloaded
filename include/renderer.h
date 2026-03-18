#ifndef RENDERER_H
#define RENDERER_H

void renderer_init(int width, int height);

void renderer_resize(int width, int height);

void renderer_begin_frame(float r, float g, float b);

void renderer_end_frame(void* sdl_window);

void renderer_apply_light(float intensity);

void renderer_draw_sky_gradient(float intensity);

#endif //RENDERER_H