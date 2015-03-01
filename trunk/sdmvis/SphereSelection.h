// Max Hermann, November 10, 2010
#ifndef SPHERESELECTION_H
#define SPHERESELECTION_H

class SphereSelection
{
public:
	SphereSelection();
	SphereSelection( float x, float y, float z, float radius );

	void draw();

	void  set_center( float x, float y, float z );
	void  get_center( float& x, float &y, float& z ) const;

	void  set_radius( float radius );
	float get_radius() const;

private:
	float m_center[3], m_radius;
};

#endif // SPHERESELECTION_H
