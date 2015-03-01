#ifndef COLORMAPRGB_H
#define COLORMAPRGB_H
#include <vector>

class vtkScalarsToColors;

class ColorMapRGB
{
public:
	bool read( const char* filename );
	void applyTo( vtkScalarsToColors* lt );

private:
	struct Datum
	{
		float rgb[3];
	};

	typedef std::vector<Datum> Table;

	Table m_table;
};

#endif // COLORMAPRGB_H
