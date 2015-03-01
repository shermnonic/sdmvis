#ifndef TENSORVISRENDERBASE_H
#define TENSORVISRENDERBASE_H

#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkOutlineFilter.h>
#include <vtkScalarBarActor.h>
#ifndef VTKPTR
#include <vtkSmartPointer.h>
#define VTKPTR vtkSmartPointer
#endif

#include "vtkTensorGlyph3.h"
#include "TensorSpectrum.h"

/////////////////////////////////////////////////////////
//  USER CONFIGURATION
/////////////////////////////////////////////////////////
#define TENSORVIS_USE_CUSTOM_CONE
#define TENSORVIS_USE_PROGRAMMABLE_VECTORGLYPH
/////////////////////////////////////////////////////////

#ifdef TENSORVIS_USE_PROGRAMMABLE_VECTORGLYPH
  #include <vtkProgrammableGlyphFilter.h>
  #define VECTORGLYPH vtkProgrammableGlyphFilter
#else
  #include "vtkGlyph3D_3.h"
  #define VECTORGLYPH vtkGlyph3D_3
#endif

#ifdef TENSORVIS_USE_CUSTOM_CONE
	#include "vtkConeSource2.h"
	#define CONESOURCE vtkConeSource2
#else
	#include <vtkConeSource.h>
	#define CONESOURCE vtkConeSource
#endif

// Forwards
class vtkDataArray;

/// Base class for tensor glyph visualizations with VTK. It provides all needed
/// VTK actors and an interface to the common functionality. This should make
/// it easy to change some specific implementation details without having to
/// adapt the user interface which only interfaces through this base class.
class TensorVisRenderBase
{
public:
	enum GlyphTypes { Sphere, Cylinder, Cube, SuperQuadric, Cone };
	
	TensorVisRenderBase();
	virtual ~TensorVisRenderBase() {}
	
	/// Adds tensor visualization actor(s) to given renderer
	void setRenderer( vtkRenderer* renderer );
	
	/// Show/hide all components of this visualization
	void setVisibility( bool );
	bool getVisibility() const;

	///@{ Show/hide specific parts of this visualization
	void setTensorVisibility( bool );
	bool getTensorVisibility() const;

	void setVectorfieldVisibility( bool );
	bool getVectorfieldVisibility() const;

	void setLegendVisibility( bool );
	bool getLegendVisibility() const;
	///@}
	
	///@{ Get/set glyph primitive, one of \a GlyphTypes
	void setGlyphType( int glyphType );
	int  getGlyphType() const;
	///@}
	
	/// Provide raw access to vtkTensorGlyph instance
	vtkTensorGlyph3* getTensorGlyph();
	
	/// Access active tensor glyph color lookup table to adapt color mapping
	vtkScalarsToColors* getLookupTable();	
	
	///@{ Forwards of prominent attributes of vtkTensorGlyph (immediate change)
	void   setGlyphScaleFactor( double scale );
	double getGlyphScaleFactor() const;
	void   setExtractEigenvalues( bool );
	bool   getExtractEigenvalues() const;
	void   setColorGlyphs( bool );
	bool   getColorGlyphs() const;
	void   setColorMode( int );
	int    getColorMode() const;
	void   setSuperquadricGamma( double gamma );
	double getSuperquadricGamma() const;
	///@}
	
	void updateColorMap();

	vtkActor* getTensorActor() { return m_tensorActor; }

	vtkPolyData* getSamplePolyData() { return m_samplePolyData; }
	
protected:
	void createScalarBar( vtkScalarsToColors* lut );

	VTKPTR<vtkLookupTable> getDefaultLookupTable() const;

	/// Provide raw access to the vector glyph instance in use
	VECTORGLYPH* getVectorGlyph() { return m_vectorGlyph; }
	
	/// Access vector glyph geometry
	CONESOURCE*  getVectorGlyphSource() { return m_vectorGlyphSource; }

	
	void updateGlyphData( vtkPoints* pts, vtkDataArray* tensor, 
	                      vtkDataArray* vectors=NULL );

	/// Custom glyph method callback for vectorfield visualization
	void vectorGlyphMethod();
	friend void forwardGlyphMethod( void* instance_of_TensorVis );

private:
	bool m_hasVectors;
	
	// Tensor glyph visualization
	VTKPTR<vtkTensorGlyph3>   m_tensorGlyph;
	VTKPTR<vtkPolyDataMapper> m_tensorMapper;
	VTKPTR<vtkActor>          m_tensorActor;

	VTKPTR<vtkPolyData>       m_samplePolyData;

	// Vectorfield visualization
	VTKPTR<VECTORGLYPH>       m_vectorGlyph;
	VTKPTR<CONESOURCE>        m_vectorGlyphSource; // glyph geometry
	VTKPTR<vtkPolyDataMapper> m_vectorMapper;
	VTKPTR<vtkActor>          m_vectorActor;

	// Color legend
	VTKPTR<vtkScalarBarActor> m_scalarBar;

	// The vectorfield glyph visualization requires access to the original
	// tensor data. That is a nasty thing!
	vtkDataArray* m_sampleTensors;
	
	// Visualization parameters
	int    m_glyphType;
	bool   m_visible;	
};

#endif // TENSORVISRENDERBASE_H
