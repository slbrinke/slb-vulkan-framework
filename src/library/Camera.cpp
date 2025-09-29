#include "Camera.h"

Camera::Camera(int width, int height, std::unique_ptr<GLFWwindow, DestroyGLFWwindow> &window) {
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);

    m_currentTime = glfwGetTime();
    m_oldTime = m_currentTime;

    glfwGetCursorPos(window.get(), &m_currentCursorX, &m_currentCursorY);
    m_oldCursorX = m_currentCursorX;
    m_oldCursorY = m_currentCursorY;
}

float Camera::getFieldOfView() {
    return m_fovy;
}

float Camera::getNear() {
    return m_near;
}

float Camera::getFar() {
    return m_far;
}

glm::mat4 Camera::getViewMatrix() {
    if(m_mode == pilotView) {
        return glm::lookAt(m_position, m_position + m_direction, m_up);
    }
    
    return glm::lookAt(m_position - m_radius * m_direction, m_position, m_up);
}

glm::mat4 Camera::getProjectionMatrix() {
    glm::mat4 projection = glm::mat4(1.0f);
    if(m_mode == trackBall || m_mode == pilotView) {
        projection = glm::perspective(m_fovy, m_aspectRatio, m_near, m_far);
    } else {
        float screenHeight = 0.5f * m_radius;
        float screenWidth = m_aspectRatio * screenHeight;
        //projection = glm::ortho(-screenWidth, screenWidth, -screenHeight, screenHeight, m_near, m_far);
        projection = glm::transpose(glm::mat4(
            1.0f / screenWidth, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f / screenHeight, 0.0f, 0.0f,
            0.0f, 0.0f, -1.0f / (m_far - m_near), - m_near / (m_far - m_near),
            0.0f, 0.0f, 0.0f, 1.0f
        ));
    }
    projection[1][1] *= -1.0f;
    return projection;
}

void Camera::setPosition(glm::vec3 position) {
    m_position = position;
}

void Camera::setAngles(float theta, float phi) {
    m_theta = theta;
    m_phi = phi;

    m_theta = glm::clamp(m_theta, -0.499f * glm::pi<float>(), 0.499f * glm::pi<float>());

    m_direction.x = - glm::sin(m_phi) * glm::cos(m_theta);
    m_direction.y = glm::sin(m_theta);
    m_direction.z = - glm::cos(m_phi) * glm::cos(m_theta);
    m_direction = glm::normalize(m_direction);
}

void Camera::setRadius(float radius) {
    m_radius = radius;
    //m_near = 0.01f * m_radius;
    m_far = 10.0f * m_radius;
}

void Camera::setNear(float depth) {
    m_near = depth;
}

void Camera::setFar(float depth) {
    m_far = depth;
}

void Camera::setMode(CameraMode mode) {
    //if mode switches to or from pilotview the position is adjusted to avoid a sudden jump
    if(mode == pilotView && m_mode != pilotView) {
        m_position -= m_radius * m_direction;
    }
    if(mode != pilotView && m_mode == pilotView) {
        m_position += m_radius * m_direction;
    }
    if(mode == frontView) {
        m_position = glm::vec3(0.0f, 0.0f, 0.0f);
        setAngles(0.0f, 0.0f);
    }
    if(mode == sideView) {
        m_position = glm::vec3(0.0f, 0.0f, 0.0f);
        setAngles(0.0f, 0.5f * glm::pi<float>());
    }
    if(mode == topView) {
        m_position = glm::vec3(0.0f, 0.0f, 0.0f);
        m_direction = glm::vec3(0.0f, -1.0f, 0.0f);
        m_up = glm::vec3(0.0f, 0.0f, 1.0f);
    }
    if(mode != topView && m_mode == topView) {
        m_up = glm::vec3(0.0f, 1.0f, 0.0f);
        if(mode == trackBall || mode == pilotView) {
            setAngles(-0.5f * glm::pi<float>(), 0.0f);
        }
    }
    m_mode = mode;
}

void Camera::addKeyFrame(double time, glm::vec3 position, glm::vec3 direction) {
    auto keyFrameIndex = m_keyFrames.size();
    m_keyFrames.resize(keyFrameIndex + 1);
    m_keyFrames[keyFrameIndex].time = time;
    m_keyFrames[keyFrameIndex].position = position;
    m_keyFrames[keyFrameIndex].direction = direction;
}

void Camera::updateInput(std::unique_ptr<GLFWwindow, DestroyGLFWwindow> &window) {
    //update time passed
    m_currentTime = glfwGetTime();
    auto deltaTime = static_cast<float>(m_currentTime - m_oldTime);
    m_oldTime = m_currentTime;

    if(m_keyFrames.empty()) {
        if(m_mode == pilotView) {
            //in pilotview mode the arrow and wasd keys move the camera around
            if(glfwGetKey(window.get(), GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(window.get(), GLFW_KEY_W) == GLFW_PRESS) {
                m_position += deltaTime * m_keySensitivity * m_direction;
            }
            if(glfwGetKey(window.get(), GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(window.get(), GLFW_KEY_A) == GLFW_PRESS) {
                m_position -= deltaTime * m_keySensitivity * glm::cross(m_direction, m_up);
            }
            if(glfwGetKey(window.get(), GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(window.get(), GLFW_KEY_S) == GLFW_PRESS) {
                m_position -= deltaTime * m_keySensitivity * m_direction;
            }
            if(glfwGetKey(window.get(), GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(window.get(), GLFW_KEY_D) == GLFW_PRESS) {
                m_position += deltaTime * m_keySensitivity * glm::cross(m_direction, m_up);
            }
        } else {
            //in the other modes the up, down, w, and s keys zoom in and out
            if(glfwGetKey(window.get(), GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(window.get(), GLFW_KEY_W) == GLFW_PRESS) {
                m_radius *= 1.0f - deltaTime * m_keySensitivity;
            }
            if(glfwGetKey(window.get(), GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(window.get(), GLFW_KEY_S) == GLFW_PRESS) {
                m_radius *= 1.0f + deltaTime * m_keySensitivity;
            }

            if(m_mode == frontView || m_mode == sideView) {
                if(glfwGetKey(window.get(), GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(window.get(), GLFW_KEY_A) == GLFW_PRESS) {
                    setAngles(m_theta, m_phi - deltaTime * m_keySensitivity);
                }
                if(glfwGetKey(window.get(), GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(window.get(), GLFW_KEY_D) == GLFW_PRESS) {
                    setAngles(m_theta, m_phi + deltaTime * m_keySensitivity);
                }
            }
        }

        //update cursor position
        glfwGetCursorPos(window.get(), &m_currentCursorX, &m_currentCursorY);
        auto deltaX = static_cast<float>(m_currentCursorX - m_oldCursorX);
        auto deltaY = static_cast<float>(m_currentCursorY - m_oldCursorY);
        m_oldCursorX = m_currentCursorX;
        m_oldCursorY = m_currentCursorY;

        if(glfwGetMouseButton(window.get(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            if(m_mode == trackBall || m_mode == pilotView) {
                setAngles(m_theta - deltaY * deltaTime * m_mouseSensitivity, m_phi - deltaX * deltaTime * m_mouseSensitivity);
            } else {
                m_position += deltaY * deltaTime * m_mouseSensitivity * m_up;
                auto xVec = glm::normalize(glm::cross(m_direction, m_up));
                m_position -= deltaX * deltaTime * m_mouseSensitivity * xVec;
            }
        }
    } else {
        //animate camera movement
        if(m_keyFrames.size() > m_currentKeyFrame) {
            if(m_currentTime > m_keyFrames[m_currentKeyFrame + 1].time) {
                m_currentKeyFrame++;
                m_position = m_keyFrames[m_currentKeyFrame].position;
                m_direction = m_keyFrames[m_currentKeyFrame].direction;
            }
            
            if(m_keyFrames.size() > m_currentKeyFrame) {
                auto animParam = static_cast<float>((m_currentTime - m_keyFrames[m_currentKeyFrame].time) / (m_keyFrames[m_currentKeyFrame + 1].time - m_keyFrames[m_currentKeyFrame].time));
                //smooth curve for softer transitions
                animParam *= animParam;
                animParam = animParam * (2.0f - animParam);
                m_position = animParam * m_keyFrames[m_currentKeyFrame + 1].position + (1.0f - animParam) * m_keyFrames[m_currentKeyFrame].position;
                m_direction = animParam * m_keyFrames[m_currentKeyFrame + 1].direction + (1.0f - animParam) * m_keyFrames[m_currentKeyFrame].direction;
            }
        }
    }

}