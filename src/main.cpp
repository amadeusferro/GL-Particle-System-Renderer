#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define WIDTH 800
#define HEIGHT 800
#define NUM 4000

float randomFloat(float min, float max) {
    float random = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    random = min + random * (max - min);
    return random;
}

std::vector<GLfloat> vertices{
    //Vertices       UV
    -1.0f,  1.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f,
    1.0f, -1.0f, 1.0f, 0.0f,
    1.0f,  1.0f, 1.0f, 1.0f
};

GLuint indices[] = {
    0, 1, 2,
    0, 2, 3,
};

struct Circle {
    glm::vec2 Center;
    glm::vec3 Color;
    std::vector<GLfloat> Vertices;
    float Radius;
    glm::vec2 Velocity;
    glm::vec2 Aceleration;
    float Mass;

    Circle(float radius, glm::vec2 center = {0.0f, 0.0f}, glm::vec3 color = {1.0f, 0.0f, 0.0f}) :
        Center(center),
        Color(color),
        Vertices(vertices),
        Radius(radius),
        Velocity(randomFloat(0.0f, 200.0f), randomFloat(0.0f, 200.0f)),
        Aceleration(glm::vec2(0.0f, 0.0f)),
        Mass(3.1415f * static_cast<float>(std::pow(radius, 2))) {}

    void update(float deltaTime) {
        Velocity = Velocity + Aceleration * deltaTime;
        Center = Center + Velocity * deltaTime;
        Aceleration = glm::vec2(0.0f, 0.0f);
    }

    void edges() {
        if (Center.x > WIDTH - Radius) {
            Center.x = WIDTH - Radius;
            Velocity.x *= -1;
        } else if (Center.x < Radius) {
            Center.x = Radius;
            Velocity.x *= -1;
        }

        if (Center.y > HEIGHT - Radius) {
            Center.y = HEIGHT - Radius;
            Velocity.y *= -1;
        } else if (Center.y < Radius) {
            Center.y = Radius;
            Velocity.y *= -1;
        }
    }

    void colides(Circle& other) {
        glm::vec2 impactVector = other.Center - Center;
        float distance = glm::length(impactVector);

        if (distance < Radius + other.Radius) {
            float overlap = (Radius + other.Radius) - distance;

            glm::vec2 dir = glm::normalize(impactVector);

            Center -= dir * overlap * 0.5f;
            other.Center += dir * overlap * 0.5f;

            glm::vec2 deltaVelocity = Velocity - other.Velocity;
            glm::vec2 deltaPosition = Center - other.Center;

            float dotProduct = glm::dot(deltaVelocity, deltaPosition);
            float distanceSquared = glm::dot(deltaPosition, deltaPosition);

            float massFactor = (2 * other.Mass) / (Mass + other.Mass);
            Velocity -= massFactor * (dotProduct / distanceSquared) * deltaPosition;

            massFactor = (2 * Mass) / (Mass + other.Mass);
            other.Velocity += massFactor * (dotProduct / distanceSquared) * deltaPosition;
        }
    }
};

const char* vertSrc = R"(
#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aTexCoord;
uniform mat4 uTransform;
out vec2 TexCoord;
void main() {
    gl_Position = uTransform * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
})";

const char* fragSrc = R"(
#version 330 core
uniform vec3 uColor;
in vec2 TexCoord;
out vec4 fragColor;

void main() {
    vec2 uv = TexCoord * 2.0 - 1.0; // Normalize to [-1, 1]
    float dist = length(uv);

    float fade = 0.05;
    float circle = smoothstep(1.0, 1.0 - fade, dist);

    fragColor = vec4(uColor, circle);
}
)";

GLuint VAO, VBO, EBO, PROG;
std::vector<Circle> circles;
glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(WIDTH), static_cast<float>(HEIGHT), 0.0f);

void init() {

    // Shaders

    GLuint vertID = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertID, 1, &vertSrc, nullptr);
    glCompileShader(vertID);
    GLint vertSuccess;
    glGetShaderiv(vertID, GL_COMPILE_STATUS, &vertSuccess);
    if(!vertSuccess) {
        char infoLog[1024];
        glGetShaderInfoLog(vertID, 1024, nullptr, infoLog);
        std::cerr << infoLog << "\n";
        return;
    }

    GLuint fragID = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragID, 1, &fragSrc, nullptr);
    glCompileShader(fragID);
    GLint fragSuccess;
    glGetShaderiv(fragID, GL_COMPILE_STATUS, &fragSuccess);
    if(!fragSuccess) {
        char infoLog[1024];
        glGetShaderInfoLog(fragID, 1024, nullptr, infoLog);
        std::cerr << infoLog << "\n";
        return;
    }

    PROG = glCreateProgram();
    glAttachShader(PROG, vertID);
    glAttachShader(PROG, fragID);
    glLinkProgram(PROG);
    GLint progSuccess;
    glGetProgramiv(PROG, GL_LINK_STATUS, &progSuccess);
    if(!progSuccess) {
        char infoLog[1024];
        glGetProgramInfoLog(PROG, 1024, nullptr, infoLog);
        std::cerr << infoLog << "\n";
        return;
    }

    glDeleteShader(vertID);
    glDeleteShader(fragID);

    // Buffers

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void render(float deltaTime) {
    glUseProgram(PROG);

    GLint transformLoc = glGetUniformLocation(PROG, "uTransform");
    GLint colorLoc = glGetUniformLocation(PROG, "uColor");

    if (transformLoc == -1 || colorLoc == -1) {
        std::cerr << "Error geting uniforms location\n";
        return;
    }

    for (size_t i = 0; i < circles.size(); ++i) {
        for (size_t j = i + 1; j < circles.size(); ++j) {
            circles[i].colides(circles[j]);
        }
    }

    glBindVertexArray(VAO);
        for (auto& circle : circles) {
            circle.update(deltaTime);
            circle.edges();

            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(circle.Center, 0.0f));
            model = glm::scale(model, glm::vec3(circle.Radius, circle.Radius, 1.0f));
            glm::mat4 transform = projection * model;

            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));
            glUniform3f(colorLoc, circle.Color.r, circle.Color.g, circle.Color.b);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
    glBindVertexArray(0);
}

int main() {
    srand(static_cast<unsigned int>(time(0)));

    for (int i = 0; i < NUM; i++) {
        float radius = randomFloat(1.0f, 5.0f);
        circles.emplace_back(
            radius,
            glm::vec2(randomFloat(radius, WIDTH - radius), randomFloat(radius, HEIGHT - radius)),
            glm::vec3(randomFloat(0.2f, 1.0f), randomFloat(0.2f, 1.0f), randomFloat(0.2f, 1.0f))
        );
    }

    if(!glfwInit()) {
        std::cerr << "Error initing glfw\n";
        return -1;
    }

    GLFWwindow* myWindow = glfwCreateWindow(WIDTH, HEIGHT, "myWindow", NULL, NULL);

    if(myWindow == nullptr) {
        std::cerr << "Error creating window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(myWindow);
    
    if(glewInit() != GLEW_OK) {
        std::cerr << "Error initing glew\n";
        glfwDestroyWindow(myWindow);
        glfwTerminate();
        return -1;
    }

    init();

    float deltaTime = 0.0f;
    float lastFrameTime = glfwGetTime();

    while (!glfwWindowShouldClose(myWindow)) {
        float currentTime = glfwGetTime();
        deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT);
        render(deltaTime);
        glfwSwapBuffers(myWindow);
    }

    glDeleteProgram(PROG);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();

    return 0;
}