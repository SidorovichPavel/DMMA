#include <tinyGL.hpp>
#include <View/View.hpp>
#include <View/KeyBoard.hpp>
#include <Shader/Shader.hpp>
#include <ThreadPool.hpp>
#include <glm/glm.hpp>
#include <Camera/Camera.hpp>

#include <memory>
#include <vector>
#include <algorithm>
#include <functional>
#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <random>

#include "Cluster.hpp"
#include "Utility.hpp"

std::pair<size_t, size_t> load_entry_data()
{
	std::ifstream fin("res\\in_data.txt");
	std::pair<size_t, size_t> res;
	fin >> res.first >> res.second;
	return res;
}

//using Point = std::pair<glm::vec3, glm::vec3>;

std::vector<Cluster> make_clusters(std::vector<glm::vec3> _Points, size_t _K)
{
	std::vector<Cluster> result;
	std::map<int, bool> use_idcs;
	
	for (auto i = 0; i < _K;)
	{
		size_t idx = (std::random_device{}() + std::random_device{}() + 2456) % _Points.size();
		if (!use_idcs[idx])
		{
			use_idcs[idx] = true;
			result.emplace_back(_Points[idx]);
			++i;
		}
	}
	return result;
}

void calc_k_means(const std::vector<glm::vec3>& _Points, std::vector<Cluster>& _Clusters)
{
	for (auto& cl : _Clusters)
		cl.clear();

	auto distance2D = [](float x1, float y1, float x2, float y2)
	{
		auto dx = x1 - x2, dy = y1 - y2;
		return std::sqrtf(dx * dx + dy * dy);
	};
	auto check = [&](const glm::vec3& _P)
	{
		auto result = _Clusters.begin();
		for (auto it = _Clusters.begin(); it != _Clusters.end(); ++it)
		{
			auto cluster_centre = it->get_centre(),
				result_centre = result->get_centre();
			if (distance2D(_P.x, _P.y, cluster_centre.x, cluster_centre.y) < distance2D(_P.x, _P.y, result_centre.x, result_centre.y))
				result = it;
		}
		return result;
	};

	for (auto& p : _Points)
		check(p)->attach_point(p);
}

void processing(tgl::KeyBoard& _Keyboard, DrawMode& _DMode)
{
	auto& keys = _Keyboard;
	using tgl::KeyCode;


	if (keys[KeyCode::Return])
	{
		static auto ret_tp = std::chrono::steady_clock::now();
		auto tp = std::chrono::steady_clock::now();

		if (tp - ret_tp > std::chrono::milliseconds(500))
		{
			if (_DMode == DrawMode::One)
				_DMode = DrawMode::Next;
			ret_tp = tp;
		}
	}


	if (keys[KeyCode::Space])
	{
		static auto sp_tp = std::chrono::steady_clock::now();
		auto tp = std::chrono::steady_clock::now();

		if (tp - sp_tp > std::chrono::milliseconds(500))
		{
			switch (_DMode)
			{
			case DrawMode::Multiple:
				_DMode = DrawMode::One;
				break;
			case DrawMode::One:
				_DMode = DrawMode::Multiple;
				break;
			}
			sp_tp = tp;
		}
	}
}

void work(const std::vector<glm::vec3>& _Points, std::vector<Cluster>& _Clusters, bool &_Stoped)
{
	ThreadPool workers(50);

	for (bool isClustersNeedUpdate = true; isClustersNeedUpdate && !_Stoped;)
	{
		calc_k_means(_Points, _Clusters);

		std::vector<std::future<bool>> results;

		for (auto& cl : _Clusters)
			results.push_back(workers.enqueue([&cl]()->bool
				{
					cl.zero_update_state();
					cl.update();
					return cl.get_update_state();
				}));

		isClustersNeedUpdate = true;
		for (auto&& f : results)
			isClustersNeedUpdate = isClustersNeedUpdate && f.get();
	}

	_Stoped = true;
}

int main()
{
	auto [count, k_means] = load_entry_data();
	if (k_means > 20 || k_means < 2)
		return -1;

	tgl::Init();

	auto style_ptr = new tgl::Style(L"LabWork-1", 0, 0, 900, 680);
	(*style_ptr) << tgl::StyleModifier::OverlappedWindow;

	auto window = std::make_unique<tgl::View>(style_ptr);
	window->init_opengl();
	window->enable_opengl_context();
	decltype(auto) events = window->get_events();

	tgl::Shader::path_prefix = "res\\";
	std::unique_ptr<tgl::Shader> point_shader(new tgl::FileShader("point"));

	tgl::KeyBoard key_board;

	ta::Camera camera(
		glm::vec3(0.f, 0.f, 5.f),
		glm::vec3(0.f),
		glm::vec3(0.f, 1.f, 0.f),
		window->get_ratio(), 45.f);

	events.size.attach(&camera, &ta::Camera::update_aspect);
	events.size.attach(tgl::view_port);
	events.key_down.attach(&key_board, &tgl::KeyBoard::key_down);
	events.key_up.attach(&key_board, &tgl::KeyBoard::key_up);
	events.mouse_wheel.attach(&camera, &ta::Camera::update_Fovy);

	std::vector<glm::vec3> points;
	points.resize(count);
	for (auto& p : points)
	{
		auto& [x, y, z] = p;
		z = 0.f;
		x = rand() % 2000 - 1000;
		x /= 1000.f;
		y = rand() % 2000 - 1000;
		y /= 1000.f;
	}

	auto clusters = make_clusters(points, k_means);

	bool algorithm_stoped = false;
	std::thread algorithm(work, std::cref(points), std::ref(clusters), std::ref(algorithm_stoped));
	//std::this_thread::sleep_for(std::chrono::milliseconds(50));

	tgl::gl::glPointSize(3);
	tgl::gl::glEnable(GL_POINT_SMOOTH);

	DrawMode draw_mode = DrawMode::Multiple;
	auto cluster = clusters.begin();

	while (window->is_open())
	{
		auto [make_update, msg] = tgl::event_pool(60);

		if (!make_update)
			continue;

		processing(key_board, draw_mode);

		for (auto& e : clusters)
			e.fill_mesh();

		tgl::clear_black();
		tgl::gl::glClearColor(1.f, 1.f, 1.f, 1.f);

		auto transform = camera.get_mat4(0.001f, 100.f);
		point_shader->use();
		point_shader->uniform_matrix4f("transform", &transform);

		switch (draw_mode)
		{
		case DrawMode::Multiple:
			for (auto& c : clusters)
			{
				auto color = c.get_color();
				point_shader->uniform_vector3f("uniColor", &color);
				c.draw(tgl::Mesh::GlDrawObject::Points);
			}
			break;
		case DrawMode::One:
			auto color = cluster->get_color();
			point_shader->uniform_vector3f("uniColor", &color);
			cluster->draw(tgl::Mesh::GlDrawObject::Points);
			break;
		case DrawMode::Next:
			cluster++;
			if (cluster == clusters.end())
				cluster = clusters.begin();
			draw_mode = DrawMode::One;
		}

		window->swap_buffers();
	}
	algorithm_stoped = true;
	algorithm.join();

	return 0;
}