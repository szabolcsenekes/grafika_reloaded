#define _USE_MATH_DEFINES
#include "camera.h"
#include <math.h>
#include <GL/gl.h>

/*
 * Clamp a value between a minimum and maximum.
 */
static float clampf(float v, float lo, float hi)
{
    return (v < lo) ? lo : (v > hi) ? hi
                                    : v;
}

/*
 * Convert degrees to radians.
 */
static float deg2rad(float degrees)
{
    return degrees * M_PI / 180.0f;
}

/*
 * Initialize the camera with default position, orientation and movement values.
 */
void camera_init(Camera *camera)
{
    camera->position.x = 0.0f;
    camera->position.y = -32.0f;

    // Default standing eye height
    camera->eye_height = 1.7f;
    camera->target_eye_height = 1.7f;
    camera->position.z = camera->eye_height;

    // Initial view direction (facing forward)
    camera->yaw = 90.0f;
    camera->pitch = 0.0f;

    // No movement at start
    camera->speed_forward = 0.0f;
    camera->speed_side = 0.0f;

    // Vertical movement (jump/gravity)
    camera->vz = 0.0f;
    camera->on_ground = 1;
}

/*
 * Rotate the camera based on mouse input.
 * Yaw = horizontal rotation
 * Pitch = vertical rotation (clamped to avoid flipping)
 */
void camera_rotate(Camera *camera, float yaw_delta_deg, float pitch_delta_deg)
{
    camera->yaw += yaw_delta_deg;
    camera->pitch += pitch_delta_deg;

    // Keep yaw in [0, 360)
    if (camera->yaw < 0.0f)
        camera->yaw += 360.0f;
    if (camera->yaw >= 360.0f)
        camera->yaw -= 360.0f;

    // Limit pitch to avoid looking too far up/down
    camera->pitch = clampf(camera->pitch, 5.0f, 175.0f);
}

/*
 * Update horizontal (X-Y plane) movement based on speed and direction.
 */
void camera_update_xy(Camera *camera, float delta_time)
{
    float yaw = deg2rad(camera->yaw);

    // Forward direction vector
    float forward_x = cosf(yaw);
    float forward_y = sinf(yaw);

    // Right direction vector (perpendicular to forward)
    float right_x = cosf(yaw + M_PI_2);
    float right_y = sinf(yaw + M_PI_2);

    // Move forward/backward
    camera->position.x += forward_x * camera->speed_forward * delta_time;
    camera->position.y += forward_y * camera->speed_forward * delta_time;

    // Move sideways
    camera->position.x += right_x * camera->speed_side * delta_time;
    camera->position.y += right_y * camera->speed_side * delta_time;
}

/*
 * Update vertical movement (Z axis).
 * Handles gravity, jumping, landing and crouching.
 */
void camera_update_z(Camera *camera, float delta_time)
{
    const float GRAVITY = 18.0f;
    const float CROUCH_SPEED = 6.0f;

    // Smooth transition between standing and crouching
    camera->eye_height += (camera->target_eye_height - camera->eye_height) * CROUCH_SPEED * delta_time;

    if (!camera->on_ground)
    {
        // Apply gravity
        camera->vz -= GRAVITY * delta_time;
        camera->position.z += camera->vz * delta_time;

        // Check for landing
        if (camera->position.z <= camera->eye_height)
        {
            camera->position.z = camera->eye_height;
            camera->vz = 0.0f;
            camera->on_ground = 1;
        }
    }
    else
    {
        // Keep camera on ground
        camera->position.z = camera->eye_height;
        camera->vz = 0.0f;
    }
}

/*
 * Apply the camera transformation to the OpenGL view matrix.
 * This defines how the scene is rendered from the camera's perspective.
 */
void camera_apply_view(const Camera *camera)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Apply rotations (inverse, because we move the world instead of the camera)
    glRotatef(-camera->pitch, 1.0f, 0.0f, 0.0f);
    glRotatef(-(camera->yaw - 90.0f), 0.0f, 0.0f, 1.0f);

    // Translate the world opposite to camera position
    glTranslatef(-camera->position.x, -camera->position.y, -camera->position.z);
}

/*
 * Get the forward direction vector of the camera on the X-Y plane.
 */
void camera_get_forward(const Camera *camera, float *fx, float *fy)
{
    float yaw = deg2rad(camera->yaw);
    *fx = cosf(yaw);
    *fy = sinf(yaw);
}