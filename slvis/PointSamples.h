#ifndef POINTSAMPLES_H
#define POINTSAMPLES_H

#include <limits>

/**
	Seed points for StreamlineRenderer, rudimentary IO and render code.	
*/	
class PointSamples
{
public:
	PointSamples()
	: m_numPoints(0), m_vdata(0), m_dirty(true)
	{}
		
	~PointSamples();
		
	/// Free GL specific resources (requires context)
	void destroyGL();	
	
	bool loadPointSamples( const char* filename );
		
	void render();

	float* getDataPtr() { return m_vdata; }
	int getNumPoints() const { return m_numPoints; }

	const float* getAABBMin() const { return m_aabb.min_; }
	const float* getAABBMax() const { return m_aabb.max_; }
	
private:	
	// data specific (CPU side)
	int m_numPoints;
	float* m_vdata; ///< vertex data (3*numPoints)
	bool m_dirty; ///< data (re-)upload to VBO required?

	// bounding box
	struct AABB
	{
		float min_[3], max_[3];
		inline void include( float* v3 ) 
		{
			if( *(v3+0) < min_[0] ) min_[0] = *(v3+0); else
			if( *(v3+0) > max_[0] ) max_[0] = *(v3+0);
			if( *(v3+1) < min_[1] ) min_[1] = *(v3+1); else
			if( *(v3+1) > max_[1] ) max_[1] = *(v3+1);
			if( *(v3+2) < min_[2] ) min_[2] = *(v3+2); else
			if( *(v3+2) > max_[2] ) max_[2] = *(v3+2);
		}
		void reset() 
		{ 
			min_[0] = min_[1] = min_[2] =  std::numeric_limits<float>::max();
			max_[0] = max_[1] = max_[2] = -std::numeric_limits<float>::max();
		}
		AABB() { reset(); }
	};
	AABB m_aabb;
	void updateAABB();

	// render specific (GPU side)
	struct VertexBufferObject
	{
		unsigned vbo; // should be GLuint
		bool initialized;
		bool create();
		void destroy();
		bool bind();
		void unbind();
		bool upload( float* vertexData, int numVertices );
		VertexBufferObject(): vbo(0), initialized(false) {}
	};
	VertexBufferObject m_vbo;
};

#endif // POINTSAMPLES_H
