// Include necessary headers
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <map>
#include <string>

// Window dimensions
const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;

// Camera settings
glm::vec3 cameraPos = glm::vec3(0.0f, 16.0f, 48.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, -0.2f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f;

float yaw = -90.0f;
float pitch = -10.0f; // Slight downward angle
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
float fov = 45.0f;

bool firstMouse = true;

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
unsigned int compileShader(unsigned int type, const char* source);
double statsTracker(GLFWwindow* window, int* totalBlocks);
void RenderText(unsigned int shader, std::string text, float x, float y, float scale, glm::vec3 color);
bool isChunkInViewFrustum(const glm::vec3& chunkPos, const glm::mat4& view, const glm::mat4& projection, float chunkSize);

// Struct for character glyphs
struct Character {
    GLuint     TextureID;  // ID handle of the glyph texture
    glm::ivec2 Size;       // Size of glyph
    glm::ivec2 Bearing;    // Offset from baseline to left/top of glyph
    GLuint     Advance;    // Offset to advance to next glyph
};

// Map to store characters
std::map<GLchar, Character> Characters;

// VAO and VBO for text rendering
GLuint textVAO, textVBO;

// VAOs, VBOs, and EBO for cubes
unsigned int stoneVAO, stoneVBO;
unsigned int dirtVAO, dirtVBO;
unsigned int grassVAO, grassVBO;
unsigned int EBO;

int main()
{
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "3D Cubes with Camera", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0); // Disable VSync

    // Set callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Capture mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Load OpenGL function pointers using GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    // Build and compile our shader program
    // ------------------------------------
    // Vertex shader for cubes
    const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos; 
    layout (location = 1) in vec3 aColor;

    out vec3 ourColor;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        ourColor = aColor;
    }
    )";

    // Fragment shader for cubes
    const char* fragmentShaderSource = R"(
    #version 330 core
    in vec3 ourColor;
    out vec4 FragColor;

    void main()
    {
        FragColor = vec4(ourColor, 1.0);
    } 
    )";

    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    // Link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // Check for linking errors
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Set up vertex data and buffers and configure vertex attributes
    // --------------------------------------------------------------
    // Dirt block (brown color)
    float dirt_vertices[] = {
        // positions          // colors (brown)
       -0.5f, -0.5f, -0.5f,   0.6f, 0.3f, 0.0f, //0
        0.5f, -0.5f, -0.5f,   0.6f, 0.3f, 0.0f, //1
        0.5f,  0.5f, -0.5f,   0.6f, 0.3f, 0.0f, //2
       -0.5f,  0.5f, -0.5f,   0.6f, 0.3f, 0.0f, //3
       -0.5f, -0.5f,  0.5f,   0.6f, 0.3f, 0.0f, //4
        0.5f, -0.5f,  0.5f,   0.6f, 0.3f, 0.0f, //5
        0.5f,  0.5f,  0.5f,   0.6f, 0.3f, 0.0f, //6
       -0.5f,  0.5f,  0.5f,   0.6f, 0.3f, 0.0f  //7
    };

    // Stone block (gray color)
    float stone_vertices[] = {
        // positions          // colors (gray)
       -0.5f, -0.5f, -0.5f,   0.5f, 0.5f, 0.5f, //0
        0.5f, -0.5f, -0.5f,   0.5f, 0.5f, 0.5f, //1
        0.5f,  0.5f, -0.5f,   0.5f, 0.5f, 0.5f, //2
       -0.5f,  0.5f, -0.5f,   0.5f, 0.5f, 0.5f, //3
       -0.5f, -0.5f,  0.5f,   0.5f, 0.5f, 0.5f, //4
        0.5f, -0.5f,  0.5f,   0.5f, 0.5f, 0.5f, //5
        0.5f,  0.5f,  0.5f,   0.5f, 0.5f, 0.5f, //6
       -0.5f,  0.5f,  0.5f,   0.5f, 0.5f, 0.5f  //7
    };

    // Grass block (green color)
    float grass_vertices[] = {
        // positions          // colors (green)
       -0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 0.0f, //0
        0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 0.0f, //1
        0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f, //2
       -0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f, //3
       -0.5f, -0.5f,  0.5f,   0.0f, 1.0f, 0.0f, //4
        0.5f, -0.5f,  0.5f,   0.0f, 1.0f, 0.0f, //5
        0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 0.0f, //6
       -0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 0.0f  //7
    };

    // Indices for EBO
    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0,   // back face
        4, 5, 6, 6, 7, 4,   // front face
        0, 1, 5, 5, 4, 0,   // bottom face
        2, 3, 7, 7, 6, 2,   // top face
        0, 3, 7, 7, 4, 0,   // left face
        1, 2, 6, 6, 5, 1    // right face
    };

    // Generate EBO
    glGenBuffers(1, &EBO);

    // Stone block setup
    glGenVertexArrays(1, &stoneVAO);
    glGenBuffers(1, &stoneVBO);

    glBindVertexArray(stoneVAO);

    glBindBuffer(GL_ARRAY_BUFFER, stoneVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(stone_vertices), stone_vertices, GL_STATIC_DRAW);

    // Bind EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Dirt block setup
    glGenVertexArrays(1, &dirtVAO);
    glGenBuffers(1, &dirtVBO);

    glBindVertexArray(dirtVAO);

    glBindBuffer(GL_ARRAY_BUFFER, dirtVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(dirt_vertices), dirt_vertices, GL_STATIC_DRAW);

    // Bind EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // EBO data is already set, no need to call glBufferData again

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Grass block setup
    glGenVertexArrays(1, &grassVAO);
    glGenBuffers(1, &grassVBO);

    glBindVertexArray(grassVAO);

    glBindBuffer(GL_ARRAY_BUFFER, grassVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(grass_vertices), grass_vertices, GL_STATIC_DRAW);

    // Bind EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // EBO data is already set, no need to call glBufferData again

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Unbind VAO
    glBindVertexArray(0);

    // Initialize FreeType for text rendering (code omitted for brevity)
    // ...

    // Render loop
    // -----------
    int total_rendered_blocks = 0;

    while (!glfwWindowShouldClose(window))
    {
        // Per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input
        // -----
        processInput(window);

        // Render
        // ------
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f); // Light sky blue background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Activate shader
        glUseProgram(shaderProgram);

        // Camera/view transformation
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        // Projection
        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)WIDTH / (float)HEIGHT, 0.1f, 500.0f);

        // Retrieve the matrix uniform locations
        unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
        unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
        unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");

        // Pass the matrices to the shader
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Define the number of chunks for each material
        int stoneChunks = 13;
        int dirtChunks = 2;
        int grassChunks = 1;
        int totalChunks = stoneChunks + dirtChunks + grassChunks;
        const int chunkSize = 16;

        total_rendered_blocks = 0;

        // Render loop for chunks
        for (int chunkY = 0; chunkY < totalChunks; chunkY++) {

            // Initialize the chunk data (make it hollow)
            int chunk[chunkSize][chunkSize][chunkSize];

            for (int x = 0; x < chunkSize; x++) {
                for (int y = 0; y < chunkSize; y++) {
                    for (int z = 0; z < chunkSize; z++) {
                        // Set outer blocks to 1 (solid), inner blocks to 0 (air)
                        if (x == 0 || x == chunkSize - 1 ||
                            y == 0 || y == chunkSize - 1 ||
                            z == 0 || z == chunkSize - 1) {
                            chunk[x][y][z] = 1; // Solid block
                        }
                        else {
                            chunk[x][y][z] = 0; // Air
                        }
                    }
                }
            }

            // Bind the appropriate VAO
            if (chunkY < stoneChunks) {
                glBindVertexArray(stoneVAO);
            }
            else if (chunkY < stoneChunks + dirtChunks) {
                glBindVertexArray(dirtVAO);
            }
            else {
                glBindVertexArray(grassVAO);
            }

            // Frustum culling
            if (!isChunkInViewFrustum(glm::vec3(0.0f, chunkY * chunkSize, 0.0f), view, projection, chunkSize)) {
                continue; // Skip rendering if chunk is not in view
            }

            // Render blocks within this chunk
            for (int x = 0; x < chunkSize; x++) {
                for (int y = 0; y < chunkSize; y++) {
                    for (int z = 0; z < chunkSize; z++) {
                        if (chunk[x][y][z] == 0) continue; // Skip air blocks

                        glm::vec3 blockPos(x * 1.0f, (chunkY * chunkSize + y) * 1.0f, z * 1.0f);
                        glm::mat4 model = glm::translate(glm::mat4(1.0f), blockPos);
                        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

                        // Check and render only the visible faces

                        // Check -X face
                        if (x == 0 || chunk[x - 1][y][z] == 0) {
                            // Render left face
                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(24 * sizeof(GLuint)));
                        }

                        // Check +X face
                        if (x == chunkSize - 1 || chunk[x + 1][y][z] == 0) {
                            // Render right face
                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(30 * sizeof(GLuint)));
                        }

                        // Check -Y face
                        if (y == 0 || chunk[x][y - 1][z] == 0) {
                            // Render bottom face
                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(12 * sizeof(GLuint)));
                        }

                        // Check +Y face
                        if (y == chunkSize - 1 || chunk[x][y + 1][z] == 0) {
                            // Render top face
                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(18 * sizeof(GLuint)));
                        }

                        // Check -Z face
                        if (z == 0 || chunk[x][y][z - 1] == 0) {
                            // Render back face
                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(0 * sizeof(GLuint)));
                        }

                        // Check +Z face
                        if (z == chunkSize - 1 || chunk[x][y][z + 1] == 0) {
                            // Render front face
                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(6 * sizeof(GLuint)));
                        }

                        total_rendered_blocks++;
                    }
                }
            }
        }

        // Calculate FPS and render text (code omitted for brevity)
		//call the statsTracker function to calculate the FPS and update the window title
        double fps = statsTracker(window, &total_rendered_blocks);

        // Swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();

		
    }

    // De-allocate resources
    // ---------------------
    glDeleteVertexArrays(1, &stoneVAO);
    glDeleteBuffers(1, &stoneVBO);

    glDeleteVertexArrays(1, &dirtVAO);
    glDeleteBuffers(1, &dirtVBO);

    glDeleteVertexArrays(1, &grassVAO);
    glDeleteBuffers(1, &grassVBO);

    glDeleteBuffers(1, &EBO);

    glDeleteProgram(shaderProgram);

    // Terminate GLFW
    glfwTerminate();
    return 0;
}

// Callback function called when the window is resized
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // Adjust viewport
    glViewport(0, 0, width, height);
}

// Process all input
void processInput(GLFWwindow* window)
{
    float cameraSpeed = 10.0f * deltaTime;
    //if click esc show mouse cursor and do not focus on window

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
	else {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}

    //enable wireframe
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraSpeed *= 2;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraUp;


    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
        // Toggle fullscreen
        if (glfwGetWindowMonitor(window) == NULL) {
            // Get the primary monitor
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            // Get the video mode of the monitor
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            // Set the window to fullscreen
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        }
        else {
            // Leave fullscreen
            glfwSetWindowMonitor(window, NULL, 100, 100, WIDTH, HEIGHT, 0);
        }
    }
}

// Callback function called when the mouse is moved
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since y-coordinates range from bottom to top
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f; // Change this value to your liking
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Make sure that when pitch is out of bounds, screen doesn't get flipped
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

// Callback function called when the mouse scroll wheel is used
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (fov >= 1.0f && fov <= 90.0f)
        fov -= static_cast<float>(yoffset);
    if (fov <= 1.0f)
        fov = 1.0f;
    if (fov >= 90.0f)
        fov = 90.0f;
}

// Function to compile a shader from source code
unsigned int compileShader(unsigned int type, const char* source)
{
    unsigned int shaderID = glCreateShader(type);
    glShaderSource(shaderID, 1, &source, NULL);
    glCompileShader(shaderID);

    // Error handling
    int success;
    char infoLog[512];
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shaderID, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    return shaderID;
}

// Function to calculate FPS and update window title
double statsTracker(GLFWwindow* window, int* totalBlocks)
{
    static double previousSeconds = 0.0;
    static int frameCount = 0;
    static double lastFps = 0.0;  // Store the last valid FPS value
    double currentSeconds = glfwGetTime();
    double elapsedSeconds = currentSeconds - previousSeconds;

    frameCount++;

    // Update FPS every 0.1 seconds
    if (elapsedSeconds >= 0.1)
    {
        lastFps = frameCount / elapsedSeconds;  // Calculate FPS

        // Display FPS and block count in window title (optional)
        char tmp[128];
        snprintf(tmp, sizeof(tmp), "OpenGL - 3D Cubes with Camera (%.1f FPS) - Blocks: %d", lastFps, *totalBlocks);
        glfwSetWindowTitle(window, tmp);

        // Reset frame count and time for next FPS calculation
        frameCount = 0;
        previousSeconds = currentSeconds;
    }

    return lastFps;  // Return the last valid FPS value, even if not updated this frame
}

// Function to render text on the screen
void RenderText(unsigned int shader, std::string text, float x, float y, float scale, glm::vec3 color)
{
    // Activate corresponding render state	
    glUseProgram(shader);
    glUniform3f(glGetUniformLocation(shader, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    // Iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * (scale / 2);
        float h = ch.Size.y * (scale / 2);
        // Update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };
        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Advance cursor for next glyph
        x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

bool isChunkInViewFrustum(const glm::vec3& chunkPos, const glm::mat4& view, const glm::mat4& projection, float chunkSize) {
    // Define the 8 corners of the chunk
    glm::vec3 corners[8];
    corners[0] = chunkPos;
    corners[1] = chunkPos + glm::vec3(chunkSize, 0, 0);
    corners[2] = chunkPos + glm::vec3(0, chunkSize, 0);
    corners[3] = chunkPos + glm::vec3(0, 0, chunkSize);
    corners[4] = chunkPos + glm::vec3(chunkSize, chunkSize, 0);
    corners[5] = chunkPos + glm::vec3(chunkSize, 0, chunkSize);
    corners[6] = chunkPos + glm::vec3(0, chunkSize, chunkSize);
    corners[7] = chunkPos + glm::vec3(chunkSize, chunkSize, chunkSize);

    // Check if any of the chunk's corners are inside the view frustum
    for (int i = 0; i < 8; i++) {
        glm::vec4 clipSpacePos = projection * view * glm::vec4(corners[i], 1.0f);

        // Perform the frustum check for each corner
        if (clipSpacePos.x > -clipSpacePos.w && clipSpacePos.x < clipSpacePos.w &&
            clipSpacePos.y > -clipSpacePos.w && clipSpacePos.y < clipSpacePos.w &&
            clipSpacePos.z > -clipSpacePos.w && clipSpacePos.z < clipSpacePos.w) {
            return true; // If any corner is inside the frustum, the chunk is visible
        }
    }

    // If no corner is visible, the chunk is not in the view frustum
    return false;
}
