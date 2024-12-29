#include "includes.hpp"
#include "shaderStuff.hpp"
#include "player.hpp"

int main() {
    // Initialize SFML window and OpenGL context
    sf::ContextSettings settings;
    //settings.antialiasingLevel = 4;
    //settings.majorVersion = 4;
    //settings.minorVersion = 3;

    sf::RenderWindow window(sf::VideoMode(1600, 1200), "Rays And Such", sf::Style::Default, settings);
    //window.setVerticalSyncEnabled(true);

    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW initialization failed!" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);    // Default depth function; only objects closer than the previous depth value are rendered
    glDepthMask(GL_TRUE);    // Enable writing to the depth buffer
    
    // Fullscreen Quad Shader
    std::map<GLenum, std::string> quadShader = {
        {GL_VERTEX_SHADER, "shaders/rendering/vertex_shader.glsl"},
        {GL_FRAGMENT_SHADER, "shaders/rendering/fragment_shader.glsl"}
    };

    Shader quadShaderProgram(quadShader);
    Quad fullScreenQuad;  // Quad object to render the full screen

    // Fullscreen Quad Shader
    std::map<GLenum, std::string> computeShader = {
        {GL_COMPUTE_SHADER, "shaders/compute/triangle_RayTrace_shader.glsl"},
    };

    Shader computeShaderProgram(computeShader);

    Texture screenTexture(1600, 1200);
    Texture normalTexture(1600, 1200);

    // Create the Player object
    Player player(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), 5.0f);  // Position, direction, speed

    // Time-related variables for smooth movement
    sf::Clock clock;

    unsigned int frameNo = 0;

    // Main loop
    while (window.isOpen()) {
        sf::Event event;

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
            break;
        }

        // Get the elapsed time
        sf::Time deltaTime = clock.restart();

        // Update the player's position, view, and projection matrices (optional)
        player.move(deltaTime, sf::Keyboard::W, sf::Keyboard::S, sf::Keyboard::A, sf::Keyboard::D, sf::Keyboard::Space, sf::Keyboard::LControl, sf::Keyboard::Q);
        player.lookAround(window, deltaTime.asSeconds());
 
        glm::mat4 view = player.getViewMatrix();
        glm::mat4 zeroedView = player.getZeroedViewMatrix();
        
        glm::mat4 projection = player.getProjectionMatrix();

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Use the compute shader
        computeShaderProgram.use();
        computeShaderProgram.setMat4("viewMatrix", zeroedView);
        computeShaderProgram.setMat4("projMatrix", projection);
        computeShaderProgram.setVec3("position", player.getPosition());
        computeShaderProgram.setUInt("frameNo", frameNo);
        computeShaderProgram.setImage("screenTexture", screenTexture.getID(), 0, GL_RGBA8);
        computeShaderProgram.setImage("normalTexture", normalTexture.getID(), 1, GL_RGBA8);

        // Dispatch compute shader (assuming it's a 512x512 grid)
        glDispatchCompute(1600 / 16, 1200 / 16, 1);  // For example, dispatching 16x16 workgroups
        
        // Wait for the compute shader to finish
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // Set the view and projection matrices to the quad shader program
        quadShaderProgram.use();
        quadShaderProgram.setMat4("view", view);
        quadShaderProgram.setMat4("projection", projection);
        quadShaderProgram.setTexture("screenTexture", screenTexture.getID(), 0);
        quadShaderProgram.setTexture("normalTexture", normalTexture.getID(), 1);

        // Bind and draw the full-screen quad
        fullScreenQuad.bind();
        fullScreenQuad.draw();
        
        window.setTitle("FPS: " + std::to_string((1/deltaTime.asSeconds())));

        // Display the frame
        window.display();
        frameNo++;
    }

    return 0;
}

