#pragma once
#include<ColliderShapes.h>
#include<array>
#include<vector>

class GameObject;
namespace GJK
{

	
	

	bool SameDirection(const v3& direction, const v3& ao);

	v3 Support(const OBB& Aobb, const OBB& Bobb, v3 direction, int Idx);

	struct Simplex {
	public:
		Simplex()
			: m_size(0)
		{}
		std::array<v3, 4> m_points;
		int m_size = 0;
		Simplex& operator=(std::initializer_list<v3> list)
		{
			m_size = 0;
			for (v3 point : list)
				m_points[m_size++] = point;

			return *this;
		}

		void push_front(v3 point)
		{
			m_points = { point, m_points[0], m_points[1], m_points[2] };
			m_size = std::min(m_size + 1, 4);
		}

		v3& operator[](int i) { return m_points[i]; }
		size_t size() const { return m_size; }

		auto begin() const { return m_points.begin(); }
		auto end() const { return m_points.end() - (4 - m_size); }
	};

	bool Line(Simplex& points, v3& direction);

	bool Triangle(Simplex& points, v3& direction);

	bool Tetrahedron(Simplex& points, v3& direction);
	

	bool NextSimplex(Simplex& points, v3& direction);
	

	bool GJK(const OBB& colliderA, const OBB& colliderB, Simplex& simplex, int& TetraIdx);
}

using namespace GJK;

struct CollisionInfo
{
	v3 Normal;
	float penDepth;
	bool HasCollision;
	GameObject* pGameObject;
};

std::pair<std::vector<v4>, size_t> GetFaceNormals(
	const std::vector<v3>& polytope,
	const std::vector<size_t>& faces);

void AddIfUniqueEdge(
	std::vector<std::pair<size_t, size_t>>& edges,
	const std::vector<size_t>& faces,
	size_t a, size_t b);

CollisionInfo EPA(const GJK::Simplex& simplex, const OBB* colliderA, const OBB* colliderB, int T_IDX);