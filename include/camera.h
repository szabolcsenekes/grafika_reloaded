#ifndef CAMERA_H
#define CAMERA_H

/*
 * Simple 3D vector used for storing positions in the world.
 */
typedef struct Vec3
{
    float x;
    float y;
    float z;
} Vec3;

/*
 * First-person style camera.
 *
 * position           - current world position of the camera
 * yaw                - horizontal rotation in degrees
 * pitch              - vertical rotation in degrees
 * speed_forward      - movement speed along the forward direction
 * speed_side         - movement speed along the right/left direction
 * vz                 - vertical velocity, used for jumping and gravity
 * on_ground          - indicates whether the camera is standing on the ground
 * eye_height         - current eye height above the ground
 * target_eye_height  - target eye height for smooth crouching
 */
typedef struct Camera
{
    Vec3 position;
    float yaw;
    float pitch;

    float speed_forward;
    float speed_side;

    float vz;
    int on_ground;

    float eye_height;
    float target_eye_height;
} Camera;

/*
 * Initialize the camera with default position, view direction,
 * movement values, and standing eye height.
 */
void camera_init(Camera *camera);

/*
 * Rotate the camera by the given yaw and pitch deltas in degrees.
 */
void camera_rotate(Camera *camera, float yaw_delta_deg, float pitch_delta_deg);

/*
 * Update horizontal camera movement (X and Y axes) based on
 * current movement speeds and elapsed time.
 */
void camera_update_xy(Camera *camera, float delta_time);

/*
 * Update vertical camera movement (Z axis), including gravity,
 * jumping, landing, and crouch smoothing.
 */
void camera_update_z(Camera *camera, float delta_time);

/*
 * Apply the camera transformation to the OpenGL model-view matrix.
 */
void camera_apply_view(const Camera *camera);

/*
 * Get the forward direction of the camera on the X-Y plane.
 * The result is written into fx and fy.
 */
void camera_get_forward(const Camera *camera, float *fx, float *fy);

#endif // CAMERA_H