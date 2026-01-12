#ifndef CAMERA_H
#define CAMERA_H

typedef struct Vec3 {
    float x;
    float y;
    float z;
} Vec3;

typedef struct Camera {
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

void camera_init(Camera* camera);

void camera_rotate(Camera* camera, float yaw_delta_deg, float pitch_delta_deg);

void camera_update(Camera* camera, float delta_time);

void camera_apply_view(const Camera* camera);

#endif // CAMERA_H