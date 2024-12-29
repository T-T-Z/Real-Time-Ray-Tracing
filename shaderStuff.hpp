#ifndef SHADER_HPP
#define SHADER_HPP
#include "includes.hpp"
class Shader {
public:
    Shader(const std::map<GLenum, std::string>& shaderPaths);
    ~Shader();

    void use() const;
    GLuint getProgram() const { return program; }

    void setMat4(const std::string& name, const glm::mat4& matrix) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setUInt(const std::string& name, unsigned int value) const;

    void setTexture(const std::string& name, GLuint textureID, GLuint unit) const;
    void setImage(const std::string& name, GLuint textureID, GLuint bindingPoint, GLenum format = GL_RGBA8) const;

private:
    GLuint compileShader(GLenum type, const std::string& source);
    GLuint createProgram(const std::vector<GLuint>& shaders);
    
    GLuint program;

    // Helper function to load shader source from a file
    std::string loadShaderSource(const std::string& filepath);
};

class Buffer {
public:
    enum Type {
        VERTEX_BUFFER,
        ELEMENT_BUFFER,
        STORAGE_BUFFER,
        UNIFORM_BUFFER
    };

    // Constructor for a buffer object
    Buffer(Type type, GLenum usage = GL_STATIC_DRAW);
    ~Buffer();

    // Generate the buffer
    void generateBuffer();

    // Bind the buffer to a specific target (e.g., GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER, etc.)
    void bind() const;

    // Unbind the buffer
    void unbind() const;

    // Upload data to the buffer
    void uploadData(const void* data, size_t size);

    // Map the buffer for reading or writing
    void* mapBuffer(GLenum access = GL_READ_WRITE);

    // Unmap the buffer
    void unmapBuffer() const;

    // Set a buffer as an SSBO or UBO (with binding points)
    void setBufferBinding(GLuint bindingPoint);

    GLuint getBufferID() const;

private:
    GLuint bufferID;    // OpenGL buffer ID
    Type type;          // Type of buffer (e.g., VBO, EBO, SSBO)
    GLenum usage;       // Usage hint for the buffer (e.g., GL_STATIC_DRAW, GL_DYNAMIC_DRAW)

    // Returns the appropriate OpenGL target based on the buffer type
    GLenum getBufferTarget() const;
};

// Quad Class
class Quad {
public:
    Quad();
    ~Quad();

    void bind() const;
    void draw() const;

private:
    GLuint vao, vbo;
};

class Texture {
public:
    // Constructor to create an empty texture
    Texture(int width, int height, GLenum internalFormat = GL_RGBA, GLenum format = GL_RGBA, GLenum dataType = GL_UNSIGNED_BYTE);

    // Destructor
    ~Texture();

    // Resize the texture (used for recreating it with a different size)
    void resize(int newWidth, int newHeight);

    // Bind the texture to a texture unit (slot)
    void bind(GLuint slot = 0) const;

    // Set the texture to a solid red color (used for debugging)
    void setSolidRed();

    // Get the texture ID (useful for passing to shaders)
    GLuint getID() const { return textureID; }

private:
    GLuint textureID;    // OpenGL texture ID
    int width, height;   // Texture dimensions
    GLenum internalFormat;
    GLenum format;
    GLenum dataType;
    
    // Set texture parameters (filters, wrapping, etc.)
    void setTextureParams() const;
};

#endif // SHADER_HPP