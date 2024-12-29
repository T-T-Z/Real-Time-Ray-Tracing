#include "includes.hpp"

class Shader {
public:
    Shader(const std::map<GLenum, std::string>& shaderPaths);
    ~Shader();

    void use() const;
    GLuint getProgram() const { return program; }

    // Declare the function here, but don't define it
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

Shader::Shader(const std::map<GLenum, std::string>& shaderPaths) {
    std::vector<GLuint> shaders;

    for (const auto& [type, path] : shaderPaths) {
        std::string source = loadShaderSource(path);
        GLuint shader = compileShader(type, source);
        shaders.push_back(shader);
    }

    program = createProgram(shaders);

    // Clean up shaders (they're already attached to the program)
    for (GLuint shader : shaders) {
        glDeleteShader(shader);
    }
}

Shader::~Shader() {
    glDeleteProgram(program);
}

void Shader::use() const {
    glUseProgram(program);
}

GLuint Shader::compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* sourceCStr = source.c_str();
    glShaderSource(shader, 1, &sourceCStr, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        char* log = new char[logLength];
        glGetShaderInfoLog(shader, logLength, &logLength, log);
        std::cerr << "Shader Compilation Failed (" << type << "): " << log << std::endl;
        delete[] log;
    }

    return shader;
}

GLuint Shader::createProgram(const std::vector<GLuint>& shaders) {
    GLuint program = glCreateProgram();

    // Attach all shaders
    for (GLuint shader : shaders) {
        glAttachShader(program, shader);
    }

    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        char* log = new char[logLength];
        glGetProgramInfoLog(program, logLength, &logLength, log);
        std::cerr << "Program Linking Failed: " << log << std::endl;
        delete[] log;
    }

    return program;
}

void Shader::setMat4(const std::string& name, const glm::mat4& matrix) const {
    GLint location = glGetUniformLocation(program, name.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, &matrix[0][0]);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    GLint location = glGetUniformLocation(program, name.c_str());
    glUniform3fv(location, 1, &value[0]);
}

void Shader::setUInt(const std::string& name, unsigned int value) const {
    GLint location = glGetUniformLocation(program, name.c_str());
    glUniform1ui(location, value);
}

void Shader::setTexture(const std::string& name, GLuint textureID, GLuint unit) const {
    GLint location = glGetUniformLocation(program, name.c_str());
    if (location == -1) {
        std::cerr << "Warning: Texture uniform " << name << " not found in shader!" << std::endl;
        return;
    }

    glActiveTexture(GL_TEXTURE0 + unit); // Activate the texture unit
    glBindTexture(GL_TEXTURE_2D, textureID); // Bind the texture

    glUniform1i(location, unit); // Set the uniform to the texture unit number
}

// New method for binding images to compute shaders
void Shader::setImage(const std::string& name, GLuint textureID, GLuint bindingPoint, GLenum format) const {
    GLint location = glGetUniformLocation(program, name.c_str());
    if (location == -1) {
        std::cerr << "Warning: Image uniform " << name << " not found in shader!" << std::endl;
        return;
    }

    // Bind the texture to an image binding point for read/write in compute shaders
    glBindImageTexture(bindingPoint, textureID, 0, GL_FALSE, 0, GL_READ_WRITE, format);
}

std::string Shader::loadShaderSource(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open shader file: " << filepath << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

class Buffer {
public:
    enum Type {
        VERTEX_BUFFER,
        ELEMENT_BUFFER,
        STORAGE_BUFFER,
        UNIFORM_BUFFER
    };

    // Constructor for a buffer object
    Buffer(Type type, GLenum usage = GL_STATIC_DRAW)
        : type(type), usage(usage), bufferID(0) {}

    // Destructor
    ~Buffer() {
        if (bufferID != 0) {
            glDeleteBuffers(1, &bufferID);
        }
    }

    // Generate the buffer
    void generateBuffer() {
        glGenBuffers(1, &bufferID);
    }

    // Bind the buffer to a specific target (e.g., GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER, etc.)
    void bind() const {
        GLenum target = getBufferTarget();
        glBindBuffer(target, bufferID);
    }

    // Unbind the buffer
    void unbind() const {
        GLenum target = getBufferTarget();
        glBindBuffer(target, 0);
    }

    // Upload data to the buffer
    void uploadData(const void* data, size_t size) {
        bind();
        glBufferData(getBufferTarget(), size, data, usage);
    }

    // Map the buffer for reading or writing
    void* mapBuffer(GLenum access = GL_READ_WRITE) {
        bind();
        return glMapBuffer(getBufferTarget(), access);
    }

    // Unmap the buffer
    void unmapBuffer() const {
        glUnmapBuffer(getBufferTarget());
    }

    // Set a buffer as an SSBO or UBO (with binding points)
    void setBufferBinding(GLuint bindingPoint) {
        bind();
        if (type == STORAGE_BUFFER) {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, bufferID);
        } else if (type == UNIFORM_BUFFER) {
            glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, bufferID);
        }
    }

    GLuint getBufferID() const { return bufferID; }

private:
    GLuint bufferID;    // OpenGL buffer ID
    Type type;          // Type of buffer (e.g., VBO, EBO, SSBO)
    GLenum usage;       // Usage hint for the buffer (e.g., GL_STATIC_DRAW, GL_DYNAMIC_DRAW)

    // Returns the appropriate OpenGL target based on the buffer type
    GLenum getBufferTarget() const {
        switch (type) {
            case VERTEX_BUFFER: return GL_ARRAY_BUFFER;
            case ELEMENT_BUFFER: return GL_ELEMENT_ARRAY_BUFFER;
            case STORAGE_BUFFER: return GL_SHADER_STORAGE_BUFFER;
            case UNIFORM_BUFFER: return GL_UNIFORM_BUFFER;
            default: return GL_ARRAY_BUFFER;  // Default to vertex buffer
        }
    }
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

Quad::Quad() {
    float vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

Quad::~Quad() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

void Quad::bind() const {
    glBindVertexArray(vao);
}

void Quad::draw() const {
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

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

// Constructor: creates a blank texture with the specified size and format
Texture::Texture(int width, int height, GLenum internalFormat, GLenum format, GLenum dataType)
    : width(width), height(height), internalFormat(internalFormat), format(format), dataType(dataType) {
    glGenTextures(1, &textureID);   // Generate texture ID
    bind();  // Bind the texture immediately

    // Create an empty 2D texture
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, dataType, nullptr);
    
    // Set texture parameters
    setTextureParams();
}

// Destructor: delete the texture when the object is destroyed
Texture::~Texture() {
    glDeleteTextures(1, &textureID);
}

// Resize the texture (recreates the texture with a new size)
void Texture::resize(int newWidth, int newHeight) {
    width = newWidth;
    height = newHeight;
    
    bind();  // Bind the texture
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, dataType, nullptr); // Update the texture size

    // Reset texture parameters after resizing
    setTextureParams();
}

// Bind the texture to a specified texture unit (slot)
void Texture::bind(GLuint slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);  // Activate the correct texture unit
    glBindTexture(GL_TEXTURE_2D, textureID);  // Bind the texture
}

// Set the texture to a solid red color (used for debugging)
void Texture::setSolidRed() {
    std::vector<unsigned char> redData(width * height * 4, 0);  // 4 bytes per pixel (RGBA)
    
    // Fill the texture with red color (255 red, 0 green, 0 blue, 255 alpha)
    for (int i = 0; i < width * height; ++i) {
        redData[i * 4 + 0] = 255;  // Red channel: full intensity
        redData[i * 4 + 1] = 0;    // Green channel: no intensity
        redData[i * 4 + 2] = 0;    // Blue channel: no intensity
        redData[i * 4 + 3] = 255;  // Alpha channel: full opacity
    }

    bind();  // Bind the texture
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, dataType, redData.data());  // Set the texture data
}

// Set texture parameters such as filtering and wrapping
void Texture::setTextureParams() const {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Linear filtering for minification
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Linear filtering for magnification
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // Wrap horizontally
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Wrap vertically
}