#ifndef WARPFIELDS_H
#define WARPFIELDS_H

#include <fstream>
#include <string>

/// Provide out-of-core access to set of deformation fields.
class Warpfields
{
public:
	Warpfields()
	{}
		
	~Warpfields()
	{
		m_curField.free();
	}
	
	typedef float ValueType;
	
	/// Displacement vectors for a single voxel
	class VoxelDisplacements
	{
		friend class Warpfields;
	public:
		VoxelDisplacements()
		: n(0), data_x(NULL), data_y(NULL), data_z(NULL)
		{}
		
		void getDisplacement( int idx, ValueType* v )
		{
			v[0] = data_x[idx];
			v[1] = data_y[idx];
			v[2] = data_z[idx];
		}
		
		void getAverage( ValueType* avg )
		{
			avg[0] = avg[1] = avg[2] = (ValueType)0.;
			for( int i=0; i < n; ++i )
			{
				avg[0] += data_x[i];
				avg[1] += data_y[i];
				avg[2] += data_z[i];
			}
			avg[0] /= n;
			avg[1] /= n;
			avg[2] /= n;
		}		
	protected:
		void allocate( int n )
		{
			free();
			data_x = new ValueType[n];
			data_y = new ValueType[n];
			data_z = new ValueType[n];
		}
		
		void free()
		{
			delete [] data_x;  data_x = NULL;
			delete [] data_y;  data_y = NULL;
			delete [] data_z;  data_z = NULL;
		}		
		
	private:
		int n;
		ValueType* data_x;
		ValueType* data_y;
		ValueType* data_z;		
	};

	bool setFile( const char* filename, int rows, int cols );

	void reset();

	void readVoxelDisplacements( int voxelIdx );	
	VoxelDisplacements getVoxelDisplacements()
	{
		return m_curField;
	}
	
protected:
	// throws exception on failure
	void prepareFileAccess();
	void setMatrixDim( int rows, int cols );
	
private:
	std::string        m_filename;
	int                m_dim[2];
	VoxelDisplacements m_curField;
	std::ifstream      m_fs;
};

#endif // WARPFIELDS_H
