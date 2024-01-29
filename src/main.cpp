#include <stdio.h>
#include <stdlib.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <shader/shader.hpp>  // Make sure this is correctly included
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include "model/objload.hpp"  // Make sure this is correctly included
#include <iostream>
#include <vector>
#include <cmath>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

GLuint shaderProgram;
GLint projLoc;



// Function to create sphere data
void createSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices, float radius, unsigned int numSlices, unsigned int numStacks, unsigned int &numVertices) {
    vertices.clear();
    indices.clear();

    for (unsigned int stack = 0; stack <= numStacks; ++stack) {
        float phi = M_PI * float(stack) / float(numStacks);
        for (unsigned int slice = 0; slice <= numSlices; ++slice) {
            float theta = 2.0f * M_PI * float(slice) / float(numSlices);
            float x = cosf(theta) * sinf(phi);
            float y = cosf(phi);
            float z = sinf(theta) * sinf(phi);

            vertices.push_back(radius * x);
            vertices.push_back(radius * y);
            vertices.push_back(radius * z);

            float u = (float)slice / numSlices;
            float v = (float)stack / numStacks;
            vertices.push_back(u);
            vertices.push_back(v);
        }
    }

    for (unsigned int stack = 0; stack < numStacks; ++stack) {
        for (unsigned int slice = 0; slice < numSlices; ++slice) {
            indices.push_back((stack + 0) * (numSlices + 1) + slice);
            indices.push_back((stack + 1) * (numSlices + 1) + slice);
            indices.push_back((stack + 0) * (numSlices + 1) + slice + 1);

            indices.push_back((stack + 0) * (numSlices + 1) + slice + 1);
            indices.push_back((stack + 1) * (numSlices + 1) + slice);
            indices.push_back((stack + 1) * (numSlices + 1) + slice + 1);
        }
    }

    numVertices = vertices.size() / 5;
}


glm::mat4 projection;
float zoomLevel = 45.0f;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    zoomLevel -= (float)yoffset * 5.0f; 
    if (zoomLevel < 1.0f)
        zoomLevel = 1.0f;
    if (zoomLevel > 45.0f)
        zoomLevel = 45.0f;

    // Update the projection matrix here
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    projection = glm::perspective(glm::radians(zoomLevel), float(width) / float(height), 0.1f, 100.0f);

    // Update the projection matrix uniform
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
}



unsigned int loadCubemap(std::vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}


int main() {
    float earthRotationAngle = 0.0f;
    float earthOrbitAngle = 0.0f;

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Window", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    glfwSetScrollCallback(window, scroll_callback);
    glEnable(GL_DEPTH_TEST);


    GLuint skyboxShaderProgram = LoadShaders("include/shader/skyboxVertexShader.vertexshader", "include/shader/skyboxFragmentShader.fragmentshader");

    shaderProgram = LoadShaders("include/shader/SimpleVertexShader.vertexshader", "include/shader/SimpleFragmentShader.fragmentshader");
    if (shaderProgram == 0) {
        std::cerr << "Failed to compile/link shaders\n";
        return -1;
    }

    projLoc = glGetUniformLocation(shaderProgram, "projection");

    // Earth
    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    unsigned int numVertices = 0;
    createSphere(sphereVertices, sphereIndices, 1.0f, 36, 18, numVertices);

    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // Sun
    std::vector<float> sunVertices;
    std::vector<unsigned int> sunIndices;
    unsigned int numSunVertices = 0;
    createSphere(sunVertices, sunIndices, 5.0f, 36, 18, numSunVertices);

    GLuint sunVao, sunVbo, sunEbo;
    glGenVertexArrays(1, &sunVao);
    glGenBuffers(1, &sunVbo);
    glGenBuffers(1, &sunEbo);

    glBindVertexArray(sunVao);
    glBindBuffer(GL_ARRAY_BUFFER, sunVbo);
    glBufferData(GL_ARRAY_BUFFER, sunVertices.size() * sizeof(float), sunVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sunEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sunIndices.size() * sizeof(unsigned int), sunIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // Define faces for the skybox
    std::vector<std::string> faces{
        "src/sky.jpg",
        "src/sky.jpg",
        "src/sky.jpg",
        "src/sky.jpg",
        "src/sky.jpg",
        "src/sky.jpg"
    };

    // Load the cubemap texture
    unsigned int cubemapTexture = loadCubemap(faces);



float skyboxVertices[] = {
    // positions          
    // Back face
    -1.0f, -1.0f, -1.0f, // Bottom-left
     1.0f,  1.0f, -1.0f, // top-right
     1.0f, -1.0f, -1.0f, // bottom-right         
     1.0f,  1.0f, -1.0f, // top-right
    -1.0f, -1.0f, -1.0f, // bottom-left
    -1.0f,  1.0f, -1.0f, // top-left
    // Front face
    -1.0f, -1.0f,  1.0f, // bottom-left
     1.0f, -1.0f,  1.0f, // bottom-right
     1.0f,  1.0f,  1.0f, // top-right
     1.0f,  1.0f,  1.0f, // top-right
    -1.0f,  1.0f,  1.0f, // top-left
    -1.0f, -1.0f,  1.0f, // bottom-left
    // Left face
    -1.0f,  1.0f,  1.0f, // top-right
    -1.0f,  1.0f, -1.0f, // top-left
    -1.0f, -1.0f, -1.0f, // bottom-left
    -1.0f, -1.0f, -1.0f, // bottom-left
    -1.0f, -1.0f,  1.0f, // bottom-right
    -1.0f,  1.0f,  1.0f, // top-right
    // Right face
     1.0f,  1.0f,  1.0f, // top-left
     1.0f, -1.0f, -1.0f, // bottom-right
     1.0f,  1.0f, -1.0f, // top-right         
     1.0f, -1.0f, -1.0f, // bottom-right
     1.0f,  1.0f,  1.0f, // top-left
     1.0f, -1.0f,  1.0f, // bottom-left     
    // Bottom face
    -1.0f, -1.0f, -1.0f, // top-right
     1.0f, -1.0f, -1.0f, // top-left
     1.0f, -1.0f,  1.0f, // bottom-left
     1.0f, -1.0f,  1.0f, // bottom-left
    -1.0f, -1.0f,  1.0f, // bottom-right
    -1.0f, -1.0f, -1.0f, // top-right
    // Top face
    -1.0f,  1.0f, -1.0f, // bottom-left
     1.0f,  1.0f,  1.0f, // top-right
     1.0f,  1.0f, -1.0f, // top-left     
     1.0f,  1.0f,  1.0f, // top-right
    -1.0f,  1.0f, -1.0f, // bottom-left
    -1.0f,  1.0f,  1.0f  // bottom-right
};


    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);


    // Load Earth texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int texWidth, texHeight, texChannels;
    unsigned char* data = stbi_load("src/Texture.jpg", &texWidth, &texHeight, &texChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cerr << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Load Sun texture
    GLuint sunTexture;
    glGenTextures(1, &sunTexture);
    glBindTexture(GL_TEXTURE_2D, sunTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    unsigned char* sunData = stbi_load("src/sun.jpg", &texWidth, &texHeight, &texChannels, 0);
    if (sunData) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, sunData);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cerr << "Failed to load Sun texture" << std::endl;
    }
    stbi_image_free(sunData);
    glBindTexture(GL_TEXTURE_2D, 0);

    glUseProgram(shaderProgram);
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    glm::vec3 cameraPos = glm::vec3(0, 0, 20); // Adjust as needed
    glm::vec3 cameraTarget = glm::vec3(0, 0, 0); // Assuming the Sun is at the origin
    glm::vec3 cameraUp = glm::vec3(0, 1, 0); // Up is in the positive Y direction

    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);

    // glm::mat4 view = glm::lookAt(glm::vec3(4, 3, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    glm::mat4 model = glm::mat4(1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    while (!glfwWindowShouldClose(window)) {
        earthRotationAngle += 0.001f;  // Controls the speed of Earth's spin
        earthOrbitAngle += 0.00001f;  
        int newWidth, newHeight;
        glfwGetFramebufferSize(window, &newWidth, &newHeight);
        if (newWidth != width || newHeight != height) {
            width = newWidth;
            height = newHeight;
            glm::mat4 projection = glm::perspective(glm::radians(zoomLevel), float(width) / float(height), 0.1f, 100.0f);
            glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
            glViewport(0, 0, width, height);
        }

        glClearColor(0.0025f, 0.1071f, 0.2121f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // Render Skybox
        glDepthMask(GL_FALSE); 
        glUseProgram(skyboxShaderProgram);
        glm::mat4 skyboxView = glm::mat4(glm::mat3(view)); 
        glUniformMatrix4fv(glGetUniformLocation(skyboxShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(skyboxView));
        glUniformMatrix4fv(glGetUniformLocation(skyboxShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glUniform1i(glGetUniformLocation(skyboxShaderProgram, "skybox"), 0);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE); // Re-enable depth writing

    
        glEnable(GL_DEPTH_TEST);




        glUseProgram(shaderProgram);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        glUniform1i(glGetUniformLocation(shaderProgram, "textureSelector"), 2); // Use Sun texture

        // Render the Sun
        glm::mat4 sunModel = glm::mat4(1.0f);
        sunModel = glm::translate(sunModel, glm::vec3(0.0f, 0.0f, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(sunModel));
        glBindTexture(GL_TEXTURE_2D, sunTexture);
        glBindVertexArray(sunVao);
        glDrawElements(GL_TRIANGLES, sunIndices.size(), GL_UNSIGNED_INT, 0);

        glUniform1i(glGetUniformLocation(shaderProgram, "textureSelector"), 1); // Use Earth texture

        // Render the Earth
        glm::mat4 earthModel = glm::mat4(1.0f);
        //earthModel = glm::translate(earthModel, glm::vec3(0.0f, 0.0f, -5.0f));
        earthModel = glm::rotate(earthModel, earthRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f)); // Earth's own rotation
        earthModel = glm::translate(earthModel, glm::vec3(10.0f, 0.0f, 0.0f)); // Distance from Sun
        earthModel = glm::rotate(earthModel, earthOrbitAngle, glm::vec3(0.0f, 1.0f, 0.0f)); // Earth's orbit around the Sun

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(earthModel));
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteBuffers(1, &sunVbo);
    glDeleteVertexArrays(1, &sunVao);
    glDeleteBuffers(1, &sunEbo);
    glDeleteTextures(1, &sunTexture);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(1, &texture);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
