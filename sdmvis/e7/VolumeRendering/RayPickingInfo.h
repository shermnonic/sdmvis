#ifndef RAYPICKINGINFO_H
#define RAYPICKINGINFO_H

/// Result of ray-picking used for instance by \a VolumeRendererRaycast.
/// The picked ray may have an intersection with an image volume and 
/// additionally normal information may be present.
class RayPickingInfo
{
public:
	RayPickingInfo();

	void reset();

	void setIntersection( float x, float y, float z );
	void setNormal      ( float x, float y, float z );
	void setRayStart    ( float x, float y, float z );
	void setRayEnd      ( float x, float y, float z );

	void getIntersection( float* v );
	void getNormal      ( float* v );
	void getRayStart    ( float* v );
	void getRayEnd      ( float* v );

	bool hasIntersection() const { return m_hasIntersection; }
	bool hasNormal      () const { return m_hasNormal;       }
	bool hasRayStart    () const { return m_hasRayStart;     }
	bool hasRayEnd      () const { return m_hasRayEnd;       }

private:
	bool m_hasIntersection;
	bool m_hasNormal;
	bool m_hasRayStart;
	bool m_hasRayEnd;
	float m_intersection[3];
	float m_normal      [3];
	float m_rayStart    [3];
	float m_rayEnd      [3];
};

#endif // RAYPICKINGINFO_H
