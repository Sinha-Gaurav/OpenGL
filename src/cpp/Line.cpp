#include "Line.h"

/// \file



// int64_t is an alias for long on my system.
struct point
{
    int16_t red;
    int16_t green;
    int16_t blue;
};

struct basepoint
{
    int64_t length;
    int64_t width;
    int16_t red;
    int16_t green;
    int16_t blue;
    double dropoff;
};



/// <summary>
/// Default constructor
/// </summary>
/// @warning This initializes x_initial, y_initial, x_final, y_final to -1.
Line::Line() {

}

/// <summary>
/// Parameterised constructor
/// </summary>
/// <param name="x_initial"> x coordinate of initial point</param>
/// <param name="y_intial"> y coordinate of initial point</param>
/// <param name="x_final"> x coordinate of final point</param>
/// <param name="y_final"> y coordinate of final point</param>
Line::Line(int x_initial, int y_intial, int x_final, int y_final) {
    this->x_initial = x_initial;
    this->y_initial = y_initial;
    this->x_final = x_final;
    this->y_final = y_final;
}


std::string Line::file_string_transfer(std::ifstream& in)
{
    std::ostringstream sstr;
    sstr << in.rdbuf();
    return sstr.str();
}

double Line::compute_absdistance(uint64_t length1, uint64_t width1, uint64_t length2, uint64_t width2)
{
    double temp;
    uint64_t dlength = std::max(length1, length2) - std::min(length1, length2);
    uint64_t dwidth = std::max(width1, width2) - std::min(width1, width2);
    temp = pow(dlength, 2) + pow(dwidth, 2);
    temp = pow(temp, 0.5);
    return temp;
}

int16_t Line::main_helper_verifybounds_int16_t(int16_t check)
{
    if (check > 0)
    {
        return check;
    }
    return 0;
}

void Line::compute_color(std::vector<float>& point_data, std::vector<struct basepoint> basepoints)
{
    struct point temp;
    temp.red = 0;
    temp.green = 0;
    temp.blue = 0;

    uint64_t x_coordinate = point_data.at(point_data.size() - 2);
    uint64_t y_coordinate = point_data.at(point_data.size() - 1);
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

unsigned int Line::shader_compile(unsigned int shader_type, const std::string& source_code)
{
    unsigned int shader_id = glCreateShader(shader_type);
    const char* source_code_ptr = source_code.c_str();     // Pointer to given string
    glShaderSource(shader_id, 1, &source_code_ptr, NULL);
    glCompileShader(shader_id);

    int compilation_result;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compilation_result);

    // Check for errors generated.
    if (compilation_result == GL_FALSE)
    {
        int log_length;
        glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
        char* log_message = (char*)alloca(log_length * sizeof(char));
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

unsigned int Line::shaders_link_and_generate_program(const std::string& vertex_shader, const std::string& fragment_shader)
{
    // Linking vertex and fragment shader and generating resulting program.
    unsigned int program_id = glCreateProgram();
    unsigned int vertex_shader_id = shader_compile(GL_VERTEX_SHADER, vertex_shader);
    unsigned int fragment_shader_id = shader_compile(GL_FRAGMENT_SHADER, fragment_shader);

    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);
    glLinkProgram(program_id);
    glValidateProgram(program_id);

    // Check for errors generated.
    int validation_result;
    glGetProgramiv(program_id, GL_VALIDATE_STATUS, &validation_result);
    if (validation_result == GL_FALSE)
    {
        int log_length;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);
        char* log_message = (char*)alloca(log_length * sizeof(char));
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


// this function generates the basepoints. A basepoint is a point whose color values are given by the 
// random number generator. Only 4 of these are actually generated, the color values of the rest are 
// determined from these points.

// further discussion is in the algorithm analysis, the basepoints are stored in a vector
struct basepoint Line::basepoint_layout_helper(uint64_t length_l, uint64_t length_r, uint64_t width_u, uint64_t width_d, std::vector<struct basepoint> basepoints, int window_width, int window_height)
{
    struct basepoint temp;


    std::uniform_int_distribution<unsigned long long> rand_length(length_l, length_r);
    std::uniform_int_distribution<unsigned long long> rand_width(width_u, width_d);
    std::uniform_int_distribution<short> rand_red(127, 255);
    std::uniform_int_distribution<short> rand_green(127, 255);
    std::uniform_int_distribution<short> rand_blue(127, 255);
    std::uniform_real_distribution<double> rand_dropoff(MIN_DROPOFF_RADIUS, MAX_DROPOFF_RADIUS);

    std::random_device hrng;
    std::mt19937 engine(hrng());


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
                temp = basepoint_layout_helper(length_l, length_r, width_u, width_d, basepoints, window_width, window_height);
            }
        }
    }


    return temp;
}

/// <summary>
/// Calculates the position of points on the line corresponding to the positions of initial and final points.
/// Pushes the points on the line onto the active buffer.
/// </summary>
/// <param name="window_width"> Width of window</param>
/// <param name="window_height"> Height of window</param>
/// <returns> 0 on successful processing\n -1 on error</returns>
/// @warning This function needs to be called before plot() to calculate the position of points.
int Line::process(int window_width, int window_height) {

    std::vector<struct basepoint> basepoints;
    struct basepoint temp;
    temp = basepoint_layout_helper(0, window_width / LENGTH_SPLIT, 0, window_height / WIDTH_SPLIT, basepoints, window_width, window_height);
    basepoints.push_back(temp);
    temp = basepoint_layout_helper(window_width - window_width / LENGTH_SPLIT, window_width, 0,
        window_height / WIDTH_SPLIT, basepoints, window_width, window_height);
    basepoints.push_back(temp);
    temp = basepoint_layout_helper(0, window_width / LENGTH_SPLIT, window_height - window_height / WIDTH_SPLIT,
        window_height, basepoints, window_width, window_height);
    basepoints.push_back(temp);
    temp = basepoint_layout_helper(window_width - window_width / LENGTH_SPLIT, window_width,
        window_height - window_height / WIDTH_SPLIT, window_height, basepoints, window_width, window_height);
    basepoints.push_back(temp);

    auto start_time = std::chrono::system_clock::now();

    int decision;
    int inc1;
    int inc2;
    int delta_x = ((x_final - x_initial) >= 0) ? (x_final - x_initial) : -(x_final - x_initial);
    int delta_y = ((y_final - y_initial) >= 0) ? (y_final - y_initial) : -(y_final - y_initial);
    int increment_x = (x_final < x_initial) ? -1 : 1;
    int increment_y = (y_final < y_initial) ? -1 : 1;
    int x = x_initial;
    int y = y_initial;
    if (delta_x > delta_y)
    {
        point_data.push_back(x);
        point_data.push_back(y);
        compute_color(point_data, basepoints);

        decision = 2 * delta_y - delta_x;
        inc1 = 2 * (delta_y - delta_x);
        inc2 = 2 * delta_y;
        for (int i = 0; i < delta_x; i++)
        {
            if (decision >= 0)
            {
                y = y + increment_y;
                decision = decision + inc1;
            }
            else
            {
                decision = decision + inc2;
            }
            x = x + increment_x;
            point_data.push_back(x);
            point_data.push_back(y);
            compute_color(point_data, basepoints);
        }
    }
    else
    {
        point_data.push_back(x);
        point_data.push_back(y);
        compute_color(point_data, basepoints);

        decision = 2 * delta_x - delta_y;
        inc1 = 2 * (delta_x - delta_y);
        inc2 = 2 * delta_x;
        for (int i = 0; i < delta_y; i++)
        {
            if (decision >= 0)
            {
                x = x + increment_x;
                decision = decision + inc1;
            }
            else
                decision = decision + inc2;
            y = y + increment_y;
            point_data.push_back(x);
            point_data.push_back(y);
            compute_color(point_data, basepoints);
        }
    }

    for (int i = 0; i < point_data.size(); i = i + 5)
    {
        point_data.at(i) = (2 * (point_data.at(i) / (double)(window_width))) - 1.0f;
        point_data.at(i + 1) = (2 * (point_data.at(i + 1) / (double)(window_height))) - 1.0f;
    }

    auto end_time = std::chrono::system_clock::now();
    std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Render Compute finished in " << duration.count() << " milliseconds.\n";
    std::cout << "Points computed: " << point_data.size() / 5 << " . Time per point: " <<
        duration.count() / (float)(point_data.size() / 5) << " milliseconds.\n";

    unsigned int buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, point_data.size() * sizeof(float), &point_data.at(0), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (const void*)(sizeof(float) * 2));

    //std::ifstream vertex_shader_source_file;
    //std::ifstream fragment_shader_source_file;

    //vertex_shader_source_file.open(VERTEX_SHADER_FILENAME, std::ios::in);
    //fragment_shader_source_file.open(FRAGMENT_SHADER_FILENAME, std::ios::in);
    //std::string vertex_shader_source = file_string_transfer(vertex_shader_source_file);       // Store the data from the vertex_shader.glsl file as string.
    //std::string fragment_shader_source = file_string_transfer(fragment_shader_source_file);   // Store the data from the fragment_shader.glsl file as string.

    std::string vertex_shader_source = "#version 330 core\n\nlayout(location = 0) in vec4 position;\nlayout(location = 1) in vec4 color;\nout vec4 color_data;\nvoid main()\n{\n\tgl_Position = position;\n\tcolor_data = color;\n}";
    std::string fragment_shader_source = "#version 330 core\n\nin vec4 color_data;\nout vec4 color;\nvoid main()\n{\n\tcolor = color_data;\n}";


    unsigned int program_id = shaders_link_and_generate_program(vertex_shader_source, fragment_shader_source);
    glUseProgram(program_id);

    return 0;
}

/// <summary>
/// Plots the line on the window
/// </summary>
/// @warning This function needs to be called after the call to process(int window_width, int window_height) 
void Line::plot() {
    glDrawArrays(GL_POINTS, 0, point_data.size() / 5);   // Draw at the points stored in the vector.
}