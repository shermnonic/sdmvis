#ifndef TENSORVIS_H
#define TENSORVIS_H

#include "TensorData.h"
#include "TensorVisBase.h"

#include <vtkPoints.h>
#include <vtkDoubleArray.h>
#include <vtkImageData.h>
#ifndef VTKPTR
#include <vtkSmartPointer.h>
#define VTKPTR vtkSmartPointer
#endif

// Forwards
class vtkRenderer;

/// VTK based tensor image visualization.
/// Add this to a vtkRenderer via \a setRenderer() function.
/// \deprecated Prefer to use \a TensorVis2 based on \a TensorDataProvider.
class TensorVis : public TensorVisBase
{
public:
	TensorVis();

	/// Load tensor data for reference volume and setup tensor visualization
	bool setup( const char* filename, vtkImageData* refvol );

	/// Create tensor glyph visualization by sampling tensor data
	/// \arg refvol Reference volume is required for physical coordinates
	void updateGlyphs( int/*SamplingStrategy*/ strategy );

protected:
	/// Create glyph data at randomly sampled positions
	/// (Implicitly called by createGlyphs(), do not call directly!)
	/// Returns number of inserted glyphs.
	int randomSampling();

	/// Create glyph data at uniform grid points
	/// (Implicitly called by createGlyphs(), do not call directly!)
	/// Returns number of inserted glyphs.
	int gridSampling();

	/// Insert glyph at sample voxel location if sample matches user criterions.
	/// \param id is the VTK id into point and tensor array
	/// Returns false if sampling location is rejected (e.g. below threshold).
	bool insertGlyph( int id, int x, int y, int z );

private:
	VTKPTR<vtkPoints>         m_samplePoints;
	VTKPTR<vtkDoubleArray>    m_sampleTensors;
	VTKPTR<vtkDoubleArray>    m_sampleVectors;

	VTKPTR<vtkImageData>      m_refVol;

	TensorData m_tensors;
};

#endif // _TENSORVIS_H_
