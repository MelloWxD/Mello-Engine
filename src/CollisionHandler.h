#pragma once
#include<ColliderShapes.h>
#include<array>
#include<vector>

namespace GJK
{

	// Old SAT based intersection test
	//bool Intersect(OBB* colliderA, OBB* colliderB)
	//{
	//	using namespace Vm;
	//	float ra, rb;
	//	m3 R, AbsR;
	//	// Compute rotation matrix expressing b in a?s coordinate frame
	//	for (int i = 0; i < 3; i++)
	//	{
	//		for (int j = 0; j < 3; j++)
	//		{
	//			R[i][j] = dot(colliderA->u[i], colliderB->u[j]);
	//		}
	//	}
	//	// Compute translation vector t
	//	v3 t = colliderB->c - colliderA->c;
	//	// Bring translation into a?s coordinate frame
	//	t = v3(dot(t, colliderA->u[0]), dot(t, colliderA->u[2]), dot(t, colliderA->u[2]));
	//	// Compute common subexpressions. Add in an epsilon term to
	//	// counteract arithmetic errors when two edges are parallel and
	//	// their cross product is (near) null (see text for details)
	//	for (int i = 0; i < 3; i++)
	//	{
	//		for (int j = 0; j < 3; j++)
	//		{
	//			AbsR[i][j] = fabs(R[i][j]) + FLT_EPSILON;
	//		}
	//	}
	//	// Axis tests
	//	// Test axes L = A0, L = A1, L = A2
	//	for (int i = 0; i < 3; i++) {
	//		ra = colliderA->e[i];
	//		rb = colliderB->e[0] * AbsR[i][0] + colliderB->e[1] * AbsR[i][1] + colliderB->e[2] * AbsR[i][2];
	//		if (fabs(t[i]) > ra + rb) return 0;
	//	}
	//	// Test axes L = B0, L = B1, L = B2
	//	for (int i = 0; i < 3; i++) 
	//	{
	//		ra = colliderA->e[0] * AbsR[0][i] + colliderA->e[1] * AbsR[1][i] + colliderA->e[2] * AbsR[2][i];
	//		rb = colliderB->e[i];
	//		if (fabs(t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i]) > ra + rb) return 0;
	//	}
	//	// Test axis L = A0 x B0
	//	ra = colliderA->e[1] * AbsR[2][0] + colliderA->e[2] * AbsR[1][0];
	//	rb = colliderB->e[1] * AbsR[0][2] + colliderB->e[2] * AbsR[0][1];
	//	if (fabs(t[2] * R[1][0] - t[1] * R[2][0]) > ra + rb) return 0;
	//	// Test axis L = A0 x B1
	//	ra = colliderA->e[1] * AbsR[2][1] + colliderA->e[2] * AbsR[1][1];
	//	rb = colliderB->e[0] * AbsR[0][2] + colliderB->e[2] * AbsR[0][0];
	//	if (fabs(t[2] * R[1][1] - t[1] * R[2][1]) > ra + rb) return 0;
	//	// Test axis L = A0 x B2
	//	ra = colliderA->e[1] * AbsR[2][2] + colliderA->e[2] * AbsR[1][2];
	//	rb = colliderB->e[0] * AbsR[0][1] + colliderB->e[1] * AbsR[0][0];
	//	if (fabs(t[2] * R[1][2] - t[1] * R[2][2]) > ra + rb) return 0;
	//	// Test axis L = A1 x B0
	//	ra = colliderA->e[0] * AbsR[2][0] + colliderA->e[2] * AbsR[0][0];
	//	rb = colliderB->e[1] * AbsR[1][2] + colliderB->e[2] * AbsR[1][1];
	//	if (fabs(t[0] * R[2][0] - t[2] * R[0][0]) > ra + rb) return 0;
	//	// Test axis L = A1 x B1
	//	ra = colliderA->e[0] * AbsR[2][1] + colliderA->e[2] * AbsR[0][1];
	//	rb = colliderB->e[0] * AbsR[1][2] + colliderB->e[2] * AbsR[1][0];
	//	if (fabs(t[0] * R[2][1] - t[2] * R[0][1]) > ra + rb) return 0;
	//	// Test axis L = A1 x B2
	//	ra = colliderA->e[0] * AbsR[2][2] + colliderA->e[2] * AbsR[0][2];
	//	rb = colliderB->e[0] * AbsR[1][1] + colliderB->e[1] * AbsR[1][0];
	//	if (fabs(t[0] * R[2][2] - t[2] * R[0][2]) > ra + rb) return 0;
	//	// Test axis L = A2 x B0
	//	ra = colliderA->e[0] * AbsR[1][0] + colliderA->e[1] * AbsR[0][0];
	//	rb = colliderB->e[1] * AbsR[2][2] + colliderB->e[2] * AbsR[2][1];
	//	if (fabs(t[1] * R[0][0] - t[0] * R[1][0]) > ra + rb) return 0;
	//	// Test axis L = A2 x B1
	//	ra = colliderA->e[0] * AbsR[1][1] + colliderA->e[1] * AbsR[0][1];
	//	rb = colliderB->e[0] * AbsR[2][2] + colliderB->e[2] * AbsR[2][0];
	//	if (fabs(t[1] * R[0][1] - t[0] * R[1][1]) > ra + rb) return 0;
	//	// Test axis L = A2 x B2
	//	ra = colliderA->e[0] * AbsR[1][2] + colliderA->e[1] * AbsR[0][2];
	//	rb = colliderB->e[0] * AbsR[2][1] + colliderB->e[1] * AbsR[2][0];
	//	if (fabs(t[1] * R[0][2] - t[0] * R[1][2]) > ra + rb) return 0;
	//	// Since no separating axis is found, the OBBs must be intersecting
	//	return 1;
	//}


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
};

std::pair<std::vector<v4>, size_t> GetFaceNormals(
	const std::vector<v3>& polytope,
	const std::vector<size_t>& faces);

void AddIfUniqueEdge(
	std::vector<std::pair<size_t, size_t>>& edges,
	const std::vector<size_t>& faces,
	size_t a, size_t b);

CollisionInfo EPA(const GJK::Simplex& simplex, const OBB* colliderA, const OBB* colliderB, int T_IDX);