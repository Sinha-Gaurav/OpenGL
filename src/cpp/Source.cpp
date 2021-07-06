#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include<Circle.h>
#include<Line.h>

int main(void)
{
    if (glfwInit() == GLFW_FALSE)
    {
        std::cerr << "ERROR: GLFW initialization failed. Exiting.";
        return 1;
    }

    GLFWwindow* window = glfwCreateWindow(700, 700, "Line Drawing", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "ERROR: GLFW failed to initialize drawing window. Exiting.";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
    {
        std::cerr << "ERROR: GLEW initialization failed. Exiting.";
        glfwTerminate();
        return 1;
    }
    std::cout << glGetString(GL_VERSION) << "\n";

    Circle line = Circle(300, 400, 100);

    line.process(700, 700);

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);
        
        line.plot();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}