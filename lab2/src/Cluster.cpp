#include "Cluster.hpp"

#include <algorithm>
#include <type_traits>
#include <limits>
#include <random>
#include <ranges>
#include <optional>

#include <Utility/utility.hpp>

void Cluster::_swap(Cluster& _Other) noexcept
{
	std::swap(mColor, _Other.mColor);
	std::swap(mCentre, _Other.mCentre);
	mPoints.swap(_Other.mPoints);
	mMesh = std::move(_Other.mMesh);
}

float Cluster::distance(const glm::vec3& p1, const glm::vec3& p2) noexcept
{
	auto dx = p1.x - p2.x,
		dy = p1.y - p2.y;

	return sqrtf(dx * dx + dy * dy);
}

Cluster::Cluster(glm::vec3 _Center) noexcept
	:
	mCentre(_Center)
{
	float r, g, b;
	int rgb = (std::random_device{}() >> 4) + (std::random_device{}() << 4);

	r = tgl::detail::cutter<int, unsigned char, 0>::get(rgb) / 255.f;
	g = tgl::detail::cutter<int, unsigned char, 1>::get(rgb) / 255.f;
	b = tgl::detail::cutter<int, unsigned char, 2>::get(rgb) / 255.f;
	mColor = glm::vec3(r, g, b);
}

Cluster::Cluster(Cluster&& _Other) noexcept
{
	_swap(_Other);
}

Cluster::~Cluster()
{
}

void Cluster::attach_point(const glm::vec3& _Point) noexcept
{
	mPoints.push_back(_Point);
}

void Cluster::fill_mesh() noexcept
{
	if (!mPoints.size())
		return;

	std::unique_lock<std::mutex> locker(mMeshMutex);
	mMesh.set_attribut(0, mPoints.size(), mPoints.data());
}

void Cluster::draw(tgl::Mesh::GlDrawObject _DrawMode) noexcept
{
	if (!mPoints.size())
		return;

	std::unique_lock<std::mutex> locker(mMeshMutex);
	mMesh.draw_array(mPoints.size(), _DrawMode);
}

std::optional<std::pair<float, glm::vec3>> Cluster::get_candidate() const noexcept
{
	std::optional<std::pair<float, glm::vec3>> result;

	float max_dist = std::numeric_limits<float>::min();
	for (auto& p : mPoints)
	{
		auto d = distance(mCentre, p);
		if (d > max_dist)
		{
			result = { d, p };
			max_dist = d;
		}
	}

	return result;
}

glm::vec3 Cluster::get_centre() noexcept
{
	return mCentre;
}

glm::vec3 Cluster::get_centre() const noexcept
{
	return mCentre;
}

glm::vec3 Cluster::get_color() noexcept
{
	return mColor;
}

glm::vec3 Cluster::get_color() const noexcept
{
	return mColor;
}

void Cluster::clear() noexcept
{
	mPoints.clear();
}
