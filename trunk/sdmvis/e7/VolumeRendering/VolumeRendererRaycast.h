// Max Hermann, June 6, 2010
#ifndef VOLUMERENDERERRAYCAST_H
#define VOLUMERENDERERRAYCAST_H

#include <GL/GLTexture.h>
#include <GL/RenderToTexture.h>
#include "ClipCube.h"
#include "RaycastShader.h"
#include "RayPickingInfo.h"
#include "LookupTable.h"

// FIXME: Raycaster depth buffer support is not here yet. Turning it off may
//        increase rendering performance on some systems.
#define VOLUMERENDERERRAYCAST_USE_DEPTH false

#define VOLUMERENDERERRAYCAST_PICKING

class VolumeRendererRaycast
{
public:
	VolumeRendererRaycast();
	virtual ~VolumeRendererRaycast();

	/// Initialize raycast shader.
	/// This function should only be called once, see also \a reinit_shader().
	/// \param textWidth 	size of render texture
	/// \param textHeight 	size of render texture
	bool init( int texWidth=512, int texHeight=512 );
	void destroy();

	/// Execute raycaster and render result.
	void render();

	/// Execute ray-picking at given coordinates. 
	/// Ray-picking requires a previous render() call since intermediate
	/// textures will be re-used.
	RayPickingInfo pick( float x, float y );

	/// Reload shader
	// Q: Really needed to be public? Currently yes, because the setters don't
	//    reload the shader and init() should only be called once!
	bool reinit() { return reinit_shader(); }

	/// Set volume texture
	void setVolume( GL::GLTexture* vtex );

	/// Set single warpfield and enable corresponding visualization mode
	void setWarpfield( GL::GLTexture* warp );

	/// Set multiple warpfields and enable corresponding visualization mode
	void setWarpfields( GL::GLTexture* mode0, GL::GLTexture* mode1, GL::GLTexture* mode2,
		                GL::GLTexture* mode3, GL::GLTexture* mode4 );

	void setWarpfields( std::vector<GL::GLTexture*> modes );

	void resetWarpfields( unsigned resize=5 );

	/// Set mean warpfield, which will be added to any reconstruction
	void setMeanwarp( GL::GLTexture* meanwarp );

	///@{ Aspect ratio of image volume
	void setAspect( float ax, float ay, float az );
	void getAspect( float& ax, float& ay, float &az ) const;
	///@}

	///@{ Guarded access to attributes of \a RaycastShader
	void  setRenderMode( RaycastShader::RenderMode mode );
	int   getRenderMode() const;
	void  setIsovalue  ( float iso );
	float getIsovalue  () const;
	void  setZNear     ( float znear );
	void  setOutputType( int type );
	int   getOutputType() const;
	void  setColorMode( int mode );
	int   getColorMode() const;	
	void  setLambda( int i, float lambda );
	float getLambda( int i ) const;
	void  setAlphaScale( float s );
	float getAlphaScale() const;
	void  setStepsize( float step );
	float getStepsize() const;
	void  setIntegrator( int type );
	int   getIntegrator() const;
	void  setIntegratorSteps( int step );
	int   getIntegratorSteps() const;
	///@}

	///@{ Texture size for offscreen rendering and light ray setup.
	void changeTextureSize( int width, int height );
	int  getTextureWidth () const { return m_front.GetWidth(); }
	int  getTextureHeight() const { return m_front.GetHeight(); }
	int  getRenderTexture() const { return m_vren.GetID(); }
	///@}


	///@{ Offscreen rendering
	void setOffscreen( bool b ) { m_offscreen = b; }
	bool getOffscreen() const { return m_offscreen; }
	///@}

	///@{ Render additional debug textures
	void setDebug( bool b ) { m_debug = b; }
	bool getDebug() const { return m_debug; }
	///@}

	void setLookupTable( LookupTable* lut );

protected:
	RaycastShader& getRaycastShader() { return m_raycast_shader; }
	const RaycastShader& getRaycastShader() const { return m_raycast_shader; }

	bool createTextures( int texWidth, int texHeight );
	void destroyTextures();
	bool createRenderToTexture( int texWidth, int texHeight );
	void destroyRenderToTexture();

	void setOffscreenTextureSize( int width, int height );

	void reshape_ortho( int w, int h );
	bool reinit_shader( std::string fs_path="", std::string vs_path="" );
	bool reinit_shader( RaycastShader& shader, std::string fs_path="", std::string vs_path="" );

	struct RenderInfo
	{
		int viewWidth, viewHeight;
		int texWidth, texHeight;
	};

	RenderInfo beginRender();
	void       endRender();

	void renderFrontAndBack( RenderInfo ri );	
	void renderRaycast( RenderInfo ri, RaycastShader& shader, 
		                RenderToTexture* r2t=NULL, GL::GLTexture* tex=NULL,
						GLint* texIDs=NULL, int n=0,
						bool renderToScreen=true,
						bool renderPoint=false, float x=0.f, float y=0.f );
	void renderDebugTextures();

#ifdef VOLUMERENDERERRAYCAST_PICKING
	void getPickResult( int i, float* result );
#endif
		
	int getNumberOfValidWarps() const;

	void updateLookupTable();

private:
	int m_verbosity;

	GL::GLTexture   m_front,  // front cube(s) (ray start positions)
	                m_back,   // back cube(s) (ray end positions)
					m_vren,   // raycasted volume (if not directly rendered)	
	                m_lut_tex;// lookup table

	ClipCube        m_cube;
	RenderToTexture m_r2t;
	RaycastShader   m_raycast_shader;
	LookupTable*    m_lut;
	LookupTable     m_default_lut;

#ifdef VOLUMERENDERERRAYCAST_PICKING
	// Picking
	enum { NumPickOutputs = 4 };
	RaycastShader   m_pick_shader;
	RenderToTexture m_pick_r2t;

	GL::GLTexture   m_pick_outputs   [NumPickOutputs];
	GLint           m_pick_output_ids[NumPickOutputs];
#endif

	// 3D textures (managed externally)
	GL::GLTexture *m_vtex,     // scalar field (required)
	              *m_meanwarp; // mean displacement field (optional with m_warp)
	// Set of displacement fields (optional)
	std::vector<GL::GLTexture*> m_warps;
	// Note:
	// Warp stuff still experimental! 
	// Should probably be put into own class/shader.

	float m_aspect[3];     // volume aspect ratio
	float m_alpha;         // alpha scaling factor for direct volume rendering
	                       // TODO: Inconsistent to store alpha value redundantly
	                       //       here and in RaycastShader.
	float m_znear;         // znear is needed for drawing near plane

	bool m_offscreen;
	bool m_debug;
};

#endif // VOLUMERENDERERRAYCAST_H
