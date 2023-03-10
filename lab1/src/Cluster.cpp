#include "Cluster.hpp"

#include <algorithm>
#include <type_traits>
#include <limits>
#include <random>
#include <ranges>

#include <Utility/utility.hpp>

void Cluster::_swap(Cluster& _Other) noexcept
{
	std::swap(mColor, _Other.mColor);
	std::swap(mCentre, _Other.mCentre);
	mPoints.swap(_Other.mPoints);
	std::swap(isUpdated, _Other.isUpdated);
	mMesh = std::move(_Other.mMesh);
}

Cluster::Cluster(glm::vec3 _Center) noexcept
	:
	mCentre(_Center),
	isUpdated(false)
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

	auto size = mPoints.size();
	std::vector<unsigned int> idcs(size);
	int idx = 0;
	std::ranges::for_each(idcs, [&idx](auto& e) {e = idx++; });

	std::unique_lock<std::mutex> locker(mMeshMutex);
	mMesh.set_attribut(0, mPoints.size(), mPoints.data());
	mMesh.set_indices(idcs.size(), idcs.data());
}

void Cluster::update() noexcept
{
	glm::vec3 old_centre = mCentre;

	auto calc_st_deviation = [](float x1, float y1, float x2, float y2)
	{
		auto dx = x1 - x2, dy = y1 - y2;
		return dx * dx + dy * dy;
	};

	double prev_standard_deviation = std::numeric_limits<double>::max();
	for (auto& point : mPoints)
	{
		double standard_deviation = 0.f;
		for (auto& other_point : mPoints)
			standard_deviation += calc_st_deviation(point.x, point.y, other_point.x, other_point.y);

		if (standard_deviation < prev_standard_deviation)
		{
			mCentre = point;
			prev_standard_deviation = standard_deviation;
		}
	}

	if (old_centre != mCentre)
		isUpdated = true;
}

void Cluster::draw(tgl::Mesh::GlDrawObject _DrawMode) noexcept
{
	if (!mPoints.size())
		return;

	std::unique_lock<std::mutex> locker(mMeshMutex);
	mMesh.draw_elements(_DrawMode);
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

bool Cluster::get_update_state() noexcept
{
	return isUpdated;
}

void Cluster::zero_update_state() noexcept
{
	isUpdated = false;
}

void Cluster::clear() noexcept
{
	mPoints.clear();
}
