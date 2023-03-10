#pragma once

#include <glm\glm.hpp>
#include <Mesh\Mesh.hpp>
#include <vector>
#include <mutex>

class Cluster
{
	glm::vec3 mColor;
	glm::vec3 mCentre;

	std::mutex mMeshMutex;
	std::vector<glm::vec3> mPoints;
	tgl::Mesh mMesh;

	void _swap(Cluster& _Other) noexcept;
	static float distance(const glm::vec3& p1, const glm::vec3& p2) noexcept;
public:
	Cluster(glm::vec3 _Center) noexcept;
	~Cluster();

	Cluster(Cluster&& _Other) noexcept;
	Cluster(const Cluster& _Other) = delete;

	void attach_point(const glm::vec3& _Point) noexcept;
	void fill_mesh() noexcept;
	
	void draw(tgl::Mesh::GlDrawObject _DrawMode) noexcept;
	std::optional<std::pair<float, glm::vec3>> get_candidate() const noexcept;
	glm::vec3 get_centre() noexcept;
	glm::vec3 get_centre() const noexcept;
	glm::vec3 get_color() noexcept;
	glm::vec3 get_color() const noexcept;
	void clear() noexcept;
};