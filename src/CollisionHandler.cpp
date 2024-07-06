#include"CollisionHandler.h"

bool GJK::SameDirection(const v3& direction, const v3& ao)
{
	return glm::dot(direction, ao) > 0;
}

v3 GJK::Support(const OBB& Aobb, const OBB& Bobb, v3 direction, int Idx)
{
	return Aobb.FindFurthestPoint(direction, Idx) - Bobb.FindFurthestPoint(-direction, Idx);
}

bool GJK::Line(Simplex& points, v3& direction)
{
	v3 a = points[0];
	v3 b = points[1];

	v3 ab = b - a;
	v3 ao = -a;

	if (SameDirection(ab, ao)) {
		direction = cross(cross(ab, ao), ab);
	}
	else {
		points = { a };
		direction = ao;
	}

	return false;
}

bool GJK::Triangle(Simplex& points, v3& direction)
{
	v3 a = points[0];
	v3 b = points[1];
	v3 c = points[2];

	v3 ab = b - a;
	v3 ac = c - a;
	v3 ao = -a;

	v3 abc = cross(ab, ac);

	if (SameDirection(cross(abc, ac), ao)) {
		if (SameDirection(ac, ao)) {
			points = { a, c };
			direction = cross(cross(ac, ao), ac);
		}

		else {
			return Line(points = { a, b }, direction);
		}
	}

	else {
		if (SameDirection(cross(ab, abc), ao)) {
			return Line(points = { a, b }, direction);
		}

		else {
			if (SameDirection(abc, ao)) {
				direction = abc;
			}

			else {
				points = { a, c, b };
				direction = -abc;
			}
		}
	}

	return false;
}

bool GJK::Tetrahedron(Simplex& points, v3& direction)
{
	v3 a = points[0];
	v3 b = points[1];
	v3 c = points[2];
	v3 d = points[3];

	v3 ab = b - a;
	v3 ac = c - a;
	v3 ad = d - a;
	v3 ao = -a;

	v3 abc = cross(ab, ac);
	v3 acd = cross(ac, ad);
	v3 adb = cross(ad, ab);

	if (SameDirection(abc, ao)) {
		return Triangle(points = { a, b, c }, direction);
	}

	if (SameDirection(acd, ao)) {
		return Triangle(points = { a, c, d }, direction);
	}

	if (SameDirection(adb, ao)) {
		return Triangle(points = { a, d, b }, direction);
	}

	return true;
}

bool GJK::NextSimplex(Simplex& points, v3& direction)
{
	switch (points.size())
	{

	case 2: return Line(points, direction);
	case 3: return Triangle(points, direction);
	case 4: return Tetrahedron(points, direction);
	default: return false;
	}
	// never should be here
	assert(false);
	return false;
}

bool GJK::GJK(const OBB& colliderA, const OBB& colliderB, Simplex& simplex, int& TetraIdx)
{
	int T = 0;
	// Get initial support point in any direction 
	v3 support = Support(colliderA, colliderB, v3(1), T);

	// Simplex is an array of points, max count is 4
	Simplex points;
	points.m_size = points.size();
	points.push_front(support);

	// New direction is towards the origin
	v3 direction = -support;

	while (T < 5)
	{
		support = Support(colliderA, colliderB, direction, T);

		if (dot(support, direction) <= 0) {
			++T; // No collision
			continue;
		}

		points.push_front(support);
		if (NextSimplex(points, direction))
		{
			TetraIdx = T;
			simplex = points;
			return true;
		}

	}
	return false;

}

std::pair<std::vector<v4>, size_t> GetFaceNormals(const std::vector<v3>& polytope, const std::vector<size_t>& faces)
{
	std::vector<v4> normals;
	size_t minTriangle = 0;
	float  minDistance = FLT_MAX;

	for (size_t i = 0; i < faces.size(); i += 3)
	{
		v3 a = polytope[faces[i]];
		v3 b = polytope[faces[i + 1]];
		v3 c = polytope[faces[i + 2]];

		v3 normal = glm::normalize(glm::cross(b - a, c - a));
		float distance = glm::dot(normal, a);

		if (distance < 0) {
			normal *= -1;
			distance *= -1;
		}

		normals.emplace_back(normal, distance);

		if (distance < minDistance) {
			minTriangle = i / 3;
			minDistance = distance;
		}
	}

	return { normals, minTriangle };
}

void AddIfUniqueEdge(std::vector<std::pair<size_t, size_t>>& edges, const std::vector<size_t>& faces, size_t a, size_t b)
{
	auto reverse = std::find(                       //      0--<--3
		edges.begin(),                              //     / \ B /   A: 2-0
		edges.end(),                                //    / A \ /    B: 0-2
		std::make_pair(faces[b], faces[a])			//   1-->--2
	);

	if (reverse != edges.end()) {
		edges.erase(reverse);
	}
	else {
		edges.emplace_back(faces[a], faces[b]);
	}
}


CollisionInfo EPA(const GJK::Simplex& simplex, const OBB* colliderA, const OBB* colliderB, int T_IDX)
{
	std::vector<v3> polytope(simplex.begin(), simplex.end());



	std::vector<size_t> faces = {
		0, 1, 2,
		0, 3, 1,
		0, 2, 3,
		1, 3, 2
	};

	auto [normals, minFace] = GetFaceNormals(polytope, faces);
	v3 minNormal;
	float minDist = FLT_EPSILON;
	while (minDist == FLT_EPSILON )
	{
		minNormal = v3(normals[minFace]);
		minDist = normals[minFace].w;
		v3 supp = GJK::Support(*colliderA, *colliderB, minNormal, T_IDX);
		float sDist = glm::dot(minNormal, supp);

		if (fabs(sDist - minDist) > 0.01f) {
			std::vector<std::pair<size_t, size_t>> uniqueEdges;

			for (size_t x = 0; x < normals.size(); ++x) {
				if (GJK::SameDirection(v3(normals[x]), supp)) {
					size_t f = x * 3;

					AddIfUniqueEdge(uniqueEdges, faces, f, f + 1);
					AddIfUniqueEdge(uniqueEdges, faces, f + 1, f + 2);
					AddIfUniqueEdge(uniqueEdges, faces, f + 2, f);

					faces[f + 2] = faces.back(); faces.pop_back();
					faces[f + 1] = faces.back(); faces.pop_back();
					faces[f] = faces.back(); faces.pop_back();

					normals[x] = normals.back(); normals.pop_back();

					--x;
				}
			}
			std::vector<size_t> newFaces;
			if (newFaces.size() == 0)
			{
				break;
			}
			for (auto [edgeIndex1, edgeIndex2] : uniqueEdges)
			{
				newFaces.push_back(edgeIndex1);
				newFaces.push_back(edgeIndex2);
				newFaces.push_back(polytope.size());
			}
			polytope.push_back(supp);

			auto [newNormals, newMinFace] = GetFaceNormals(polytope, newFaces);

			float oldMinDist = FLT_MAX;

			for (size_t i = 0; i < normals.size(); ++i)
			{
				if (normals[i].w < oldMinDist)
				{
					normals[i].w = (oldMinDist);
					minFace = i;
				}
			}

			if (newNormals[newMinFace].w < oldMinDist)
			{
				minFace = newMinFace + normals.size();
			}

			faces.insert(faces.end(), newFaces.begin(), newFaces.end());
			normals.insert(normals.end(), newNormals.begin(), newNormals.end());
		}
	}

	CollisionInfo info;
	info.Normal = minNormal;
	info.penDepth = minDist + 0.001f;
	info.HasCollision = true;

	return info;
}