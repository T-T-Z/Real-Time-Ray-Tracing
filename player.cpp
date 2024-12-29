#include "includes.hpp"

class Player {
public:
    // Constructor with default parameters for field of view (fov), aspect ratio, and planes
    Player(glm::vec3 position, glm::vec3 direction, float speed, 
           float fov = 100.0f, float aspectRatio = 4.0f / 3.0f, 
           float nearPlane = 0.1f, float farPlane = 100.0f);

    // Move the player based on keyboard input
    void move(const sf::Time& deltaTime, const sf::Keyboard::Key& forwardKey, 
              const sf::Keyboard::Key& backwardKey, const sf::Keyboard::Key& leftKey, 
              const sf::Keyboard::Key& rightKey, const sf::Keyboard::Key& upKey, 
              const sf::Keyboard::Key& downKey, const sf::Keyboard::Key& canLookKey);

    // Update the player's look direction based on mouse movement
    void lookAround(const sf::Window& window, float deltaTime);

    // Get the transformation matrix of the player (model matrix)
    glm::mat4 getTransformationMatrix() const;

    // Get the view matrix for the camera
    glm::mat4 getViewMatrix() const;

    glm::mat4 getZeroedViewMatrix() const;

    // Get the projection matrix
    glm::mat4 getProjectionMatrix() const;

    // Get the player's position
    glm::vec3 getPosition() const;

private:
    glm::vec3 position;
    glm::vec3 direction;
    float speed;
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;
    float yaw;    // Horizontal rotation (yaw)
    float pitch;  // Vertical rotation (pitch)
    bool canLookAround;  // Vertical rotation (pitch)
};

Player::Player(glm::vec3 position, glm::vec3 direction, float speed, 
               float fov, float aspectRatio, float nearPlane, float farPlane)
    : position(position), direction(glm::normalize(direction)), 
      speed(speed), fov(fov), aspectRatio(aspectRatio), 
      nearPlane(nearPlane), farPlane(farPlane), yaw(0.0f), pitch(0.0f) {}

void Player::move(const sf::Time& deltaTime, const sf::Keyboard::Key& forwardKey, 
                  const sf::Keyboard::Key& backwardKey, const sf::Keyboard::Key& leftKey, 
                  const sf::Keyboard::Key& rightKey, const sf::Keyboard::Key& upKey, 
                  const sf::Keyboard::Key& downKey, const sf::Keyboard::Key& canLookKey) {
    float velocity = speed * deltaTime.asSeconds();

    if (sf::Keyboard::isKeyPressed(forwardKey)) {
        position += direction * velocity;
    }
    if (sf::Keyboard::isKeyPressed(backwardKey)) {
        position -= direction * velocity;
    }
    if (sf::Keyboard::isKeyPressed(leftKey)) {
        position -= glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f))) * velocity;
    }
    if (sf::Keyboard::isKeyPressed(rightKey)) {
        position += glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f))) * velocity;
    }
    if (sf::Keyboard::isKeyPressed(upKey)) {
        position += glm::vec3(0.0f, 1.0f, 0.0f) * velocity;
    }
    if (sf::Keyboard::isKeyPressed(downKey)) {
        position -= glm::vec3(0.0f, 1.0f, 0.0f) * velocity;
    }
    if (sf::Keyboard::isKeyPressed(canLookKey)) {
        canLookAround = !canLookAround;
    }

    std::cout<<"pos : "<<position.x<<", "<<position.y<<", "<<position.z<<" dir : "<<direction.x<<", "<<direction.y<<", "<<direction.z<<std::endl;
}

void Player::lookAround(const sf::Window& window, float deltaTime) {
    if (canLookAround){
    // Get the mouse position relative to the window
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::Vector2i center(window.getSize().x / 2, window.getSize().y / 2);

    // Calculate the mouse delta movement
    float deltaX = mousePos.x - center.x;
    float deltaY = mousePos.y - center.y;

    // Sensitivity for mouse movement
    float sensitivity = 0.1f;

    // Update yaw and pitch based on mouse movement
    yaw += deltaX * sensitivity;
    pitch -= deltaY * sensitivity;  // Invert the Y-axis if needed

    // Clamp the pitch to prevent flipping over (e.g., pitch between -89 and 89 degrees)
    pitch = glm::clamp(pitch, -89.0f, 89.0f);

    // Update the direction vector based on the new yaw and pitch
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction = glm::normalize(front);

        // Move the mouse back to the center of the screen
        sf::Mouse::setPosition(center, window);
    }
}

glm::mat4 Player::getTransformationMatrix() const {
    glm::mat4 model = glm::mat4(1.0f); // Identity matrix
    model = glm::translate(model, position);  // Apply translation (move the player)
    model = glm::rotate(model, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));  // Yaw (Y-axis)
    model = glm::rotate(model, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f));  // Pitch (X-axis)

    return model;
}

glm::mat4 Player::getViewMatrix() const {
    glm::vec3 target = position + direction;  // The point the camera is looking at
    glm::vec3 up(0.0f, 1.0f, 0.0f);  // Up direction
    return glm::lookAt(position, target, up);  // LookAt function to create a view matrix
}

glm::mat4 Player::getZeroedViewMatrix() const {
    glm::vec3 target = direction;  // The point the camera is looking at
    glm::vec3 up(0.0f, 1.0f, 0.0f);  // Up direction
    return glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), target, up);  // LookAt function to create a view matrix
}

glm::mat4 Player::getProjectionMatrix() const {
    return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);  // Perspective projection matrix
}

glm::vec3 Player::getPosition() const {
    return position;
}
