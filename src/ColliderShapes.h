#pragma once
#include<array>
#include<vk_types.h>
#include"glm/gtx/transform.hpp"


	using Point = v3; //Define points to just be a v3

	class Sphere {
	public:
		Sphere(v3 p, float r) {
			Position = p;
			Radius = r;
		}
		v3 Position = v3(0);
		float Radius = 0;
	};

	class AABB {
	public:
		AABB()
		{
			min = v3(0);
			max = v3(1.f);
			max_Bound = v3(1.f);
			min_Bound = v3(0);
			parent_pos = v3(0);
		}
		AABB(v3 mn, v3 mx)
		{
			min = mn;
			max = mx;
			isValid(); // call anyway because we cant trust user input
			parent_pos = v3(0);
		}

		v3 min;
		v3 max;	// Not done in min max way
		v3 max_Bound;
		v3 min_Bound;
		v3 parent_pos;

		v3 prevMinB;
		v3 prevMaxB;
		v3 prevRot = v3(0);

		bool isAutoRotated = false;
		bool compareVec3(v3 a, v3 b) {
			int c = 0;
			for (int x = 0; x < 3; ++x) {
				if (a[x] == b[x])
				{
					++c;
					if (c == 3)
						return true;
					continue;
				}
				else {
					break;
				}
			}
			return false;
		}

		void setMaxBound(v3 m) {
			max_Bound = m;
			isValid();
		}

		void setMinBound(v3 m) {
			min_Bound = m;
			isValid();
		}

		void Update(v3 pos) {
			parent_pos = pos;
			min = parent_pos + min_Bound;
			max = parent_pos + max_Bound;

			isValid();
		}



		// fix if the min > max and/or max < min.
		void Fix() {
			if (this->min.x > this->max.x) {
				float temp = this->min.x;
				this->min.x = max.x;
				this->max.x = temp;
			}
			if (this->min.y > this->max.y) {
				float temp = this->min.y;
				this->min.y = max.y;
				this->max.y = temp;
			}
			if (this->min.z > this->max.z) {
				float temp = this->min.z;
				this->min.z = max.z;
				this->max.z = temp;
			}
		}

		bool isValid() {
			if (this->min.x < this->max.x && this->min.y < this->max.y && this->min.z < this->max.z)
			{
				// If min max are invalid then swap round teh axis.
			}
			else {
				Fix();
			}
			return true;
		}

		/// <summary>
		/// Auto Generates Bounds based on the .Objs vertices
		/// </summary>
		/// <param name="verts">A list of the obj Vertices</param>
		/*void generateExtents(std::vector<v3> verts, Transform t) {
			v3 s = t.getScaleAsVec3();

			if (verts.size() > 0) {
				AABB out;

				out.min_Bound = verts[0];
				out.max_Bound = verts[1];
				for (v3 v : verts) {
					if ((v.getX()) < out.min_Bound.getX()) {
						out.min_Bound.setX(v.getX());
					}
					if ((v.getX()) > out.max_Bound.getX()) {
						out.max_Bound.setX(v.getX());
					}

					if ((v.getY()) < out.min_Bound.getY()) {
						out.min_Bound.setY(v.getY());
					}
					if ((v.getY()) > out.max_Bound.getY()) {
						out.max_Bound.setY(v.getY());
					}

					if ((v.getZ()) < out.min_Bound.getZ()) {
						out.min_Bound.setZ(v.getZ());
					}
					if ((v.getZ()) > out.max_Bound.getZ()) {
						out.max_Bound.setZ(v.getZ());
					}
				}

				if (out.min_Bound.getX() == out.max_Bound.getX()) {
					out.max_Bound.setX(-out.max_Bound.getX());
				}
				if (out.min_Bound.getZ() == out.min_Bound.getZ()) {
					out.min_Bound.setZ(-out.max_Bound.getZ());
				}
				v3 outmin = v3(out.min_Bound.getX() * s.getX(), out.min_Bound.getY() * s.getY(), out.min_Bound.getZ() * s.getZ());
				v3 outmax = v3(out.max_Bound.getX() * s.getX(), out.max_Bound.getY() * s.getY(), out.max_Bound.getZ() * s.getZ());

				setMinBound(outmin);
				setMaxBound(outmax);
			}
			updateRotation(t);
		}*/
	};





	struct OBB
	{
	public:
		OBB(v3 offs, v3 ex) {
			c = offs;
			offset = offs;
			min = c - e;
			max = c + e;
			e = ex;
			scaledExtents = ex;
			scaledMax = max;
			scaledMin = min;
			updateCorners();
		}
		OBB()
		{
			e = v3(1);
			c = v3(0); e = v3(1);
			min = c - e;
			max = c + e;
			scaledExtents = v3(1);
			scaledMax = max;
			scaledMin = min;
			updateCorners();

		}
		std::array<v3, 8> m_coords; // the 8 verts that define the bounding volume of the OBB useful for GJK.
		Point c;
		v3 u[3] = { v3(1, 0, 0), v3(0, 1, 0), v3(0, 0, 1) };
		v3 e, scaledExtents;
		v3 min, max, scaledMin, scaledMax = v3(0);
		v3 offset = v3(0); // Incase the origin of the object isn't centred.
		m4 R = m4(1);
		Transform parent_transform;
		bool fixbug = false;
		/// <summary>
		/// Update the eight corner verts of the OBB (Should be called in update)
		/// </summary>
		/// <param name="R">Rotation matrix of parent</param>
		/// <param name="MMat">Model matrix of parent</param>
		void updateCorners() {
			m_coords = {
				// Bottom four verts
				scaledMin, // c-e bottom close left corner E
				scaledMin + v3(2 * scaledExtents.x, 0, 0), // botom close right corner H
				scaledMin + v3(0, 0,  2 * scaledExtents.z), // bottom far left corner F
				scaledMin + v3(2 * scaledExtents.x, 0 , 2 * scaledExtents.z), // bottom far right corner G

				// Top four verts
				scaledMax - v3(0,0 ,2 * scaledExtents.z), // top close right corner D
				scaledMax - v3(2 * scaledExtents.x, 0, 2 * scaledExtents.z), // top close left corner A
				scaledMax - v3(2 * scaledExtents.x,0, 0), // top far left corner B
				scaledMax // top far right corner C

			}; // verts raw positions for later use
		}
		/// <summary>
		/// Get the eight bounding verts of the OBB
		/// Split into 2 teterahedrons to form a quad
		/// </summary>
		/// <param name="idx">index of which tetrahedron wanted 0 or 1</param>
		/// <returns>std::array of 4 vertices of given Tetrahedron</returns>
		std::array<v3, 4> getColliderVerts(int idx) const
		{
			switch (idx) // Get each of the tetrahedrons that occypy the space of thte OBB
			{
			case 0: // T1 case 
				return std::array<v3, 4> {m_coords[0], m_coords[1], m_coords[3], m_coords[4]}; // return tetrahedron 1

			case 1: // T2 case 
				return std::array<v3, 4> {m_coords[3], m_coords[4], m_coords[7], m_coords[6]}; // return tetrahedron 2

			case 2: // T3 case 
				return std::array<v3, 4> {m_coords[0], m_coords[5], m_coords[6], m_coords[4]}; // return tetrahedron 3

			case 3: // T4 case 
				return std::array<v3, 4> {m_coords[0], m_coords[6], m_coords[3], m_coords[2]}; // return tetrahedron 4

			case 4: // T5 case 
				return std::array<v3, 4> {m_coords[0], m_coords[4], m_coords[3], m_coords[6]}; // return tetrahedron 5

			default:
				assert(false); // Should never be here
			}
		}
		void setExtents(v3 e) {
			this->e = e;
		}

		void setOffset(v3 o) {
			this->offset = o;
		}
		void update(v3 pos) { // For camera ONLY 
			c = pos + offset;
			min = c - e;
			max = c + e;
			scaledMin = min;
			scaledMax = max;
			Transform t;
			t.position = pos;
			t.scale = v3(1);

			parent_transform = t;



			updateCorners();
		}
		void update(Transform t, m4 MMat)
		{
			c = t.position + offset;
			scaledExtents = e * t.scale;
			scaledMin = c - scaledExtents;
			scaledMax = c + scaledExtents;

			parent_transform = t;
			R = glm::rotate(t.rotation.x, v3(1, 0, 0)) * glm::rotate(t.rotation.y, v3(0, 1, 0)) * glm::rotate(t.rotation.z, v3(0, 0, 1));

			updateCorners();

			v3 prec = c - (this->parent_transform.position);
			v4 precentre = v4(prec.x, prec.y, prec.z, 1);
			auto ne = R * precentre;
			v4 newc = (R * precentre) + v4(t.position.x, t.position.y, t.position.z, 1);
			c = newc;
			for (int x = 0; x < m_coords.size(); ++x)
			{
				v4 prex = v4(m_coords[x].x, m_coords[x].y, m_coords[x].z, 1) - v4(this->parent_transform.position.x, this->parent_transform.position.y, this->parent_transform.position.z, 1);
				v3 newx = (R * prex) + v4(t.position.x, t.position.y, t.position.z, 1);
				m_coords[x] = newx;


			}


		}

		// Find the furthest point in a given direction on the Bounding Volume OBB

		v3 FindFurthestPoint(v3 direction, int idx) const
		{
			v3  maxPoint;
			float maxDistance = -FLT_MAX;
			auto verts = getColliderVerts(idx);
			for (v3 vertex : verts) {
				float distance = dot(vertex, direction);
				if (distance > maxDistance)
				{
					maxDistance = distance;
					maxPoint = vertex;
				}
			}
			return maxPoint;
		}

		
	};



	////class Plane {
	////public:

	////	Plane(Point Normal, float Distance) {
	////		N = Normal;
	////		d = Distance;
	////	}

	////	Plane(Point a, Point b, Point c) {
	////		v3 lhs = Vm::normalize(Vm::cross(b - a, c - a));
	////		N = lhs;
	////		d = Vm::dot(lhs, a);
	////	}
	////	v3 N;
	////	float d;
	////};

	////class Line {
	////public:
	////	Line(v3 s, v3 e)
	////	{
	////		start = s;
	////		end = e;
	////	}
	////	v3 start = v3(0);
	////	v3 end = v3(0);

	////	float getLength() {
	////		float x, y, z;
	////		x = (end.getX() - start.getX());
	////		y = (end.getY() - start.getY());
	////		z = (end.getZ() - start.getZ());
	////		return sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2));
	////	}
	////	float getLengthSq() {
	////		float x, y, z;
	////		x = (end.getX() - start.getX());
	////		y = (end.getY() - start.getY());
	////		z = (end.getZ() - start.getZ());
	////		return pow(x, 2) + pow(y, 2) + pow(z, 2);
	////	}
	////};

	////class Ray {
	////public:
	////	Ray(v3 o, v3 d) {
	////		pos = o;
	////		dir = d;
	////	}

	////	Point pos;
	////	v3 dir;

	////	v3 getNormal() { return _Normal; }

	////	void setNormal(v3 n) { _Normal = Vm::normalize(n); }
	////private:
	////	v3 _Normal;
	////};


	class Triangle {
	public:
		Triangle(Point a, Point b, Point c) {
			A = a;
			B = b;
			C = c;
		}
		Point A, B, C;
	};
