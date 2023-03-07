#include <tinyGL.hpp>
#include <Shader/Shader.hpp>
#include <View/KeyBoard.hpp>
#include <Camera/Camera.hpp>
#include <Mesh/Mesh.hpp>

#include <memory>
#include <iostream>
#include <random>

int main(int argc, char* args[])
{
	size_t count{};
	std::cout << "Entry points count: "; 
	std::cin >> count;

	tgl::Init();

	auto style = new tgl::Style(L"lab2");
	(*style) << tgl::StyleModifier::OverlappedWindow;

	auto window = std::make_unique<tgl::View>(style);
	window->init_opengl();
	window->enable_opengl_context();
	decltype(auto) events = window->get_events();

	tgl::KeyBoard key_board;

	ta::Camera camera(
		glm::vec3(0.f, 0.f, 2.5f),
		glm::vec3(0.f),
		glm::vec3(0.f, 1.f, 0.f),
		window->get_ratio(), 45.f);

	events.size.attach(&camera, &ta::Camera::update_aspect);
	events.size.attach(tgl::view_port);
	events.key_down.attach(&key_board, &tgl::KeyBoard::key_down);
	events.key_up.attach(&key_board, &tgl::KeyBoard::key_up);
	events.mouse_wheel.attach(&camera, &ta::Camera::update_Fovy);

	tgl::Shader::path_prefix = "res/glsl/";
	std::unique_ptr<tgl::Shader> main_shader;
	main_shader = std::make_unique<tgl::FileShader>("point");

	std::vector<glm::vec3> points;
	points.resize(count);
	for (auto& p : points)
	{
		p = glm::vec3(std::random_device{}() % 2000 - 1000.f, std::random_device{}() % 2000 - 1000.f, 0.f);
		p /= 1000.f;
	}

	tgl::Mesh mesh;
	mesh.set_attribut(0, 3, points.size() * 3, &points[0].x);

	for (;;)
	{
		auto [msg, update] = tgl::event_pool(60);
		if (!window->is_open()) break;

		tgl::clear_black();

		auto transform = camera.get_mat4(0.001f, 10.f);
		main_shader->use();
		main_shader->uniform_matrix4f("transform", &transform);
		glm::vec3 color(0.75f, 0.45f, 0.15f);
		main_shader->uniform_vector3f("uniColor", &color);

		mesh.draw_array(count, tgl::Mesh::GlDrawObject::Points);

		window->swap_buffers();
	}


	return 0;
}