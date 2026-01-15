#define _USE_MATH_DEFINES
#include "camera.h"
#include <math.h>
#include <GL/gl.h>

static float clampf(float v, float lo, float hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

static float deg2rad(float degrees) {
    return degrees * M_PI / 180.0f;
}

void camera_init(Camera* camera) {
    camera->position.x = 0.0f;
    camera->position.y = 0.0f;

    camera->eye_height = 1.7f;
    camera->target_eye_height = 1.7f;
    camera->position.z = camera->eye_height;
    
    camera->yaw = 90.0f;
    camera->pitch = 0.0f;

    camera->speed_forward = 0.0f;
    camera->speed_side = 0.0f;
    
    camera->vz = 0.0f;
    camera->on_ground = 1;
}

void camera_rotate(Camera* camera, float yaw_delta_deg, float pitch_delta_deg) {
    camera->yaw += yaw_delta_deg;
    camera->pitch += pitch_delta_deg;

    if (camera->yaw < 0.0f) camera->yaw += 360.0f;
    if (camera->yaw >= 360.0f) camera->yaw -= 360.0f;

    camera->pitch = clampf(camera->pitch, 5.0f, 175.0f);
}

void camera_update(Camera* camera, float delta_time) {
    const float GRAVITY = 18.0f;
    const float GROUND_Z = camera->eye_height;

    float yaw = deg2rad(camera->yaw);

    float forward_x = cosf(yaw);
    float forward_y = sinf(yaw);

    float right_x = cosf(yaw + M_PI_2);
    float right_y = sinf(yaw + M_PI_2);

    camera->position.x += forward_x * camera->speed_forward * delta_time;
    camera->position.y += forward_y * camera->speed_forward * delta_time;

    camera->position.x += right_x * camera->speed_side * delta_time;
    camera->position.y += right_y * camera->speed_side * delta_time;

    if (!camera->on_ground) {
        camera->vz -= GRAVITY * delta_time;
        camera->position.z += camera->vz * delta_time;

        if (camera->position.z <= GROUND_Z) {
            camera->position.z = GROUND_Z;
            camera->vz = 0.0f;
            camera->on_ground = 1;
        }
    }

    float crouch_speed = 6.0f;

    camera->eye_height += (camera->target_eye_height - camera->eye_height) * crouch_speed * delta_time;

    if (camera->on_ground) {
        camera->position.z = camera->eye_height;
    }
}

void camera_apply_view(const Camera* camera) {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glRotatef(-camera->pitch, 1.0f, 0.0f, 0.0f);

    glRotatef(-(camera->yaw - 90.0f), 0.0f, 0.0f, 1.0f);

    glTranslatef(-camera->position.x, -camera->position.y, -camera->position.z);
}

void camera_get_forward(const Camera* camera, float* fx, float* fy) {
    float yaw = deg2rad(camera->yaw);
    *fx = cosf(yaw);
    *fy = sinf(yaw);
}