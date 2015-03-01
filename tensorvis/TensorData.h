#ifndef TENSORDATA_H
#define TENSORDATA_H

#include "ImageDataSpace.h"

/// Management and file IO for our custom tensor+vector data.
/// Our custom data consists of a 3x3 matrix, each accompanied by a 3D vector.
/// We use 3D index addressing of the tensor data.
class TensorData
{
public:
	TensorData();
	~TensorData();

	/// Load tensor data from disk, supports several file formats
	bool load( const char* filename, ImageDataSpace space=ImageDataSpace() );
	/// Read raw tensor+vector data of given dimensionality from disk. 
	bool load( const char* filename, int* dims );
	/// Write raw tensor+vector data of given dimensionality to disk.
	void save( const char* filename ) const;
	
	/// Create new tensor+vector data
	void allocateData( ImageDataSpace space );

	/// Get data dimensions, spacing and origin
	ImageDataSpace getImageDataSpace() const { return m_space; }

	///// Handle with care!
	//void setImageDataSpace( ImageDataSpace space ) { m_space = space; }

	///@{ Access tensor and vector data
	void getTensor( int x, int y, int z, float (&tensor3x3)[9] ) const;
	void getVector( int x, int y, int z, float (&v)[3] ) const;

	void setTensor( int x, int y, int z, double* tensor3x3 );
	void setTensor( int x, int y, int z, float* tensor3x3 );
	void setVector( int x, int y, int z, double* v3 );
	void setVector( int x, int y, int z, float* v3 );

	void setTensor( int id, double* tensor3x3 );
	void setVector( int id, double* v3 );
	///@}

	///@{ Access mask (if available)
	bool hasMask() const { return m_mask != 0; }
	float getMask( int x, int y, int z ) const;
	///@}

protected:
	/// Set/replace internal data pointer which is subsequently managed by this class.
	void setTensors( float* tensors, ImageDataSpace space );
	/// Free memory
	void freeData();

	float* dataPtr();
	float* maskPtr();

	/// Returns size of tensorfield in number of floats
	unsigned getSize() const;
	// Addressing
	size_t getTensorOfs( int x, int y, int z ) const;
	size_t getVectorOfs( int x, int y, int z ) const;

	bool load_raw_or_mxvol( const char* filename, ImageDataSpace space=ImageDataSpace() );
	bool load_nrrd        ( const char* filename );

	void save_tensorfield( const char* filename ) const;
	void save_mxvol( const char* filename ) const;

private:
	float* m_data;
	float* m_mask; // Mask valid data values via a threshold (optional)
	static const char s_magic[9];

	ImageDataSpace m_space;
};

#endif // TENSORDATA_H
