#ifndef PLAYER_HPP
#define PLAYER_HPP

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
    bool canLookAround;
};

#endif // PLAYER_HPP
