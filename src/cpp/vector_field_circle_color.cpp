#include <cassert>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define VERTEX_SHADER_FILENAME "vertex_shader_color.glsl"
#define FRAGMENT_SHADER_FILENAME "fragment_shader_color.glsl"
#define REDUCTION_FACTOR 50
#define SCALING_FACTOR 5
#define MIN_DROPOFF_RADIUS 3 * std::max(window_width, window_height) / 4
#define MAX_DROPOFF_RADIUS std::max(window_width, window_height)
#define SIMILARITY_THRESHOLD 50
#define LENGTH_SPLIT 4
#define WIDTH_SPLIT 4

std::random_device hrng;
std::mt19937 engine(hrng());
long window_width;
long window_height;

struct point
{
    int16_t red;
    int16_t green;
    int16_t blue;
};

struct basepoint
{
    uint64_t length;
    uint64_t width;
    int16_t red;
    int16_t green;
    int16_t blue;
    double dropoff;
};

bool oob_warn;

std::string file_string_transfer(std::ifstream& in)
{
    std::ostringstream sstr;
    sstr << in.rdbuf();
    return sstr.str();
}

unsigned int shader_compile(unsigned int shader_type, const std::string& source_code)
{
    unsigned int shader_id = glCreateShader(shader_type);
    const char* source_code_ptr = source_code.c_str();
    glShaderSource(shader_id, 1, &source_code_ptr, NULL);
    glCompileShader(shader_id);

    int compilation_result;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compilation_result);
    if (compilation_result == GL_FALSE)
    {
        int log_length;
        glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
        char *log_message = (char*)alloca(log_length * sizeof(char));
        glGetShaderInfoLog(shader_id, log_length, &log_length, log_message);

        std::cerr << "ERROR: Shader compilation failed." << "\n";
        std::cerr << "The shader was of type " << shader_type << ".\n";
        std::cerr << "The program might not operate correctly.\n";
        std::cerr << "The error encountered was:\n\n";

        std::cerr << log_message << "\n";

        glDeleteShader(shader_id);
        return 0;
    }

    return shader_id;
}

unsigned int shaders_link_and_generate_program(const std::string& vertex_shader,
    const std::string& fragment_shader)
{
    unsigned int program_id = glCreateProgram();
    unsigned int vertex_shader_id = shader_compile(GL_VERTEX_SHADER, vertex_shader);
    unsigned int fragment_shader_id = shader_compile(GL_FRAGMENT_SHADER, fragment_shader);

    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);
    glLinkProgram(program_id);
    glValidateProgram(program_id);

    int validation_result;
    glGetProgramiv(program_id, GL_VALIDATE_STATUS, &validation_result);
    if (validation_result == GL_FALSE)
    {
        int log_length;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);
        char *log_message = (char*) alloca(log_length * sizeof(char));
        glGetProgramInfoLog(program_id, log_length, &log_length, log_message);

        std::cerr << "ERROR: Shader validation failed." << "\n";
        std::cerr << "The program might not operate correctly.\n";
        std::cerr << "The error encountered was:\n\n";

        std::cerr << log_message << "\n";

        glDeleteProgram(program_id);
        return 0;
    }

    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);
    return program_id;
}

double compute_absdistance(uint64_t length1, uint64_t width1, uint64_t length2, uint64_t width2)
{
    double temp;
    uint64_t dlength = std::max(length1, length2) - std::min(length1, length2);
    uint64_t dwidth = std::max(width1, width2) - std::min(width1, width2);
    temp = pow(dlength, 2) + pow(dwidth, 2);
    temp = pow(temp, 0.5);
    return temp;
}


int16_t main_helper_verifybounds_int16_t(int16_t check)
{
    if (check > 0)
    {
        return check;
    }
    return 0;
}

void compute_color(std::vector<float>& point_data, std::vector<struct basepoint> basepoints)
{
    struct point temp;
    temp.red = 0;
    temp.green = 0;
    temp.blue = 0;

    uint64_t x_coordinate = point_data.at(point_data.size() - 2) + (window_width / 2);
    uint64_t y_coordinate = point_data.at(point_data.size() - 1) + (window_height / 2);
    for (uint64_t i = 0; i < basepoints.size(); i++)
    {
        if ((basepoints.at(i).length == x_coordinate) && (basepoints.at(i).width == y_coordinate))
        {
            temp.red = basepoints.at(i).red;
            temp.green = basepoints.at(i).green;
            temp.blue = basepoints.at(i).blue;
            break;
        }
        temp.red = temp.red +
            main_helper_verifybounds_int16_t((double)basepoints.at(i).red * (1.0 - ((1.0 / basepoints.at(i).dropoff) * compute_absdistance(x_coordinate, y_coordinate, basepoints.at(i).length, basepoints.at(i).width))));
        temp.green = temp.green +
            main_helper_verifybounds_int16_t((double)basepoints.at(i).green * (1.0 - ((1.0 / basepoints.at(i).dropoff) * compute_absdistance(x_coordinate, y_coordinate, basepoints.at(i).length, basepoints.at(i).width))));
        temp.blue = temp.blue +
            main_helper_verifybounds_int16_t((double)basepoints.at(i).blue * (1.0 - ((1.0 / basepoints.at(i).dropoff) * compute_absdistance(x_coordinate, y_coordinate, basepoints.at(i).length, basepoints.at(i).width))));
    }
    if (temp.red > 255)
    {
        temp.red = 255;
    }
    if (temp.green > 255)
    {
        temp.green = 255;
    }
    if (temp.blue > 255)
    {
        temp.blue = 255;
    }

    point_data.push_back(temp.red / 255.0f);
    point_data.push_back(temp.green / 255.0f);
    point_data.push_back(temp.blue / 255.0f);
}

void circle(std::vector<float>& point_data, long x_initial, long y_initial, long radius,
    std::vector<struct basepoint> basepoints)
{
    long x_centre = x_initial + 20;
    long y_centre = y_initial + 400;

    long decision = 1 - radius;
    long increment_east = 3;
    long increment_southeast = (-2 * radius) + 5;
    long x = 0;
    long y = radius;
    while (y >= x)
    {
        point_data.push_back(x + x_centre);
        point_data.push_back(y + y_centre);
        compute_color(point_data, basepoints);
        point_data.push_back(-x + x_centre);
        point_data.push_back(y + y_centre);
        compute_color(point_data, basepoints);
        point_data.push_back(x + x_centre);
        point_data.push_back(-y + y_centre);
        compute_color(point_data, basepoints);
        point_data.push_back(-x + x_centre);
        point_data.push_back(-y + y_centre);
        compute_color(point_data, basepoints);
        point_data.push_back(y + x_centre);
        point_data.push_back(x + y_centre);
        compute_color(point_data, basepoints);
        point_data.push_back(-y + x_centre);
        point_data.push_back(x + y_centre);
        compute_color(point_data, basepoints);
        point_data.push_back(y + x_centre);
        point_data.push_back(-x + y_centre);
        compute_color(point_data, basepoints);
        point_data.push_back(-y + x_centre);
        point_data.push_back(-x + y_centre);
        compute_color(point_data, basepoints);

        if (decision < 0)
        {
            decision = decision + increment_east;
            increment_east = increment_east + 2;
            increment_southeast = increment_southeast + 2;
        }
        else
        {
            decision = decision + increment_southeast;
            increment_east = increment_east + 2;
            increment_southeast = increment_southeast + 4;
            y = y - 1;
        }
        x = x + 1;
    }
}

struct basepoint basepoint_layout_helper(uint64_t length_l, uint64_t length_r, uint64_t width_u, uint64_t width_d, std::vector<struct basepoint> basepoints)
{
    struct basepoint temp;


    std::uniform_int_distribution<unsigned long long> rand_length(length_l, length_r);
    std::uniform_int_distribution<unsigned long long> rand_width(width_u, width_d);
    std::uniform_int_distribution<short> rand_red(127, 255);
    std::uniform_int_distribution<short> rand_green(127, 255);
    std::uniform_int_distribution<short> rand_blue(127, 255);
    std::uniform_real_distribution<double> rand_dropoff(MIN_DROPOFF_RADIUS, MAX_DROPOFF_RADIUS);


    temp.length = rand_length(engine);
    engine.discard(temp.length);
    temp.width = rand_width(engine);
    engine.discard(temp.width);
    temp.red = rand_red(engine);
    engine.discard(temp.red);
    temp.green = rand_green(engine);
    engine.discard(temp.green);
    temp.blue = rand_blue(engine);
    engine.discard(temp.blue);
    temp.dropoff = rand_dropoff(engine);
    engine.discard(MAX_DROPOFF_RADIUS);

    if (SIMILARITY_THRESHOLD)
    {
        for (uint64_t i = 0; i < basepoints.size(); i++)
        {
            if ((abs(temp.red - basepoints.at(i).red) < SIMILARITY_THRESHOLD) && (abs(temp.green - basepoints.at(i).green) < SIMILARITY_THRESHOLD) && (abs(temp.blue - basepoints.at(i).blue) < SIMILARITY_THRESHOLD))
            {
                temp = basepoint_layout_helper(length_l, length_r, width_u, width_d, basepoints);
            }
        }
    }


    return temp;
}

void point_plotter_function(std::vector<float>& point_data, int x_coordinate, int y_coordinate,
    std::vector<struct basepoint> basepoints)
{
    long x_initial = x_coordinate;
    long y_initial = y_coordinate;
    long x_vector = x_initial;
    long y_vector = y_initial;
    long radius = sqrt((x_vector * x_vector) + (y_vector * y_vector)) / SCALING_FACTOR;
    circle(point_data, x_initial, y_initial, radius, basepoints);
}

int main(void)
{
    if (glfwInit() == GLFW_FALSE)
    {
        std::cerr << "ERROR: GLFW initialization failed. Exiting.";
        return 1;
    }

    // Get the boundaries of the window.
    std::cout << "Enter Window Width: ";
    std::cin >> window_width;
    std::cout << "Enter Window Height: ";
    std::cin >> window_height;

    std::vector<float> point_data;

    std::vector<struct basepoint> basepoints;
    struct basepoint temp;
    temp = basepoint_layout_helper(0, window_width / LENGTH_SPLIT, 0, window_height / WIDTH_SPLIT, basepoints);
    basepoints.push_back(temp);
    temp = basepoint_layout_helper(window_width - window_width / LENGTH_SPLIT, window_width, 0,
        window_height / WIDTH_SPLIT, basepoints);
    basepoints.push_back(temp);
    temp = basepoint_layout_helper(0, window_width / LENGTH_SPLIT, window_height - window_height / WIDTH_SPLIT,
        window_height, basepoints);
    basepoints.push_back(temp);
    temp = basepoint_layout_helper(window_width - window_width / LENGTH_SPLIT, window_width,
        window_height - window_height / WIDTH_SPLIT, window_height, basepoints);
    basepoints.push_back(temp);

    auto start_time = std::chrono::system_clock::now();
    for (int i = -(window_width / 2); i < (window_width / 2); i = i + REDUCTION_FACTOR)
    {
        for (int j = -(window_height / 2); j < (window_height / 2); j = j + REDUCTION_FACTOR)
        {
            point_plotter_function(point_data, i, j, basepoints);
        }
    }

    for (int i = 0; i < point_data.size(); i = i + 5)
    {
        point_data.at(i) = point_data.at(i) / (double)(window_width / 2);
        point_data.at(i + 1) = point_data.at(i + 1) / (double)(window_height / 2);
        if ((point_data.at(i) > 1.0f) || (point_data.at(i + 1) > 1.0f) || (point_data.at(i) < (-1.0f)) || (point_data.at(i + 1) < (-1.0f)))
        {
            point_data.at(i) = -1.0f;
            point_data.at(i + 1) = -1.0f;
            oob_warn = true;
        }
    }

    auto end_time = std::chrono::system_clock::now();
    std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Render Compute finished in " << duration.count() << " milliseconds.\n";
    std::cout << "Points computed: " << point_data.size() / 5 << ". Time per point: " <<
        duration.count() / (float)(point_data.size() / 5) << " milliseconds.\n";
    std::cout << point_data.size() << "\n";

    GLFWwindow* window = glfwCreateWindow(window_width, window_height, "Vector Field - Circle Drawing", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "ERROR: GLFW failed to initialize drawing window. Exiting.";
        glfwTerminate();
        exit(1);
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
    {
        std::cerr << "ERROR: GLEW initialization failed. Exiting.";
        glfwTerminate();
        exit(1);
    }
    std::cout << glGetString(GL_VERSION) << "\n";
    if (oob_warn == true)
    {
        std::cerr << "WARNING: Some points are out of bounds of the current window.\n";
        std::cerr << "They have been moved to (0, 0). Please verify settings.\n";

    }

    unsigned int buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, point_data.size() * sizeof(float), &point_data.at(0), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (const void*)(sizeof(float) * 2));

   /* std::ifstream vertex_shader_source_file;
    std::ifstream fragment_shader_source_file;

    vertex_shader_source_file.open(VERTEX_SHADER_FILENAME, std::ios::in);
    fragment_shader_source_file.open(FRAGMENT_SHADER_FILENAME, std::ios::in);
    std::string vertex_shader_source = file_string_transfer(vertex_shader_source_file);
    std::string fragment_shader_source = file_string_transfer(fragment_shader_source_file);*/

    std::string vertex_shader_source = "#version 330 core\n\nlayout(location = 0) in vec4 position;\nlayout(location = 1) in vec4 color;\nout vec4 color_data;\nvoid main()\n{\n\tgl_Position = position;\n\tcolor_data = color;\n}";
    std::string fragment_shader_source = "#version 330 core\n\nin vec4 color_data;\nout vec4 color;\nvoid main()\n{\n\tcolor = color_data;\n}";

    unsigned int program_id = shaders_link_and_generate_program(vertex_shader_source, fragment_shader_source);
    glUseProgram(program_id);

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_POINTS, 0, point_data.size() / 5);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}