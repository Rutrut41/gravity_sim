#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <random>

const char* vertexShaderSource = R"glsl(#version 330 core
layout(location=0)in vec3 aPos;uniform mat4 model;uniform mat4 view;uniform mat4 projection;
void main(){gl_Position=projection*view*model*vec4(aPos,1.0);})glsl";

const char* fragmentShaderSource = R"glsl(
#version 330 core
out vec4 FragColor;
uniform vec4 objectColor; // Add this uniform
void main() {
    FragColor = objectColor; // Use the uniform color
}
)glsl";

bool running = true;
bool pause = false;
bool mouseCoupled = true;
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  1.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
float lastX = 400.0, lastY = 300.0;
float yaw = -90;
float pitch =0.0;
float deltaTime = 0.0;
float lastFrame = 0.0;
float globalTime = 0.0f;
int windowWidth = 800;
int windowHeight = 600;
GLuint shaderProgram;
GLint objectColorLoc;
GLint projectionLoc;
double currentMouseX = 0.0;
double currentMouseY = 0.0;
float initialFOV = 45.0f;
float currentFOV = 45.0f;
bool showSelectionBox = true;
glm::vec3 initialCameraPos = glm::vec3(0.0f, 1000.0f, 15000.0f);
bool isDragging = false;
double lastDragY = 0.0;
float focusY = 500.0f;
float dragSensitivity = 0.0005f;

const double G_BASE = 6.6743e-11; // m^3 kg^-1 s^-2
double gravityMultiplier = 1.0;
const float c = 299792458.0;
float initMass = 5.0f * pow(10, 20) / 5;

GLFWwindow* StartGLU();
GLuint CreateShaderProgram(const char* vertexSource, const char* fragmentSource);
void CreateVBOVAO(GLuint& VAO, GLuint& VBO, const float* vertices, size_t vertexCount);
void UpdateCam(GLuint shaderProgram, glm::vec3 cameraPos);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void mouse_callback(GLFWwindow* window, double xpos, double ypos);
glm::vec3 sphericalToCartesian(float r, float theta, float phi);
void DrawGrid(GLuint shaderProgram, GLuint gridVAO, size_t vertexCount);
void DrawSelectionBox();
void DrawOrbits(GLuint shaderProgram);
void DrawGravityDisplay();


class Object {
    public:
        GLuint VAO, VBO;
        glm::vec3 position = glm::vec3(400, 300, 0);
        glm::vec3 velocity = glm::vec3(0, 0, 0);
        size_t vertexCount;
        glm::vec4 color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

        bool Initalizing = false;
        bool Launched = false;
        bool target = false;

        float mass;
        float density;  // kg / m^3  HYDROGEN
        float radius;

        glm::vec3 LastPos = position;
        std::vector<glm::vec3> orbitTrail;

        Object(glm::vec3 initPosition, glm::vec3 initVelocity, float mass, float density = 3344) {
            this->position = initPosition;
            this->velocity = initVelocity;
            this->mass = mass;
            this->density = density;
            this->radius = pow(((3 * this->mass/this->density)/(4 * 3.14159265359)), (1.0f/3.0f)) / 100000;

            orbitTrail.push_back(initPosition);

            // Generate vertices (centered at origin)
            std::vector<float> vertices = Draw();
            vertexCount = vertices.size();

            CreateVBOVAO(VAO, VBO, vertices.data(), vertexCount);
        }

        std::vector<float> Draw() {
            std::vector<float> vertices;
            int stacks = 10;
            int sectors = 10;

            for(float i = 0.0f; i <= stacks; ++i){
                float theta1 = (i / stacks) * glm::pi<float>();
                float theta2 = (i+1) / stacks * glm::pi<float>();
                for (float j = 0.0f; j < sectors; ++j){
                    float phi1 = j / sectors * 2 * glm::pi<float>();
                    float phi2 = (j+1) / sectors * 2 * glm::pi<float>();
                    glm::vec3 v1 = sphericalToCartesian(radius, theta1, phi1);
                    glm::vec3 v2 = sphericalToCartesian(radius, theta1, phi2);
                    glm::vec3 v3 = sphericalToCartesian(radius, theta2, phi1);
                    glm::vec3 v4 = sphericalToCartesian(radius, theta2, phi2);

                    auto applyDeformation = [](glm::vec3& v) {
                        float len = glm::length(v);
                        glm::vec3 dir = glm::normalize(v);
                        float deform = 1.0f + 0.4f * sin(globalTime * 2.5f + dir.x * 3.0f)
                                             + 0.3f * cos(globalTime * 1.8f + dir.y * 2.5f)
                                             + 0.35f * sin(globalTime * 3.2f + dir.z * 2.0f)
                                             + 0.25f * sin(globalTime * 1.4f);
                        v = dir * len * deform;
                    };

                    applyDeformation(v1);
                    applyDeformation(v2);
                    applyDeformation(v3);
                    applyDeformation(v4);

                    vertices.insert(vertices.end(), {v1.x, v1.y, v1.z});
                    vertices.insert(vertices.end(), {v2.x, v2.y, v2.z});
                    vertices.insert(vertices.end(), {v3.x, v3.y, v3.z});

                    vertices.insert(vertices.end(), {v2.x, v2.y, v2.z});
                    vertices.insert(vertices.end(), {v4.x, v4.y, v4.z});
                    vertices.insert(vertices.end(), {v3.x, v3.y, v3.z});
                }
            }
            return vertices;
        }
        
        void UpdatePos(){
            this->position[0] += this->velocity[0] / 94;
            this->position[1] += this->velocity[1] / 94;
            this->position[2] += this->velocity[2] / 94;
            this->radius = pow(((3 * this->mass/this->density)/(4 * 3.14159265359)), (1.0f/3.0f)) / 100000;
        }
        void UpdateVertices() {
            // Generate new vertices with current radius
            std::vector<float> vertices = Draw();
            
            // Update the VBO with new vertex data
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        }
        glm::vec3 GetPos() const {
            return this->position;
        }
        void accelerate(float x, float y, float z){
            this->velocity[0] += x / 96;
            this->velocity[1] += y / 96;
            this->velocity[2] += z / 96;
        }
        float CheckCollision(const Object& other) {
            float dx = other.position[0] - this->position[0];
            float dy = other.position[1] - this->position[1];
            float dz = other.position[2] - this->position[2];
            float distance = std::pow(dx*dx + dy*dy + dz*dz, (1.0f/2.0f));
            if (other.radius + this->radius > distance){
                return -0.2f;
            }
            return 1.0f;
        }

        void UpdateOrbitTrail() {
            orbitTrail.push_back(position);
            if (orbitTrail.size() > 2500) {
                orbitTrail.erase(orbitTrail.begin());
            }
        }
};
std::vector<Object> objs = {};

std::vector<float> CreateGridVertices(float size, int divisions, const std::vector<Object>& objs);

GLuint gridVAO, gridVBO; // 100x100 grid with 10 divisions


int main() {
    GLFWwindow* window = StartGLU();
    shaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);

    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    glUseProgram(shaderProgram);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    mouseCoupled = false;

    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    //projection matrix
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 750000.0f);
    projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    cameraPos = glm::vec3(0.0f, 1000.0f,  15000.0f);
    initialCameraPos = cameraPos;


    objs = {
        // Original Earth and Moon - separated much further
        Object(glm::vec3(-10000, 500, 0), glm::vec3(0, 0, 200), 5.97219*pow(10, 24), 5515),
        Object(glm::vec3(10000, 500, 0), glm::vec3(0, 0, -200), 7.34767309*pow(10, 22), 3344),

        // Test objects with various sizes - positioned far apart
        // Red object - 2x Earth mass
        Object(glm::vec3(0, 500, 15000), glm::vec3(-150, 0, 0), 1.2*pow(10, 24), 5515),
        // Blue object - 4x Earth mass
        Object(glm::vec3(-8000, 500, -8000), glm::vec3(100, 0, 100), 2.4*pow(10, 24), 5515),
        // Green object - 1x Earth mass
        Object(glm::vec3(8000, 500, -8000), glm::vec3(-100, 0, -100), 5.97219*pow(10, 23), 5515),
        // Yellow object - 3x Earth mass
        Object(glm::vec3(0, 500, -15000), glm::vec3(150, 0, 0), 1.8*pow(10, 24), 5515),
        // Purple object - 5x Earth mass
        Object(glm::vec3(15000, 500, 5000), glm::vec3(-120, 160, 0), 2.98*pow(10, 24), 5515),
        // Cyan object - 6x Earth mass
        Object(glm::vec3(-15000, 500, -5000), glm::vec3(140, -140, 0), 3.58*pow(10, 24), 5515),
    };

    // Set custom colors for test objects
    objs[0].color = glm::vec4(0.0f, 0.5f, 1.0f, 1.0f);   // Earth - Blue
    objs[1].color = glm::vec4(1.0f, 0.8f, 0.0f, 1.0f);   // Moon - Yellow
    objs[2].color = glm::vec4(1.0f, 0.2f, 0.2f, 1.0f);   // Red
    objs[3].color = glm::vec4(0.2f, 0.2f, 1.0f, 1.0f);   // Blue
    objs[4].color = glm::vec4(0.2f, 1.0f, 0.2f, 1.0f);   // Green
    objs[5].color = glm::vec4(1.0f, 1.0f, 0.2f, 1.0f);   // Yellow
    objs[6].color = glm::vec4(1.0f, 0.2f, 1.0f, 1.0f);   // Purple
    objs[7].color = glm::vec4(0.2f, 1.0f, 1.0f, 1.0f);   // Cyan

    std::vector<float> gridVertices = CreateGridVertices(100000.0f, 50, objs);
    CreateVBOVAO(gridVAO, gridVBO, gridVertices.data(), gridVertices.size());
    std::cout<<"Earth radius: "<<objs[0].radius<<std::endl;
    std::cout<<"Moon radius: "<<objs[1].radius<<std::endl;
    for(size_t i = 2; i < objs.size(); i++) {
        std::cout<<"Test object "<<i<<" radius: "<<objs[i].radius<<std::endl;
    }

    while (!glfwWindowShouldClose(window) && running == true) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        globalTime = currentFrame;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glfwSetKeyCallback(window, keyCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        UpdateCam(shaderProgram, cameraPos);
        if (!objs.empty() && objs.back().Initalizing) {
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
                // Increase mass by 1% per second
                objs.back().mass *= 1.0 + 1.0 * deltaTime;
                
                // Update radius based on new mass
                objs.back().radius = pow(
                    (3 * objs.back().mass / objs.back().density) / 
                    (4 * 3.14159265359f), 
                    1.0f/3.0f
                ) / 100000.0f;
                
                // Update vertex data
                objs.back().UpdateVertices();
            }
        }

        // Draw the grid
        glUseProgram(shaderProgram);
        glUniform4f(objectColorLoc, 1.0f, 1.0f, 1.0f, 0.25f); // White color with 50% transparency for the grid
        gridVertices = CreateGridVertices(10000.0f, 50, objs);
        glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
        glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), gridVertices.data(), GL_DYNAMIC_DRAW);
        DrawGrid(shaderProgram, gridVAO, gridVertices.size());

        for(auto& obj : objs) {
            obj.UpdateOrbitTrail();
        }

        DrawOrbits(shaderProgram);

        for(auto& obj : objs) {
            obj.UpdateVertices();
        }

        // Draw the triangle
        for(auto& obj : objs) {
            glUniform4f(objectColorLoc, obj.color.r, obj.color.g, obj.color.b, obj.color.a);

            for(auto& obj2 : objs){
                if(&obj2 != &obj && !obj.Initalizing && !obj2.Initalizing){
                    float dx = obj2.GetPos()[0] - obj.GetPos()[0];
                    float dy = obj2.GetPos()[1] - obj.GetPos()[1];
                    float dz = obj2.GetPos()[2] - obj.GetPos()[2];
                    float distance = sqrt(dx * dx + dy * dy + dz * dz);

                    if (distance > 0) {
                        std::vector<float> direction = {dx / distance, dy / distance, dz / distance};
                        distance *= 1000;
                        double Gforce = (G_BASE * gravityMultiplier * obj.mass * obj2.mass) / (distance * distance);

                        // Prevent planets from sticking together by reducing gravity at close range
                        float minDistance = (obj.radius + obj2.radius) * 3.0f;
                        if (distance / 1000.0f < minDistance) {
                            Gforce *= 0.05; // Reduce gravity significantly when too close, allows them to orbit
                        }

                        float acc1 = Gforce / obj.mass;
                        std::vector<float> acc = {direction[0] * acc1, direction[1]*acc1, direction[2]*acc1};
                        if(!pause){
                            obj.accelerate(acc[0], acc[1], acc[2]);
                        }

                        //collision
                        obj.velocity *= obj.CheckCollision(obj2);
                    }
                }
            }
            if(obj.Initalizing){
                obj.radius = pow(((3 * obj.mass/obj.density)/(4 * 3.14159265359)), (1.0f/3.0f)) / 100000;
                obj.UpdateVertices();
            }

            //update positions
            if(!pause){
                obj.UpdatePos();
            }
            
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position); // Apply position here
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glBindVertexArray(obj.VAO);
            glDrawArrays(GL_TRIANGLES, 0, obj.vertexCount / 3);
        }

        DrawGravityDisplay();
        DrawSelectionBox();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    for (auto& obj : objs) {
        glDeleteVertexArrays(1, &obj.VAO);
        glDeleteBuffers(1, &obj.VBO);
    }

    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);

    glDeleteProgram(shaderProgram);
    glfwTerminate();

    glfwTerminate();
    return 0;
}

GLFWwindow* StartGLU() {
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW, panic" << std::endl;
        return nullptr;
    }
    GLFWwindow* window = glfwCreateWindow(800, 600, "3D_TEST", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window." << std::endl;
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW." << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 800, 600);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Standard blending for transparency

    return window;
}

GLuint CreateShaderProgram(const char* vertexSource, const char* fragmentSource) {
    // Vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);

    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
    }

    // Fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
    }

    // Shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}
void CreateVBOVAO(GLuint& VAO, GLuint& VBO, const float* vertices, size_t vertexCount) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(float), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void UpdateCam(GLuint shaderProgram, glm::vec3 cameraPos) {
    glUseProgram(shaderProgram);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    float cameraSpeed = 500.0f * deltaTime;
    bool shiftPressed = (mods & GLFW_MOD_SHIFT) != 0;
    Object& lastObj = objs[objs.size() - 1];
    

    if (glfwGetKey(window, GLFW_KEY_W)==GLFW_PRESS){
        cameraPos += cameraSpeed * cameraFront;
    }
    if (glfwGetKey(window, GLFW_KEY_S)==GLFW_PRESS){
        cameraPos -= cameraSpeed * cameraFront;
    }

    if (glfwGetKey(window, GLFW_KEY_A)==GLFW_PRESS){
        cameraPos -= cameraSpeed * glm::normalize(glm::cross(cameraFront, cameraUp));
    }
    if (glfwGetKey(window, GLFW_KEY_D)==GLFW_PRESS){
        cameraPos += cameraSpeed * glm::normalize(glm::cross(cameraFront, cameraUp));
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){
        cameraPos += cameraSpeed * cameraUp;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
        cameraPos -= cameraSpeed * cameraUp;
    }

    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS){
        pause = true;
    }
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_RELEASE){
        pause = false;
    }
    
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS){
        glfwTerminate();
        glfwWindowShouldClose(window);
        running = false;
    }

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        mouseCoupled = !mouseCoupled;
        if (mouseCoupled) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS) {
        float centerX = windowWidth / 2.0f;
        float centerY = windowHeight / 2.0f;
        float offsetX = (currentMouseX - centerX) / centerX;
        float offsetY = (currentMouseY - centerY) / centerY;

        glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));
        glm::vec3 moveDir = cameraFront + right * offsetX * 0.5f + cameraUp * offsetY * 0.5f;
        moveDir = glm::normalize(moveDir);

        cameraPos += moveDir * 1500.0f;
        cameraPos.y = focusY;
    }

    if (key == GLFW_KEY_MINUS && action == GLFW_PRESS) {
        float centerX = windowWidth / 2.0f;
        float centerY = windowHeight / 2.0f;
        float offsetX = (currentMouseX - centerX) / centerX;
        float offsetY = (currentMouseY - centerY) / centerY;

        glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));
        glm::vec3 moveDir = cameraFront + right * offsetX * 0.5f + cameraUp * offsetY * 0.5f;
        moveDir = glm::normalize(moveDir);

        cameraPos -= moveDir * 1500.0f;
        cameraPos.y = focusY;
    }

    if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
        gravityMultiplier *= 0.8f;
        if (gravityMultiplier < 0.01f) gravityMultiplier = 0.01f;
        std::cout << "Gravidade reduzida: " << gravityMultiplier << std::endl;
    }

    if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
        gravityMultiplier *= 1.25f;
        if (gravityMultiplier > 100.0f) gravityMultiplier = 100.0f;
        std::cout << "Gravidade aumentada: " << gravityMultiplier << std::endl;
    }

    if (key == GLFW_KEY_5 && action == GLFW_PRESS) {
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 750000.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 viewProj = projection * view;

        float boxSize = windowWidth * 0.15f;
        float halfBox = boxSize / 2.0f;
        int selectedObjIndex = -1;

        for (size_t i = 0; i < objs.size(); i++) {
            glm::vec4 objScreenPos = viewProj * glm::vec4(objs[i].position, 1.0f);
            objScreenPos /= objScreenPos.w;

            float screenX = (objScreenPos.x + 1.0f) * 0.5f * windowWidth;
            float screenY = (1.0f - objScreenPos.y) * 0.5f * windowHeight;

            if (screenX > currentMouseX - halfBox && screenX < currentMouseX + halfBox &&
                screenY > currentMouseY - halfBox && screenY < currentMouseY + halfBox &&
                objScreenPos.z > 0) {
                selectedObjIndex = i;
                break;
            }
        }

        if (selectedObjIndex >= 0) {
            Object& selectedObj = objs[selectedObjIndex];
            float radiusRatio = 0.75f;
            float newMass = selectedObj.mass * (radiusRatio * radiusRatio * radiusRatio);

            glm::vec3 newPos = selectedObj.position;
            if (!selectedObj.orbitTrail.empty()) {
                newPos = selectedObj.orbitTrail.front();
            }

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> speedVariation(0.8f, 1.2f);
            std::uniform_real_distribution<> directionVariation(-1.0f, 1.0f);

            float speedMultiplier = speedVariation(gen);
            glm::vec3 velocityVariation = selectedObj.velocity * speedMultiplier;

            glm::vec3 randomOffset = glm::vec3(
                directionVariation(gen) * glm::length(selectedObj.velocity),
                directionVariation(gen) * glm::length(selectedObj.velocity),
                directionVariation(gen) * glm::length(selectedObj.velocity)
            );

            glm::vec3 finalVelocity = velocityVariation + randomOffset;

            objs.emplace_back(newPos, finalVelocity, newMass, selectedObj.density);
            objs.back().color = selectedObj.color;
            objs.back().color.w = 1.0f;

            std::cout << "Nova planeta criada! Tamanho: " << (radiusRatio * 100.0f) << "% do original" << std::endl;
        } else {
            std::cout << "Nenhuma planeta dentro da caixa de selecao!" << std::endl;
        }
    }

    if (key == GLFW_KEY_4 && action == GLFW_PRESS) {
        float separationThreshold = 10000.0f;
        float pushDistance = 3000.0f;
        int count = 0;

        std::cout << "Botao 4 pressionado! Verificando planetas..." << std::endl;

        for (size_t i = 0; i < objs.size(); i++) {
            for (size_t j = i + 1; j < objs.size(); j++) {
                glm::vec3 diff = objs[j].position - objs[i].position;
                float distance = glm::length(diff);

                std::cout << "Distancia entre planeta " << i << " e " << j << ": " << distance << std::endl;

                if (distance < separationThreshold && distance > 0.1f) {
                    glm::vec3 direction = glm::normalize(diff);

                    objs[i].velocity -= direction * pushDistance * 0.01f;
                    objs[j].velocity += direction * pushDistance * 0.01f;
                    count++;
                    std::cout << "Despegando par " << i << " e " << j << std::endl;
                }
            }
        }
        std::cout << "Total de pares despegados: " << count << std::endl;
    }

    // init arrows pos up down left right
    if(!objs.empty() && objs[objs.size() - 1].Initalizing){
        if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT)){
            if (!shiftPressed) {
                objs[objs.size()-1].position[1] += 0.5;
            }
        };
        if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
            if (!shiftPressed) {
                objs[objs.size()-1].position[1] -= 0.5;
            }
        }
        if(key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT)){
            objs[objs.size()-1].position[0] += 0.5;
        };
        if(key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT)){
            objs[objs.size()-1].position[0] -= 0.5;
        };
        if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
            objs[objs.size()-1].position[2] += 0.5;
        };

        if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
            objs[objs.size()-1].position[2] -= 0.5;
        }
    };
    
};
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    currentMouseX = xpos;
    currentMouseY = ypos;

    if (isDragging) {
        float dragDelta = (ypos - lastDragY) * dragSensitivity;
        focusY += dragDelta;
        lastDragY = ypos;
    }

    if (!mouseCoupled) return;

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.003f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if(pitch > 89.0f) pitch = 89.0f;
    if(pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods){
    if (button == GLFW_MOUSE_BUTTON_LEFT){
        if (action == GLFW_PRESS){
            if (!mouseCoupled) {
                isDragging = true;
                lastDragY = currentMouseY;
            } else {
                objs.emplace_back(glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0f, 0.0f, 0.0f), initMass);
                objs[objs.size()-1].Initalizing = true;
            }
        };
        if (action == GLFW_RELEASE){
            if (isDragging) {
                isDragging = false;
            } else if (!objs.empty()) {
                objs[objs.size()-1].Initalizing = false;
                objs[objs.size()-1].Launched = true;
            }
        };
    };
};
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
    float cameraSpeed = 50000.0f * deltaTime;
    if(yoffset>0){
        cameraPos += cameraSpeed * cameraFront;
    } else if(yoffset<0){
        cameraPos -= cameraSpeed * cameraFront;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 750000.0f);
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}

glm::vec3 sphericalToCartesian(float r, float theta, float phi){
    float x = r * sin(theta) * cos(phi);
    float y = r * cos(theta);
    float z = r * sin(theta) * sin(phi);
    return glm::vec3(x, y, z);
};
void DrawGrid(GLuint shaderProgram, GLuint gridVAO, size_t vertexCount) {
    glUseProgram(shaderProgram);
    glm::mat4 model = glm::mat4(1.0f); // Identity matrix for the grid
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(gridVAO);
    glPointSize(5.0f);
    glDrawArrays(GL_LINES, 0, vertexCount / 3);
    glBindVertexArray(0);
}

void DrawSelectionBox() {
    glUseProgram(0);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, (float)windowWidth, (float)windowHeight, 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    float boxSize = windowWidth * 0.15f;
    float halfBox = boxSize / 2.0f;

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(currentMouseX - halfBox, currentMouseY - halfBox);
    glVertex2f(currentMouseX + halfBox, currentMouseY - halfBox);
    glVertex2f(currentMouseX + halfBox, currentMouseY + halfBox);
    glVertex2f(currentMouseX - halfBox, currentMouseY + halfBox);
    glEnd();
    glLineWidth(1.0f);

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glUseProgram(shaderProgram);
}

void DrawGravityDisplay() {
    // Gravity value is displayed in console with each button press
}

void DrawOrbits(GLuint shaderProgram) {
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushMatrix();

    int tubeSegments = 8;

    for (const auto& obj : objs) {
        if (obj.orbitTrail.size() > 1) {
            float tubeRadius = obj.radius * 0.4f;
            glColor3f(obj.color.r, obj.color.g, obj.color.b);

            for (size_t i = 0; i < obj.orbitTrail.size() - 1; i++) {
                glm::vec3 p1 = obj.orbitTrail[i];
                glm::vec3 p2 = obj.orbitTrail[i + 1];
                glm::vec3 direction = glm::normalize(p2 - p1);

                glm::vec3 perpendicular = glm::vec3(0, 1, 0);
                if (glm::abs(glm::dot(direction, perpendicular)) > 0.9f) {
                    perpendicular = glm::vec3(1, 0, 0);
                }

                glm::vec3 right = glm::normalize(glm::cross(direction, perpendicular));
                glm::vec3 up = glm::normalize(glm::cross(right, direction));

                glBegin(GL_QUAD_STRIP);
                for (int j = 0; j <= tubeSegments; j++) {
                    float angle = 2.0f * 3.14159265f * j / tubeSegments;
                    float cosA = cos(angle);
                    float sinA = sin(angle);

                    glm::vec3 offset1 = (cosA * right + sinA * up) * tubeRadius;
                    glm::vec3 offset2 = (cosA * right + sinA * up) * tubeRadius;

                    float brightness1 = (p1.z + 10000.0f) / 20000.0f;
                    float brightness2 = (p2.z + 10000.0f) / 20000.0f;
                    brightness1 = glm::clamp(brightness1, 0.1f, 1.5f);
                    brightness2 = glm::clamp(brightness2, 0.1f, 1.5f);

                    glColor3f(obj.color.r * brightness1, obj.color.g * brightness1, obj.color.b * brightness1);
                    glVertex3f(p1.x + offset1.x, p1.y + offset1.y, p1.z + offset1.z);
                    glColor3f(obj.color.r * brightness2, obj.color.g * brightness2, obj.color.b * brightness2);
                    glVertex3f(p2.x + offset2.x, p2.y + offset2.y, p2.z + offset2.z);
                }
                glEnd();
            }
        }
    }

    glPopMatrix();
    glPopAttrib();
}

std::vector<float> CreateGridVertices(float size, int divisions, const std::vector<Object>& objs) {
    std::vector<float> vertices;
    float step = size / divisions;
    float halfSize = size / 2.0f;

    // x axis
    for (int yStep = 3; yStep <= 3; ++yStep) {
        float y = -halfSize*0.3f + yStep * step;
        for (int zStep = 0; zStep <= divisions; ++zStep) {
            float z = -halfSize + zStep * step;
            for (int xStep = 0; xStep < divisions; ++xStep) {
                float xStart = -halfSize + xStep * step;
                float xEnd = xStart + step;
                vertices.push_back(xStart); vertices.push_back(y); vertices.push_back(z);
                vertices.push_back(xEnd);   vertices.push_back(y); vertices.push_back(z);
            }
        }
    }

    // // yzxis
    // for (int xStep = 0; xStep <= divisions; ++xStep) {
    //     float x = -halfSize + xStep * step;
    //     for (int zStep = 0; zStep <= divisions; ++zStep) {
    //         float z = -halfSize + zStep * step;s
    //         for (int yStep = 0; yStep < divisions; ++yStep) {
    //             float yStart = -halfSize + yStep * step;
    //             float yEnd = yStart + step;
    //             vertices.push_back(x); vertices.push_back(yStart); vertices.push_back(z);
    //             vertices.push_back(x); vertices.push_back(yEnd);   vertices.push_back(z);
    //         }
    //     }
    // }

    // zaxis
    for (int xStep = 0; xStep <= divisions; ++xStep) {
        float x = -halfSize + xStep * step;
        for (int yStep = 3; yStep <= 3; ++yStep) {
            float y = -halfSize*0.3f + yStep * step;
            for (int zStep = 0; zStep < divisions; ++zStep) {
                float zStart = -halfSize + zStep * step;
                float zEnd = zStart + step;
                vertices.push_back(x); vertices.push_back(y); vertices.push_back(zStart);
                vertices.push_back(x); vertices.push_back(y); vertices.push_back(zEnd);
            }
        }
    }
    

    // displacement
    // for (int i = 0; i < vertices.size(); i += 3) {
    //     glm::vec3 vertexPos(vertices[i], vertices[i+1], vertices[i+2]);
    //     glm::vec3 totalDisplacement(0.0f);

    //     for (const auto& obj : objs) {
    //         glm::vec3 toObject = obj.GetPos() - vertexPos;
    //         float distance = glm::length(toObject);

    //         float distance_m = distance * 1000.0f;
            
    //         float strength = (G * obj.mass) / (distance_m * distance_m);
    //         glm::vec3 displacement = glm::normalize(toObject) * strength;

    //         totalDisplacement += -displacement * (2/distance);
    //     }

    //     vertexPos += totalDisplacement; 

    //     // Update vertex data
    //     vertices[i]   = vertexPos[0];
    //     vertices[i+1] = vertexPos[1];
    //     vertices[i+2] = vertexPos[2];
    // }
    float minz = 0.0f;
    for (int i = 0; i < vertices.size(); i += 3) {
        glm::vec3 vertexPos(vertices[i], vertices[i+1], vertices[i+2]);
        glm::vec3 totalDisplacement(0.0f);
        

        for (const auto& obj : objs) {
            glm::vec3 toObject = obj.GetPos() - vertexPos;
            float distance = glm::length(toObject);

            float distance_m = distance * 1000.0f;
            float rs = (2*G_BASE*obj.mass)/(c*c);

            float z = 2 * sqrt(rs*(distance_m - rs)) * 100.0f;
            totalDisplacement += z;
            

        }
        
        vertexPos += totalDisplacement; 

         vertices[i+1] = vertexPos[1] / 15.0f - 3000.0f;
    }
    

    return vertices;
}














