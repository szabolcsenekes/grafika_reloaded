#ifndef RENDERER_H
#define RENDERER_H

/*
 * Initialize OpenGL render state and set up the initial viewport/projection.
 */
void renderer_init(int width, int height);

/*
 * Update the viewport and projection matrix after a window resize.
 */
void renderer_resize(int width, int height);

/*
 * Start rendering a new frame by clearing the color and depth buffers.
 * The given RGB values define the clear color.
 */
void renderer_begin_frame(float r, float g, float b);

/*
 * Finish the current frame and present it on the SDL window.
 */
void renderer_end_frame(void *sdl_window);

/*
 * Apply scene lighting with the given intensity value.
 */
void renderer_apply_light(float intensity);

/*
 * Draw a fullscreen sky background with a vertical color gradient.
 */
void renderer_draw_sky_gradient(float intensity);

/*
 * Apply animated fog parameters based on time, camera position,
 * and whether the camera is close to the pond area.
 */
void renderer_apply_dynamic_fog(float global_time, float cam_x, float cam_y, int pond_enabled, float pond_x, float pond_y);

#endif // RENDERER_H