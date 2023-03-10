#include <tinyGL.hpp>
#include <Shader/Shader.hpp>
#include <View/KeyBoard.hpp>
#include <Camera/Camera.hpp>
#include <Mesh/Mesh.hpp>
#include <ThreadPool.hpp>

#include <memory>
#include <iostream>
#include <random>

#include "Cluster.hpp"

float distance(const glm::vec3& p1, const glm::vec3& p2);
float culc_mean_cluster_dist(const std::list<Cluster>& _Clusters);

int main(int argc, char* args[])
{
	size_t count{ 1000000 };
	//std::cout << "Entry points count: ";
	//std::cin >> count;

	tgl::Init();

	auto style = new tgl::Style(L"lab2");
	(*style) << tgl::StyleModifier::OverlappedWindow;

	auto window = std::make_unique<tgl::View>(style);
	window->init_opengl();
	window->enable_opengl_context();
	decltype(auto) events = window->get_events();

	tgl::Keyboard keyboard;

	ta::Camera camera(
		glm::vec3(0.f, 0.f, 2.5f),
		glm::vec3(0.f),
		glm::vec3(0.f, 1.f, 0.f),
		window->get_ratio(), 45.f);

	events.size.attach(&camera, &ta::Camera::update_aspect);
	events.size.attach(tgl::view_port);
	events.key_down.attach(&keyboard, &tgl::Keyboard::key_down);
	events.key_up.attach(&keyboard, &tgl::Keyboard::key_up);
	events.mouse_wheel.attach(&camera, &ta::Camera::update_Fovy);

	tgl::Shader::path_prefix = "res/glsl/";
	std::unique_ptr<tgl::Shader> main_shader = std::make_unique<tgl::FileShader>("point");

	std::vector<glm::vec3> points;
	points.resize(count);
	for (auto& p : points)
	{
		p = glm::vec3(std::random_device{}() % 2000 - 1000.f, std::random_device{}() % 2000 - 1000.f, 0.f);
		p /= 1000.f;
	}

	auto fp = points.begin() + (std::random_device{}() % points.size());
	auto target = std::make_pair(std::numeric_limits<float>::min(), points.end());

	for (auto it = points.begin(); it != points.end(); it++)
	{
		if (fp == it || target.second == fp)
			continue;

		auto dist = distance(*fp, *it);
		if (dist > target.first)
			target = { dist, it };
	}

	std::list<Cluster> clusters;
	clusters.emplace_back(*fp);
	clusters.emplace_back(*target.second);

	ThreadPool thread_pool(20);

	for (;;)
	{
		for (auto& c : clusters) c.clear();

		for (auto& point : points)
		{
			auto target_cluster = std::make_pair(std::numeric_limits<float>::max(), clusters.end());
			for (auto cluster = clusters.begin(); cluster != clusters.end(); cluster++)
			{
				auto d = distance(cluster->get_centre(), point);
				if (d < target_cluster.first)
					target_cluster = { d, cluster };
			}

			if (target_cluster.second != clusters.end())
				target_cluster.second->attach_point(point);
		}

		std::list<std::pair<float, glm::vec3>> condidates;
		std::list<std::future<std::optional<std::pair<float, glm::vec3>>>> future_condidates;
		std::ranges::for_each(clusters, [&](const Cluster& cluster) {
			future_condidates.emplace_back(thread_pool.enqueue(&Cluster::get_candidate, &cluster));
			});

		auto future_mcd = thread_pool.enqueue(culc_mean_cluster_dist, std::cref(clusters));

		for (; !future_condidates.empty();)
			for (auto fcondidate = future_condidates.begin(); fcondidate != future_condidates.end();)
			{
				if (fcondidate->wait_for(std::chrono::seconds(0)) == std::future_status::ready)
				{
					auto condidate = fcondidate->get();
					if (condidate)
						condidates.push_back(*condidate);
					fcondidate = future_condidates.erase(fcondidate);
				}
				else
					fcondidate++;
			}

		auto condidate = std::ranges::max(condidates, [](const auto& a, const auto& b) {return a.first < b.first; });

		auto mean_cluster_dist = future_mcd.get();

		if (condidate.first > mean_cluster_dist)
			clusters.emplace_back(condidate.second);
		else
			break;
	}

	std::ranges::for_each(clusters, &Cluster::fill_mesh);

	for (;;)
	{
		auto [msg, update] = tgl::event_pool(60);
		if (!window->is_open()) break;

		tgl::clear_black();

		auto transform = camera.get_mat4(0.001f, 10.f);
		main_shader->use();
		main_shader->uniform_matrix4f("transform", &transform);

		for (auto& cluster : clusters)
		{
			auto color = cluster.get_color();
			main_shader->uniform_vector3f("uniColor", &color);
			cluster.draw(tgl::Mesh::GlDrawObject::Points);
		}

		window->swap_buffers();
	}

	return 0;
}

float distance(const glm::vec3& p1, const glm::vec3& p2)
{
	auto dx = p1.x - p2.x,
		dy = p1.y - p2.y;

	return sqrtf(dx * dx + dy * dy);
}

float culc_mean_cluster_dist(const std::list<Cluster>& _Clusters)
{
	float mean_cluster_dist = 0.f;
	size_t dist_count = 0;
	for (auto it1 = _Clusters.begin(); it1 != _Clusters.end(); it1++)
		for (auto it2 = it1;;)
		{
			it2++;
			if (it2 == _Clusters.end()) break;

			mean_cluster_dist += distance(it1->get_centre(), it2->get_centre());
			dist_count++;
		}
	return mean_cluster_dist * 0.5f / dist_count;
}