#pragma once
#include <cassert>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define VERTEX_SHADER_FILENAME "vertex_shader.glsl"
#define FRAGMENT_SHADER_FILENAME "fragment_shader.glsl"
#define MIN_DROPOFF_RADIUS 3 * std::max(window_width, window_height) / 4
#define MAX_DROPOFF_RADIUS std::max(window_width, window_height)
#define SIMILARITY_THRESHOLD 50
#define LENGTH_SPLIT 4
#define WIDTH_SPLIT 4

class Circle
{
public:
	int x_center;
	int y_center;
	int radius;
	std::vector<float> point_data;
	Circle();
	Circle(int x, int y, int radius);
	int process(int window_width, int window_height);
	void plot();
	
private:
	std::string file_string_transfer(std::ifstream& in);
	unsigned int shader_compile(unsigned int shader_type, const std::string& source_code);
	unsigned int shaders_link_and_generate_program(const std::string& vertex_shader, const std::string& fragment_shader);
	double compute_absdistance(uint64_t length1, uint64_t width1, uint64_t length2, uint64_t width2);
	int16_t main_helper_verifybounds_int16_t(int16_t check);
	void compute_color(std::vector<float>& point_data, std::vector<struct basepoint> basepoints);
	struct basepoint basepoint_layout_helper(uint64_t length_l, uint64_t length_r, uint64_t width_u, uint64_t width_d, std::vector<struct basepoint> basepoints, int window_width, int window_height);
};