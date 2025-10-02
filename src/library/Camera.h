#ifndef SLBVULKAN_CAMERA_H
#define SLBVULKAN_CAMERA_H

#include <glm/ext.hpp>

#include "Context.h"

/**
 * Different modes specifying how view and projection matrix are created.
 */
enum CameraMode {
    trackBall, /**< The camera rotates around the center position with perspective projection. */
    pilotView, /**< The camera can move around space freely with perspective projection. */
    frontView, /**< The camera is pointed along the negative z-axis with orthographic projection. */
    sideView, /**< The camera is pointed along the negative x-axis with orthographic projection. */
    topView /**< The camera is pointed downwards from above the scene with orthographic projection. */
};

/**
 * Keyframes used to animate camera movement over time
 */
struct CameraKeyFrame {
    double time; /**< Timestamp at which the camera reaches the specified location */
    glm::vec3 position; /**< Position of the camera at the specified time */
    glm::vec3 direction; /**< Direction the camera is pointed in at the specified time */
};

/**
 * GPU representation of the relevant camera parameters.
 * 
 * Used to pass view and projection matrix to shaders.
 * Width and height of the screen is also added as float values to get screen coordinates in the shader without having to cast to float every time.
 */
struct CameraUniforms {
    glm::mat4 viewMatrix; /**< Matrix converting world coordinates to camera coordinates */
    glm::mat4 projectionMatrix; /**< Matrix converting camera coordinates to screen coordinates */
    float screenWidth; /**< Width of the rendered image in number of pixels */
    float screenHeight; /**< Height of the rendered image in number of pixels */
    float pad1; /**< Padding for now */
    float pad2; /**< Padding for now */
};

/**
 * Camera to view the rendered scene.
 * Determines what part of the scene is visible by specifying a view and projection matrix used in the shaders.
 * The different camera modes can be used to switch between trackball, pilotview and total scene view.
 * Glfw input is used to update position and direction of the camera as well as the camera mode.
 */
class Camera {
public:
    /**
     * Creates a default camera in trackball mode.
     * Initially centered around world origin with radius 1.0.
     * Width and height parameters determine the aspect ratio used for the projection matrix.
     * @param width Width of the window in number of pixels.
     * @param height Height of the window in number of pixels.
     * @param window Glfw window used to request input and display the rendered image.
     */
    Camera(int width, int height, std::unique_ptr<GLFWwindow, DestroyGLFWwindow> &window);

    ~Camera() = default;

    /**
     * Returns the (full) vertical opening angle used for perspective projection.
     * @return opening angle in radians
     */
    float getFieldOfView();

    /**
     * Returns the nearest depth value limiting the view frustum
     * @return smallest visible z-coordinate
     */
    float getNear();

    /**
     * Returns the farthest depth value limiting the view frustum
     * @return greatest visible z-coordinate
     */
    float getFar();

    /**
     * Provides the view matrix.
     * @return 4x4 view matrix
     */
    glm::mat4 getViewMatrix();

    /**
     * Provides the projection matrix.
     * @return 4x4 projection matrix
     */
    glm::mat4 getProjectionMatrix();

    /**
     * Changes the position of the camera.
     * @param position new position in world coordinates
     */
    void setPosition(glm::vec3 position);

    /**
     * Changes the orientation of the camera.
     * The direction is calculated using the provided angles.
     * If both angles are set to 0 the resulting direction is (0, 0, -1).
     * @param theta vertical angle clamped to [-0.5*pi; 0.5*pi]
     * @param phi horizontal angle clamped to [0, 2*pi]
     */
    void setAngles(float theta, float phi);

    /**
     * Changes the radius used for trackball mode.
     * @param radius distance between camera and trackball center
     */
    void setRadius(float radius);

    /**
     * Changes the nearest depth value limiting the view frustum.
     * @param depth smallest visible z-coordinate
     */
    void setNear(float depth);

    /**
     * Changes the farthest depth value limiting the view frustum.
     * @param depth greates visible z-coordinate
     */
    void setFar(float depth);

    /**
     * Changes the camera mode.
     * This alters how the camera is positioned and how it can be manipulated.
     * @param mode Mode that the camera is switched to.
     */
    void setMode(CameraMode mode);

    /**
     * Adds a keyframe to the camera movement animation.
     * @param time timestamp in seconds
     * @param position location of the camera
     * @param direction orientation of the camera
     */
    void addKeyFrame(double time, glm::vec3 position, glm::vec3 direction);

    /**
     * Updates the camera according to glfw input.
     * If the up or down key is pressed on the keyboard either the position or the radius are adjusted.
     * If the left mouse button is pressed the direction is updated based on the current and previous cursor positions.
     * All input is scaled by the time passed since the last frame.
     * @param window Glfw window used to access input.
     */
    void updateInput(std::unique_ptr<GLFWwindow, DestroyGLFWwindow> &window);
    
private:
    CameraMode m_mode = trackBall; /**< Current camera mode as defined by CameraMode enum. */

    glm::vec3 m_position{0.0f}; /**< Position the camera is either located or pointed at depending on the mode. */
    glm::vec3 m_direction{0.0f, 0.0f, -1.0f}; /**< Direction the camera is oriented in. Automatically updated based on input. */
    glm::vec3 m_up{0.0f, 1.0f, 0.0f}; /**< Vector defining upward direction of the camera perspective. Can be used to rotate around view direction. */

    float m_radius = 1.0f; /**< Radius of the trackball. Also indicates the position in frontView, sideView and topView modes. */
    float m_theta = 0.0f; /**< Vertical angle between direction and xz-plane. Always between -0.5*pi and 0.5*pi. */
    float m_phi = 0.0f; /**< Horizontal angle between direction (projected onto xz-plane) and z-axis. Always between 0 and 2*pi. */

    float m_mouseSensitivity = 3.0f; /**< Scale at which changes from mouse input affect the camera */
    float m_keySensitivity = 1.0f; /**< Scale at which changes from key input affect the camera */

    float m_aspectRatio; /**< Width to height ratio of the rendered image. */
    float m_fovy = glm::radians(60.0f); /**< Vertical opening angle of the camera in radians. */
    float m_near = 0.01f; /**< Closest distance to camera included in the view. */
    float m_far = 10.0f; /**< Farthest distance to camera included in the view. */

    double m_currentTime = 0.0; /**< Current time provided by glfw. */
    double m_currentCursorX = 0.0; /**< X-coordinate of the current cursor position within the window. */
    double m_currentCursorY = 0.0; /**< Y-coordinate of the current cursor position within the window. */
    double m_oldTime = 0.0; /**< Timestamp recorded for the previous frame. */
    double m_oldCursorX = 0.0; /**< X-coordinate of the cursor position recorded for the previous frame. */
    double m_oldCursorY = 0.0; /**< Y-coordinate of the cursor position recorded for the previous frame. */

    uint32_t m_currentKeyFrame = 0; /**< Index of the last keyframe reached within the animation. */
    std::vector<CameraKeyFrame> m_keyFrames; /**< Keyframes used to animate camera movement. */
    
};

#endif //SLBVULKAN_CAMERA_H