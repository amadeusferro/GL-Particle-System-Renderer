#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define WIDTH 1280
#define HEIGHT 720
#define NUM 4000

#define SHOWQUAD 0

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
        Velocity(randomFloat(-100.0f, 100.0f), randomFloat(-100.0f, 100.0f)),
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

struct Rectangle {
    float x, y, w, h;

    Rectangle(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}

    bool contains(Circle circle) {
        return (circle.Center.x >= x - w &&
                circle.Center.x <= x + w &&
                circle.Center.y >= y - h &&
                circle.Center.y <= y + h);
    }

    bool intersects(Rectangle range) {
        return !(range.x - range.w > x + w ||
                range.x + range.w < x - w ||
                range.y - range.h > y + h ||
                range.y + range.h < y - h);
    }
};

template<typename T>
struct QuadTree {
    Rectangle boundary;
    unsigned long long capacity;
    std::vector<T> elements;
    QuadTree<T>* northWest;
    QuadTree<T>* northEast;
    QuadTree<T>* southWest;
    QuadTree<T>* southEast;
    bool divided;

    QuadTree(Rectangle boundary, unsigned long long capacity) : 
    boundary(boundary), capacity(capacity), divided(false) {}
    
    ~QuadTree() {
        clear();
    }

    void clear() {
        elements.clear();
        if (divided) {
            delete northWest;
            delete northEast;
            delete southWest;
            delete southEast;
            northWest = northEast = southWest = southEast = nullptr;
            divided = false;
        }
    }

    void subdivide() {
        Rectangle ne = Rectangle(boundary.x + boundary.w / 2, boundary.y - boundary.h / 2, boundary.w / 2, boundary.h / 2);
        northEast = new QuadTree<T>(ne, capacity);

        Rectangle nw = Rectangle(boundary.x - boundary.w / 2, boundary.y - boundary.h / 2, boundary.w / 2, boundary.h / 2);
        northWest = new QuadTree<T>(nw, capacity);

        Rectangle se = Rectangle(boundary.x + boundary.w / 2, boundary.y + boundary.h / 2, boundary.w / 2, boundary.h / 2);
        southEast = new QuadTree<T>(se, capacity);

        Rectangle sw = Rectangle(boundary.x - boundary.w / 2, boundary.y + boundary.h / 2, boundary.w / 2, boundary.h / 2);
        southWest = new QuadTree<T>(sw, capacity);
        divided = true;
    }

    bool insert(T element) {

        if(!boundary.contains(*element)) {
            return false;
        }

        if(elements.size() < capacity) {
            elements.push_back(element);
            return true;
        } else {
            if(!divided) {
                subdivide();
            }

            if(northEast->insert(element)) {
                return true;
            } else if(northWest->insert(element)) {
                return true;
            } else if(southEast->insert(element)) {
                return true;
            } else if(southWest->insert(element)) {
                return true;
            }
            return false;
        }
    }

    void query(Rectangle range, std::vector<T>& found) {
        if(!boundary.intersects(range)) {
            return;
        } else {
            for (auto& element : elements) {
                if(range.contains(*element)) {
                    found.push_back(element);
                }
            }

            if(divided) {
                northWest->query(range, found);
                northEast->query(range, found);
                southWest->query(range, found);
                southEast->query(range, found);
            }
        }
    }

    std::vector<GLfloat> getVertices() {
        std::vector<GLfloat> vertices;

        float x1 = (boundary.x - boundary.w) / (WIDTH / 2.0f) - 1.0f;
        float y1 = (boundary.y - boundary.h) / (HEIGHT / 2.0f) - 1.0f;
        float x2 = (boundary.x + boundary.w) / (WIDTH / 2.0f) - 1.0f;
        float y2 = (boundary.y - boundary.h) / (HEIGHT / 2.0f) - 1.0f;
        float x3 = (boundary.x + boundary.w) / (WIDTH / 2.0f) - 1.0f;
        float y3 = (boundary.y + boundary.h) / (HEIGHT / 2.0f) - 1.0f;
        float x4 = (boundary.x - boundary.w) / (WIDTH / 2.0f) - 1.0f;
        float y4 = (boundary.y + boundary.h) / (HEIGHT / 2.0f) - 1.0f;

        vertices.push_back(x1); vertices.push_back(y1);
        vertices.push_back(x2); vertices.push_back(y2);
        vertices.push_back(x3); vertices.push_back(y3);
        vertices.push_back(x4); vertices.push_back(y4);

        if (divided) {
            auto nwVertices = northWest->getVertices();
            vertices.insert(vertices.end(), nwVertices.begin(), nwVertices.end());

            auto neVertices = northEast->getVertices();
            vertices.insert(vertices.end(), neVertices.begin(), neVertices.end());

            auto swVertices = southWest->getVertices();
            vertices.insert(vertices.end(), swVertices.begin(), swVertices.end());

            auto seVertices = southEast->getVertices();
            vertices.insert(vertices.end(), seVertices.begin(), seVertices.end());
        }

        return vertices;
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
Rectangle boundary(WIDTH/2, HEIGHT/2, WIDTH/2, HEIGHT/2);
QuadTree<Circle*> quadTree(boundary, 15);
GLuint quadVAO, quadVBO, quadPROG;
std::vector<GLfloat> quadVertices;

const char* quadVertSrc = R"(
#version 330 core

layout(location = 0) in vec2 aPos;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
})";

const char* quadFragSrc = R"(
#version 330 core

out vec4 fragColor;

void main() {
    fragColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}
)";

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

    // Show quadtree boundary lines
    
    #if SHOWQUAD == 1

    // Quadtree shaders

    GLuint quadVertID = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(quadVertID, 1, &quadVertSrc, nullptr);
    glCompileShader(quadVertID);
    GLint quadVertSuccess;
    glGetShaderiv(quadVertID, GL_COMPILE_STATUS, &quadVertSuccess);
    if(!quadVertSuccess) {
        char infoLog[1024];
        glGetShaderInfoLog(quadVertID, 1024, nullptr, infoLog);
        std::cerr << infoLog << "\n";
        return;
    }

    GLuint quadFragID = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(quadFragID, 1, &quadFragSrc, nullptr);
    glCompileShader(quadFragID);
    GLint quadFragSuccess;
    glGetShaderiv(quadFragID, GL_COMPILE_STATUS, &quadFragSuccess);
    if(!quadFragSuccess) {
        char infoLog[1024];
        glGetShaderInfoLog(quadFragID, 1024, nullptr, infoLog);
        std::cerr << infoLog << "\n";
        return;
    }

    quadPROG = glCreateProgram();
    glAttachShader(quadPROG, quadVertID);
    glAttachShader(quadPROG, quadFragID);
    glLinkProgram(quadPROG);
    GLint quadProgSuccess;
    glGetProgramiv(quadPROG, GL_LINK_STATUS, &quadProgSuccess);
    if(!quadProgSuccess) {
        char infoLog[1024];
        glGetProgramInfoLog(quadPROG, 1024, nullptr, infoLog);
        std::cerr << infoLog << "\n";
        return;
    }

    glDeleteShader(quadVertID);
    glDeleteShader(quadFragID);

    // QuadTree Buffers

    glGenVertexArrays(1, &quadVAO);
    glBindVertexArray(quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);

        quadVertices = quadTree.getVertices();
        glBufferData(GL_ARRAY_BUFFER, quadVertices.size() * sizeof(float), quadVertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    #endif

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

    quadTree.clear();

    for (auto& circle : circles) {
        quadTree.insert(&circle);
    }

    for (auto& circle : circles) {
        std::vector<Circle*> possibleCollisions;
        Rectangle range(circle.Center.x, circle.Center.y, circle.Radius * 2, circle.Radius * 2);
        quadTree.query(range, possibleCollisions);
        for (auto& other : possibleCollisions) {
            if (&circle != other) {
                circle.colides(*other);
            }
        }
    }

    static float accumulator = 0.0f;
    static bool canRender = false;

    if(accumulator >= (1.0f / 30.0f)) {
        canRender = true;
        accumulator = 0.0f;
    } else {
        accumulator += deltaTime;
        canRender = false;
    }

    if(canRender) {
        glClear(GL_COLOR_BUFFER_BIT);
    }

    glBindVertexArray(VAO);
        for (auto& circle : circles) {
            circle.update(deltaTime);
            circle.edges();

            if(canRender) {
                glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(circle.Center, 0.0f));
                model = glm::scale(model, glm::vec3(circle.Radius, circle.Radius, 1.0f));
                glm::mat4 transform = projection * model;

                glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));
                glUniform3f(colorLoc, circle.Color.r, circle.Color.g, circle.Color.b);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }
        }
    glBindVertexArray(0);
    glUseProgram(0);

    // Rendering quadtree boundaries

    #if SHOWQUAD == 1
    static std::vector<GLfloat> lastQuadVertices;
    std::vector<GLfloat> currentQuadVertices = quadTree.getVertices();

    if (currentQuadVertices != lastQuadVertices) {
        lastQuadVertices = currentQuadVertices;
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, currentQuadVertices.size() * sizeof(float), currentQuadVertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    }

    glUseProgram(quadPROG);
    glBindVertexArray(quadVAO);
    glLineWidth(2.0f);
        glDrawArrays(GL_LINE_LOOP, 0, currentQuadVertices.size() / 2);
    glBindVertexArray(0);
    glUseProgram(0);
    #endif
}

void setTitle(GLFWwindow* pWindow, float dt) {
    static float accumulator = 0.0f;

    if(accumulator >= 1.0f) {
        accumulator = 0.0f;
        char newTitle[20];
        sprintf(newTitle, "Update: %f", (1.0f/dt));
        glfwSetWindowTitle(pWindow, newTitle);
    } else {
        accumulator += dt;
    }
}

int main() {
    srand(static_cast<unsigned int>(time(0)));

    for (int i = 0; i < NUM; i++) {
        float radius = randomFloat(1.0f, 5.0f);
        Circle circle(
            radius,
            glm::vec2(randomFloat(radius, WIDTH - radius), randomFloat(radius, HEIGHT - radius)),
            glm::vec3(randomFloat(0.2f, 1.0f), randomFloat(0.2f, 1.0f), randomFloat(0.2f, 1.0f))
        );
        circles.emplace_back(circle);
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
        setTitle(myWindow, deltaTime);
        lastFrameTime = currentTime;
        glfwPollEvents();
        render(deltaTime);
        glfwSwapBuffers(myWindow);
    }

    glDeleteProgram(PROG);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();

    return 0;
}