#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include "xProgram.h"
#include "xCamera.h"
#include "sphere_generator.h"
#include "transfer.h"
#include <fstream>
#include <sstream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Callback functions for handling window resizing, mouse movement, and scroll input
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window); // Handles keyboard input
std::string readFile(const char* path); // Reads a file and returns its content as a string
GLuint loadHDRTexture(const char* path); // Loads an HDR texture from a file

// Global variables for screen dimensions, camera, and timing
int screenWidth = 5120;
int screenHeight = 2880;
xCamera camera(glm::vec3(0.0f, 0.0f, 5.0f)); // Camera positioned at (0, 0, 5)
float lastX = screenWidth / 2.0f; // Last mouse X position
float lastY = screenHeight / 2.0f; // Last mouse Y position
bool firstMouse = true; // Flag to handle the first mouse movement
float deltaTime = 0.0f; // Time between current and last frame
float lastFrame = 0.0f; // Time of the last frame
float shCoeffs[9][3]; // Spherical harmonics coefficients
float shaderInput[4][3];
float K2[3];
float placeWeight;
const float PI = 3.14159265359;

int main() {
    // Initialize GLFW and configure OpenGL context
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a GLFW window
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "SH Sphere", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Set callback functions for window resizing, mouse movement, and scroll input
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Disable cursor for FPS-style camera

    // Initialize GLAD to load OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Enable depth testing and sRGB framebuffer
    glEnable(GL_DEPTH_TEST);

    // Generate sphere geometry (vertices and indices)
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    generateSphere(vertices, indices);

    // Setup VAO, VBO, and EBO for the object sphere
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // Define vertex attributes (position and normal)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Setup VAO, VBO, and EBO for the background sphere (sky)
    GLuint skyVAO, skyVBO, skyEBO;
    glGenVertexArrays(1, &skyVAO);
    glGenBuffers(1, &skyVBO);
    glGenBuffers(1, &skyEBO);
    glBindVertexArray(skyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Load and compile shaders for the object and sky
    std::string vertCode = readFile("shader.vert");
    std::string fragCode = readFile("shader.frag");
    xProgram shader((char*)vertCode.c_str(), (char*)fragCode.c_str());

    std::string skyVert = readFile("sky.vert");
    std::string skyFrag = readFile("sky.frag");
    xProgram skyShader((char*)skyVert.c_str(), (char*)skyFrag.c_str());

    // Load HDR environment map and compute spherical harmonics coefficients
    std::string place = "grace";
    std::string floatFile = place + "_probe.float";
    std::string hdrFile = place + "_probe_mine.hdr";

    if (place == "stpeters") {
        placeWeight = 8.0f;
    }
    else if (place == "uffizi") {
        placeWeight = 0.33f;
    }
    else if (place == "grace") {
        placeWeight = 8.0f;
    }
    else {
        placeWeight = 1.0f;
    }

    int guessWidth = guessFloatWidth(floatFile.c_str());
    printf("Guessed Width : %d\n", guessWidth);

    computeSHFromFloatFile(floatFile.c_str(), guessWidth, shCoeffs);
    convertFloatToHDR(floatFile.c_str(), hdrFile.c_str(), guessWidth);
    GLuint hdrTexture = loadHDRTexture(hdrFile.c_str());

    // Scale SH coefficients for rendering
    for (int i = 0; i < 9; i++) {
        printf("%d : ", i);
        for (int j = 0; j < 3; j++) {
            shCoeffs[i][j] *= 0.1f;
            printf("%lf ", shCoeffs[i][j]);
        }
        printf("\n");
    }
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 3; j++) {
            shaderInput[i][j] = shCoeffs[i][j];
        }
    }
    for (int i = 0; i < 3; i++) {
        K2[i] = 0.0f;
        float sh0 = shCoeffs[0][i];
        float sh1 = shCoeffs[1][i];
        float sh2 = shCoeffs[2][i];
        float sh3 = shCoeffs[3][i];
        float sh4 = shCoeffs[4][i];
        float sh5 = shCoeffs[5][i];
        float sh6 = shCoeffs[6][i];
        float sh7 = shCoeffs[7][i];
        float sh8 = shCoeffs[8][i];

        glm::vec3 optDir = normalize(glm::vec3(-sh3, -sh1, sh2));

        float q4 = sqrt(15.0 / (4.0 * PI)) * optDir.x * optDir.y;
        float q5 = -sqrt(15.0 / (4.0 * PI)) * optDir.y * optDir.z;
        float q6 = sqrt(5.0 / (16.0 * PI)) * (3.0 * optDir.z * optDir.z - 1.0);
        float q7 = -sqrt(15.0 / (4.0 * PI)) * optDir.x * optDir.z;
        float q8 = sqrt(15.0 / (16.0 * PI)) * (optDir.x * optDir.x - optDir.y * optDir.y);

        K2[i] = q4 * sh4 + q5 * sh5 + q6 * sh6 + q7 * sh7 + q8 * sh8;
        K2[i] *= sqrt(4.0 * PI / 5.0);
    }
    for (int i = 0; i < 3; i++) {
        printf("%d : %lf\n", i, K2[i]);
    }

    glUseProgram(shader.program);
    glUniform1f(glGetUniformLocation(shader.program, "weight"), placeWeight);
    glUniform3fv(glGetUniformLocation(shader.program, "sh"), 9, &shaderInput[0][0]);
    glUniform1fv(glGetUniformLocation(shader.program, "k2"), 3, &K2[0]);

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate frame time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window); // Handle user input

        // Clear the screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render the background sphere (sky)
        glDepthFunc(GL_LEQUAL);
        glUseProgram(skyShader.program);
        glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // Remove translation for skybox
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / screenHeight, 0.1f, 100.0f);
        glm::mat4 skyModel = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f)); // Scale the sky sphere
        glUniformMatrix4fv(glGetUniformLocation(skyShader.program, "model"), 1, GL_FALSE, glm::value_ptr(skyModel));
        glUniformMatrix4fv(glGetUniformLocation(skyShader.program, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(skyShader.program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        glBindVertexArray(skyVAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glDepthFunc(GL_LESS);

        // Render the object sphere
        glUseProgram(shader.program);
        view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f); // Identity model matrix
        glUniformMatrix4fv(glGetUniformLocation(shader.program, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shader.program, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader.program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(shader.program, "cameraPos"), 1, glm::value_ptr(camera.Position));
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Terminate GLFW
    glfwTerminate();
    return 0;
}

// Reads a file and returns its content as a string
std::string readFile(const char* path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Loads an HDR texture from a file
GLuint loadHDRTexture(const char* path) {
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float* data = stbi_loadf(path, &width, &height, &nrComponents, 0);
    GLuint hdrTex = 0;

    if (data) {
        glGenTextures(1, &hdrTex);
        glBindTexture(GL_TEXTURE_2D, hdrTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F,
            width, height, 0, GL_RGB, GL_FLOAT, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    }
    else {
        std::cerr << "Failed to load HDR image." << std::endl;
    }
    return hdrTex;
}

// Callback for resizing the window
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Callback for mouse movement
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

// Callback for scroll input
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll((float)yoffset);
}

// Handles keyboard input
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);
}
