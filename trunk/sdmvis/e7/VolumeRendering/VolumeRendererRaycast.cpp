// Max Hermann, June 6, 2010
#include "VolumeRendererRaycast.h"
#include <GL/glew.h>
#include <GL/glu.h>		// for gluOrtho2D

#ifndef NO_BOOST_FILESYSTEM
#include <Misc/FilesystemTools.h>
#endif

#ifdef WIN32
// disable some VisualStudio warnings
#pragma warning(disable: 4244) 
#endif

using namespace GL;

VolumeRendererRaycast::VolumeRendererRaycast()
: m_verbosity(3)
, m_offscreenPreviewQuality(false)
, m_meanwarp(NULL)
, m_offscreen(true)
, m_debug(false)
, m_lut(NULL)
{
	m_aspect[0]=m_aspect[1]=m_aspect[2]=1.f;
	resetWarpfields();
}

VolumeRendererRaycast::~VolumeRendererRaycast()
{}	

void VolumeRendererRaycast::reshape_ortho( int w, int h )
{
	glViewport( 0,0,w,h );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluOrtho2D( 0,w,0,h ); // 1:1 pixel to coordinates relation
	glMatrixMode( GL_MODELVIEW );
}

void VolumeRendererRaycast::setVolume( GLTexture* vtex )
{
	m_vtex = vtex;
}

void VolumeRendererRaycast::setWarpfield( GLTexture* warp )
{
	resetWarpfields(1);
	m_warps[0] = warp;
}

void VolumeRendererRaycast::resetWarpfields( unsigned resize )
{
	// We do not own the pointer and thus are not required to delete them.

	// For backwards compatibility, at least 5 modes are provided.
	m_warps.resize(	std::max( (int)resize, 5 ) );
	// Initialize with NULL
	for( unsigned i=0; i < m_warps.size(); i++ )
		m_warps[i] = NULL;
}

void VolumeRendererRaycast::setMeanwarp( GLTexture* meanwarp )
{
	m_meanwarp = meanwarp;
}

void VolumeRendererRaycast::setWarpfields( GLTexture* mode0, GLTexture* mode1, 
	                                       GLTexture* mode2, GLTexture* mode3, 
										   GLTexture* mode4 )
{
	resetWarpfields(5);
	m_warps[0] = mode0;
	m_warps[1] = mode1;
	m_warps[2] = mode2;
	m_warps[3] = mode3;
	m_warps[4] = mode4;
}

void VolumeRendererRaycast::setWarpfields( std::vector<GL::GLTexture*> modes )
{
	resetWarpfields( (unsigned)modes.size() );
	for( unsigned i=0; i < modes.size(); i++ )
		m_warps[i] = modes[i];
}

void VolumeRendererRaycast::setAspect( float ax, float ay, float az )
{
	m_aspect[0] = ax;
	m_aspect[1] = ay;
	m_aspect[2] = az;

	m_cube.set_scale( 2*m_aspect[0], 2*m_aspect[1], 2*m_aspect[2] );
	m_cube.set_texcoord_scale( 1,1,1 );

	// Note:
	// Assume that texture is of same size as volume. This might not be the
	// case on older Hardware, where only power-of-two sizes are allowed.
	// For that case the following texcoord scaling according to the aspect
	// ratios should be implemented:
	//    cube.set_texcoord_scale( (g_vol->resX() / (float)g_vtex.GetWidth ()),
	//                             (g_vol->resY() / (float)g_vtex.GetHeight()),
	//                             (g_vol->resZ() / (float)g_vtex.GetDepth ()) );
}

void VolumeRendererRaycast::setIsovalue( float iso )
{
	m_raycast_shader.set_isovalue( iso );
}

float VolumeRendererRaycast::getIsovalue() const
{
	return m_raycast_shader.get_isovalue();
}

void VolumeRendererRaycast::setZNear( float znear )
{
	m_znear = znear;
}

void VolumeRendererRaycast::setRenderMode( RaycastShader::RenderMode mode )
{
	m_raycast_shader.set_rendermode( mode );
	if( !m_raycast_shader.init() )
		std::cerr << "Fatal Error: Reloading raycast shader!" << std::endl;
}


int VolumeRendererRaycast::getRenderMode() const
{
	return (int)m_raycast_shader.get_rendermode();
}

void VolumeRendererRaycast::setOutputType( int type )
{
	m_raycast_shader.set_output_type( 0, type );
	reinit_shader( m_raycast_shader );
}

int VolumeRendererRaycast::getOutputType() const
{
	return m_raycast_shader.get_output_type(0);
}

void VolumeRendererRaycast::setColorMode( int mode )
{
	m_raycast_shader.set_colormode( mode );
}

int VolumeRendererRaycast::getColorMode() const
{
	return m_raycast_shader.get_colormode();
}

void VolumeRendererRaycast::setLambda( int i, float lambda )
{	
	m_raycast_shader.set_lambda( i, lambda );
}

float VolumeRendererRaycast::getLambda( int i ) const
{
	return m_raycast_shader.get_lambda( i );
}

void  VolumeRendererRaycast::setAlphaScale( float s )
{
	m_raycast_shader.set_alpha_scale( s );
}

float VolumeRendererRaycast::getAlphaScale() const
{
	return m_raycast_shader.get_alpha_scale();
}

void VolumeRendererRaycast::setStepsize( float step )
{
	m_raycast_shader.set_stepsize( step );

	// Change in stepsize requires new pre-intergration of LUT 
	updateLookupTable();
}

float VolumeRendererRaycast::getStepsize() const
{
	return m_raycast_shader.get_stepsize();
}

void VolumeRendererRaycast::setIntegrator( int type )
{
	m_raycast_shader.set_integrator( type );
	if( !m_raycast_shader.init() )
		std::cerr << "Fatal Error: Reloading raycast shader!" << std::endl;
}

int VolumeRendererRaycast::getIntegrator() const
{
	return m_raycast_shader.get_integrator();
}

void VolumeRendererRaycast::setIntegratorSteps( int step )
{
	m_raycast_shader.set_integrator_steps( step );
	if( !m_raycast_shader.init() )
		std::cerr << "Fatal Error: Reloading raycast shader!" << std::endl;
}

int VolumeRendererRaycast::getIntegratorSteps() const
{
	return m_raycast_shader.get_integrator_steps();
}

void VolumeRendererRaycast::getAspect( float& ax, float& ay, float& az ) const
{
	ax = m_aspect[0];
	ay = m_aspect[1];
	az = m_aspect[2];
}

void VolumeRendererRaycast::setOffscreenTextureSize( int width, int height )
{
	// Allocate GPU memory
	GLint internalFormat = GL_RGBA32F; //was: GL_RGB12 (see note below)
	m_front.Image(0, internalFormat, width,height, 0, GL_RGBA, GL_FLOAT, NULL );
	m_back .Image(0, internalFormat, width,height, 0, GL_RGBA, GL_FLOAT, NULL );
	m_vren .Image(0, internalFormat, width,height, 0, GL_RGBA, GL_FLOAT, NULL );
	m_vrenlq.Image(0, internalFormat, width/2,height/2, 0, GL_RGBA, GL_FLOAT, NULL );

#ifdef VOLUMERENDERERRAYCAST_PICKING
	// Picking texture has fixed size 1x1
	for( int i=0; i < NumPickOutputs; i++ )
		m_pick_outputs[i].Image(0, internalFormat, 1,1, 0, GL_RGBA, GL_FLOAT, NULL );
#endif

	// Note on internalFormat for legacy hardware support:
	// A channel resolution of 8-bit leads to quantization artifacts so to 
	// support legacy hardware one could use a 12-bit format instead of RGBA32F.
	// RGB12 for example should be hardware supported also on pretty old GPU's.
	// Alternatively one could resort to generate start/end positions explicitly
	// via simple arithmetics on the CPU or directly in the shader.
}

//------------------------------------------------------------------------------
//	getNumberOfValidWarps()
//------------------------------------------------------------------------------
int VolumeRendererRaycast::getNumberOfValidWarps() const
{
	int num=0;
	for( unsigned i=0; i < m_warps.size(); i++ )
		if( m_warps[i] != NULL )
			num++;
	return num;
}

//------------------------------------------------------------------------------
//	reinit_shader()
//------------------------------------------------------------------------------
bool VolumeRendererRaycast::reinit_shader( RaycastShader& shader, 
	                                 std::string fs_path, std::string vs_path )
{
	assert( m_vtex );
	shader.set_volume_size     ( m_vtex->GetWidth (), 
	                             m_vtex->GetHeight(), 
	                             m_vtex->GetDepth () );
	shader.set_warp_enabled    ( m_warps[0]!=NULL );
	shader.set_multiwarp_enabled(m_warps[0]!=NULL && m_warps[1]!=NULL);
						// was: 
	                    //         && m_warps[2]!=NULL && m_warps[3]!=NULL &&
	                    //            m_warps[4]!=NULL );
	shader.set_meanwarp_enabled( m_meanwarp!=NULL );
	shader.set_num_warps( getNumberOfValidWarps() );
	if( !shader.init( fs_path, vs_path ) )
	{
		std::cerr << "Error: Couldn't init raycast shader!" << std::endl;
		return false;
	}

	updateLookupTable();

	return true;
}

bool VolumeRendererRaycast::reinit_shader( 
	                                 std::string fs_path, std::string vs_path )
{
	return 
		reinit_shader( m_raycast_shader, fs_path, vs_path )
#ifdef VOLUMERENDERERRAYCAST_PICKING
	 && reinit_shader( m_pick_shader   , fs_path, vs_path )
#endif
		;

}

//------------------------------------------------------------------------------
//	init()
//------------------------------------------------------------------------------
bool VolumeRendererRaycast::init( int texWidth, int texHeight )
{
	using namespace std;

	assert( m_vtex );
	
	// --- Find shader source ---

	// find shader source
	string fs_path, vs_path, lut_path;
#ifndef NO_BOOST_FILESYSTEM
	int verbosity = 0;
	vector<string> search_paths, search_relpaths;
	search_paths.push_back( "./" );
	search_paths.push_back( "../");
	search_paths.push_back( "../../");
	//search_paths.push_back( argv[0] );
	//search_paths.push_back( boost::filesystem::initial_path().string() );  // same as "./"
	search_paths.push_back( Misc::get_executable_path() );	
	//search_relpaths.push_back("../");

	// FIXME: project specific search path
	search_relpaths.push_back("shader/");
	search_relpaths.push_back("data/shader/");
	search_relpaths.push_back("../e7/VolumeRendering/");
	search_relpaths.push_back("../sdmvis/e7/VolumeRendering/");
	search_relpaths.push_back("../trunk/sdmvis/e7/VolumeRendering/");
	search_relpaths.push_back("Release/");
	search_relpaths.push_back("../raycast/");
	search_relpaths.push_back("../../raycast/");

	fs_path = Misc::find_file( "raycast.fs.glsl", search_paths, "shader", search_relpaths, true, verbosity );
	vs_path = Misc::find_file( "raycast.vs.glsl", search_paths, "shader", search_relpaths, true, verbosity );
	lut_path= Misc::find_file( "raycast_default_lookup.xml", 
		                                          search_paths, "shader", search_relpaths, true, verbosity );
	cout << "Using raycast fragment shader found in .\\" << fs_path << endl;
	if( fs_path.compare( vs_path ) != 0 )
		cout << "Using raycast vertex shader found in .\\" << vs_path << endl;
	cout << "Found default lookup table in .\\" << lut_path << endl;
#endif
	
	// --- Setup default lookup table ---

	if( lut_path.empty() )
		lut_path = "shader/raycast_default_lookup.xml";

	if( !m_default_lut.read( lut_path.c_str() ) )
	{
		cerr << "Error: Default lookup table could not be loaded!" << endl;
		//return false;
	}
	else
	{
		m_lut = &m_default_lut;
	}

	// --- Init RaycastShader ---
	
	m_raycast_shader.set_num_channels ( 1 ); // m_vol->numChannels()	
	m_raycast_shader.set_rendermode   ( RaycastShader::RenderIsosurface );
	m_raycast_shader.set_depth_enabled( VOLUMERENDERERRAYCAST_USE_DEPTH );

#ifdef VOLUMERENDERERRAYCAST_PICKING
	m_pick_shader.set_num_channels( 1 );
	m_pick_shader.set_rendermode( RaycastShader::RenderIsosurface );
	m_pick_shader.set_num_outputs( 4 );
	m_pick_shader.set_output_type( 0, RaycastShader::OutputHit      );
	m_pick_shader.set_output_type( 1, RaycastShader::OutputNormal   );
	m_pick_shader.set_output_type( 2, RaycastShader::OutputRayStart );
	m_pick_shader.set_output_type( 3, RaycastShader::OutputRayEnd   );
	m_pick_shader.set_depth_enabled( false );
#endif

	// --- Setup 2D front and back texture ---
	
	if( !createTextures(texWidth,texHeight) ) 
		return false;


	// --- Init RenderToTexture ---
	
	if( !createRenderToTexture(texWidth,texHeight) ) 
		return false;

	// --- Update shader ---

	if( !reinit_shader( fs_path, vs_path ) )
		return false;
	
	return true;	
}

bool VolumeRendererRaycast::createTextures( int texWidth, int texHeight )
{
	using namespace std;
	if( m_verbosity > 1 ) cout << "Creating 2D textures..." << endl;
#ifdef VOLUMERENDERERRAYCAST_PICKING
	for( int i=0; i < NumPickOutputs; i++ )
	{
		if( !m_pick_outputs[i].Create(GL_TEXTURE_2D) )
		{
			cerr << "Error: Couldn't create 2D textures!" << endl;
			return false;
		}

		m_pick_output_ids[i] = m_pick_outputs[i].GetID();
	}
#endif
	if(     !m_front      .Create(GL_TEXTURE_2D) 
		|| 	!m_back       .Create(GL_TEXTURE_2D) 
		||	!m_vren       .Create(GL_TEXTURE_2D) 
		||  !m_vrenlq     .Create(GL_TEXTURE_2D)
		||  !m_lut_tex    .Create(GL_TEXTURE_1D)
		)
	{
		cerr << "Error: Couldn't create 2D textures!" << endl;
		return false;
	}
	// allocate GPU mem
	setOffscreenTextureSize( texWidth, texHeight );
	return true;
}

void VolumeRendererRaycast::destroyTextures()
{
	using namespace std;
	if( m_verbosity > 1 ) cout << "Destroying 2D textures..." << endl;
	m_front      .Destroy();
	m_back       .Destroy();
	m_vren       .Destroy();
	m_vrenlq     .Destroy();
	m_lut_tex    .Destroy();
#ifdef VOLUMERENDERERRAYCAST_PICKING
	for( int i=0; i < NumPickOutputs; i++ )	
		m_pick_outputs[i].Destroy();
#endif
}

void VolumeRendererRaycast::changeTextureSize( int width, int height )
{
	using namespace std;
	if( m_verbosity > 1 ) cout << "Changing texture size to " 
							   << width << " x " << height << endl;

	if( width==getTextureWidth() && height==getTextureHeight() )
		return;

	destroyRenderToTexture();
	destroyTextures();

	if( !createTextures(width,height) )
		throw; // FIXME: replace runtime error by purposeful exception
	if( !createRenderToTexture(width,height) )
		throw; // FIXME: replace runtime error by purposeful exception
}

bool VolumeRendererRaycast::createRenderToTexture( int texWidth, int texHeight )
{
	using namespace std;
	if( m_verbosity > 1 ) cout << "Initializing rendering to texture...\n";
	if(    !m_r2t.init( getTextureWidth(),getTextureHeight(), m_front.GetID(), 
		                 VOLUMERENDERERRAYCAST_USE_DEPTH ) 
#ifdef VOLUMERENDERERRAYCAST_PICKING
	 	|| !m_pick_r2t.init( 1,1, m_pick_output_ids, 4, false )
		//   was:     .init( 1, 1, m_pick_output.GetID(), false )
#endif
		)
	{
		cerr << "Error: Couldn't initalize rendering to texture!" << endl;
		return false;
	}
	return true;
}

void VolumeRendererRaycast::destroyRenderToTexture()
{
	m_r2t.deinit();
}

//------------------------------------------------------------------------------
//	destroy()
//------------------------------------------------------------------------------
void VolumeRendererRaycast::destroy()
{
	std::cout << "VolumeRendererRaycast::destroy()" << std::endl;
	m_raycast_shader.deinit();
	destroyRenderToTexture();
	destroyTextures();
}

//------------------------------------------------------------------------------
//	render functions
//------------------------------------------------------------------------------

// Return unit texture coordinates (maybe with half-pixel offset)
void get_texcoords( float& s0, float& s1, float& t0, float& t1 )
{
#if 1
	s0 = 0.; //(.5f + 0) / (float)g_texwidth;
	s1 = 1.; //(.5f + g_texwidth-1) / (float)g_texwidth;
	t0 = 0.; //(.5f + 0) / (float)g_texheight;
	t1 = 1.; //(.5f + g_texheight-1) / (float)g_texheight;
#else
	// FIXME: half pixel offset really needed here?
	s0 = (.5f + 0) / (float)g_texwidth;
	s1 = (.5f + g_texwidth-1) / (float)g_texwidth;
	t0 = (.5f + 0) / (float)g_texheight;
	t1 = (.5f + g_texheight-1) / (float)g_texheight;
#endif
}

/// Draw a single quad from (0,0) to (W,H)
void draw_quad( float W, float H, GLenum unit=GL_TEXTURE0 )
{
	// get texture coordinates (maybe with half-pixel offset)
	float s0, s1, t0, t1;
	get_texcoords( s0, s1, t0, t1 );

	// make sure the correct unit is assigned (needed?)
	glActiveTexture( unit );

	// FIXME: do we produce an overdraw when viewSize > texture resolution?
	glBegin( GL_QUADS );
	glMultiTexCoord2f( unit, s0,t0 );  glVertex2f( 0,0 );
	glMultiTexCoord2f( unit, s1,t0 );  glVertex2f( W,0 );
	glMultiTexCoord2f( unit, s1,t1 );  glVertex2f( W,H );
	glMultiTexCoord2f( unit, s0,t1 );  glVertex2f( 0,H ); 
	glEnd();
}

/// Draw a single point at pixel coordinate (x,y) relative to size (W,H). 
/// Note that (W,H) is required to compute normalized texture coordinates.
void draw_point( float x, float y, float W, float H, GLenum unit=GL_TEXTURE0 )
{
	glActiveTexture( unit );
#if 0
	glBegin( GL_POINTS );
	glMultiTexCoord2f( unit, x/(W-1), y/(H-1) ); glVertex2f(x,y);
	glEnd();
#else
	float s0, s1, t0, t1;
	s0 = s1 = x;
	t0 = t1 = y;
	glBegin( GL_QUADS );
	glMultiTexCoord2f( unit, s0,t0 );  glVertex2f( 0,0 );
	glMultiTexCoord2f( unit, s1,t0 );  glVertex2f( W,0 );
	glMultiTexCoord2f( unit, s1,t1 );  glVertex2f( W,H );
	glMultiTexCoord2f( unit, s0,t1 );  glVertex2f( 0,H ); 
	glEnd();
#endif
}

VolumeRendererRaycast::RenderInfo VolumeRendererRaycast::beginRender()
{
	RenderInfo ri;

	// save common states
	glPushAttrib( GL_ALL_ATTRIB_BITS );	

	// save projection and modelview matrix
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glMatrixMode( GL_MODELVIEW );	
	glPushMatrix();	
	
	// fill RenderInfo

	GLint viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );
	
	ri.viewWidth  = viewport[2],
	ri.viewHeight = viewport[3];
	
	// front, back and offscreen texture are assumed to have identical size
	ri.texWidth  = m_front.GetWidth(),
	ri.texHeight = m_front.GetHeight();	

	// blend mode
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glBlendEquation( GL_FUNC_ADD );
	//glEnable( GL_BLEND );
	
	// REMARK: 
	// - blending defines how raycasting result (a texture) is combined with
	//   the existing scene, especially noticeable for direct volume rendering
	// - other blending functions:
	//		for opacity-weighted color values use: 
	//             ( GL_ONE, GL_ONE_MINUS_SRC_ALPHA )
	//		default blending:
	//             ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA )

	return ri;
}

void VolumeRendererRaycast::endRender()
{
	//  Restore OpenGL states

	glActiveTexture( GL_TEXTURE0 );

	// restore modelview
	glPopMatrix();
	
	// restore original projection
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
	
	// restore states
	glPopAttrib();	
}

void VolumeRendererRaycast::renderFrontAndBack( RenderInfo ri )
{
	//--------------------------------------------------------------------------
	// Step 1) Light ray setup
	// - rasterize front and back faces of volume geometry
	// - store in separate textures "back" and "front"
	// - ray-direction = back minus front

	glFrontFace( GL_CCW );
	glEnable( GL_CULL_FACE );
	glDisable( GL_TEXTURE_3D );

	glTranslatef( -m_aspect[0], -m_aspect[1], -m_aspect[2] );

	// set viewport to match texture dimensions
	glViewport( 0,0, ri.texWidth,ri.texHeight );
	glClearColor( 0,0,0,0 ); // clearcolor (0,0,0) indicates zero ray length

	// --- a) render back faces ---
	m_r2t.bind( m_back.GetID() );

	glClear( GL_COLOR_BUFFER_BIT );

	glCullFace( GL_FRONT );
	m_cube.draw_rgbcube();

	m_r2t.unbind();

	// --- b) render front faces ---
	m_r2t.bind( m_front.GetID(), VOLUMERENDERERRAYCAST_USE_DEPTH );

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ); 

	glCullFace( GL_BACK );
	m_cube.draw_rgbcube();

	glDisable( GL_CULL_FACE );
	m_cube.draw_nearclip( (float)m_znear + 0.01f );

	m_r2t.unbind();

}

void VolumeRendererRaycast::renderRaycast( RenderInfo ri, RaycastShader& shader,
	                                       RenderToTexture* r2t, GL::GLTexture* tex,
										   GLint* texIDs, int n,
										   bool renderToScreen, 
										   bool renderPoint, float x, float y )
{
	//--------------------------------------------------------------------------
	// Step 2) Traverse ray
	// - use single-pass shader with loops (see RaycastShader)

	glDisable( GL_DEPTH_TEST );
	glDisable( GL_LIGHTING );
	glDisable( GL_CULL_FACE );
	if( renderToScreen )
		glEnable( GL_BLEND );
	else
		// Avoid discarding alpha=0 results
		glDisable( GL_BLEND );
	glColor4f( 1,1,1,1 );

	// setup renderer
	if( r2t && tex )
	{
		// setup to render a texture sized quad
		reshape_ortho( tex->GetWidth(), tex->GetHeight() );
		//was: reshape_ortho( ri.texWidth, ri.texHeight );
		glLoadIdentity();

		// enable render to texture
		if( !texIDs || n<=0 )
			// bind single texture (the explicitly given GLTexture2D)
			r2t->bind( tex->GetID(), VOLUMERENDERERRAYCAST_USE_DEPTH );
		else
			// bind multiple textures (as given by texIDs)
			r2t->bind( texIDs, n, VOLUMERENDERERRAYCAST_USE_DEPTH );

		// FIXME: clear needed?
		glClearColor( 0,0,0,0 );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}
	else
	{
		// render screen sized quad
		reshape_ortho( ri.viewWidth, ri.viewHeight );
		glLoadIdentity();
	}

	// setup shader
	{	
		// REMARK:
		// Textures must be bound according to the specific texture unit
		// layout specified in RaycastShader::bind().

		// bind textures to specific texture units
		GLuint tex0=1;
		GLuint curtex=tex0;
		m_front.Bind( curtex++ );
		m_back .Bind( curtex++ );
		m_vtex->Bind( curtex++ );
		m_lut_tex.Bind( curtex++ );
		
		// bind warp textures (if available)		
		int numValidWarps = 0;
		if( m_warps[0] )
		{
			// FIXME: assume that either only first or all five warps are set!			
			for( int i=0; i < m_warps.size(); ++i )
				if( m_warps[i] )
				{
					m_warps[i]->Bind( curtex++ );
					numValidWarps++;
				}			
			
			if( m_meanwarp )				
				m_meanwarp->Bind( curtex++ );
		}

		// WORKAROUND: Recompile shader if number of warps not set correctly.
		if( m_raycast_shader.get_num_warps() != numValidWarps )
		{
			std::cerr << "Error: Mismatch in number of warpfields!\n";			
		}

		if( r2t )
		{
			// bind front geometry depth as last unit
			glActiveTexture( GL_TEXTURE0 + curtex++ );
			glBindTexture( GL_TEXTURE_2D, r2t->getDepthTex() );
		}

		shader.bind(tex0);
	}	

	// produce fragment stream
	if( r2t && tex )
	{
		// render volume to texture		
		if( !renderPoint )
		{
			draw_quad( tex->GetWidth(), tex->GetHeight(), GL_TEXTURE0 );
			//was: draw_quad( ri.texWidth, ri.texHeight, GL_TEXTURE0 );
		}
		else
		{
			draw_point( x, y, ri.texWidth, ri.texHeight, GL_TEXTURE0 );
		}

		shader.release();
		r2t->unbind();

		// render result texture to screen
		if( renderToScreen )
		{
			reshape_ortho( ri.viewWidth, ri.viewHeight );

			getOffscreenTexture().Bind();

			//const GLfloat envcol[4] = { 0,0,0,0 };
			//glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND );
			//glTexEnvfv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, envcol );
			//glDisable( GL_BLEND );
			
			//glBlendFunc( GL_ONE, GL_SRC_ALPHA ); //GL_ONE_MINUS_SRC_ALPHA );

			glAlphaFunc( GL_GREATER, (GLclampf)0.0001 );
			glEnable( GL_ALPHA_TEST );

			glEnable( GL_TEXTURE_2D );
			draw_quad( ri.viewWidth, ri.viewHeight, GL_TEXTURE0 );

			glDisable( GL_ALPHA_TEST );
		}
	}
	else
	{
		// draw quad
		draw_quad( ri.viewWidth, ri.viewHeight, GL_TEXTURE0 );
		shader.release();
	}
}

void VolumeRendererRaycast::renderDebugTextures()
{
	// Draw some debug textures into lower left corner:
	// - front texture
	// - back texture
	// - pick output (scaled)		
	glDisable( GL_BLEND );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );
	glEnable( GL_TEXTURE_2D );

	reshape_ortho( 200 + NumPickOutputs*100, 100 );
	glClear( GL_DEPTH_BUFFER_BIT ); // HACK
	glPushMatrix();
	glLoadIdentity();		   m_front.Bind();		 draw_quad( 100, 100 );
	glTranslatef( 100,0,0 );   m_back .Bind();		 draw_quad( 100, 100 );
#ifdef VOLUMERENDERERRAYCAST_PICKING
	for( int i=0; i < NumPickOutputs; i++ )
	{
		glTranslatef( 100,0,0 );
		m_pick_outputs[i].Bind(); draw_quad( 100, 100 );
	}
#endif
	glTranslatef( 100,0,0 ); m_lut_tex.Bind(); 	 draw_quad( 100, 100 );
	glPopMatrix();
}

//------------------------------------------------------------------------------
//	render()
//------------------------------------------------------------------------------
void VolumeRendererRaycast::render()
{
	if( !m_vtex )
		return;
	
	// Setup
	RenderInfo ri = beginRender();
	
	// Step 1) Light ray setup
	// - rasterize front and back faces of volume geometry
	// - store in separate textures "back" and "front"
	// - ray-direction = back minus front
	renderFrontAndBack( ri );

	// Step 2) Traverse ray
	// - use single-pass shader with loops (see RaycastShader)
	if( m_offscreen )
		renderRaycast( ri, m_raycast_shader, &m_r2t, &getOffscreenTexture() );
	else
		renderRaycast( ri, m_raycast_shader );

	// Draw some debug textures into lower left corner
	if( m_debug )
		renderDebugTextures();
	
	//  Restore OpenGL states
	endRender();
}

//------------------------------------------------------------------------------
//	pick()
//------------------------------------------------------------------------------

void VolumeRendererRaycast::getPickResult( int i, float* result )
{	
	m_pick_r2t.bind( m_pick_outputs[i].GetID() );
	glReadPixels( 0,0,1,1, GL_RGBA, GL_FLOAT, result );
	m_pick_r2t.unbind();
}

RayPickingInfo VolumeRendererRaycast::pick( float x, float y )
{
	RayPickingInfo info;
	if( !m_vtex )
		return info;

	RenderInfo ri = beginRender();

	// Light ray setup can be skipped. We assume that valid start/end positions
	// have already been written to front/back texture in a previous render()
	// call.

	// Raycast single ray through x,y.
	// Do not render to screen.
	renderRaycast( ri, m_pick_shader, &m_pick_r2t, &m_pick_outputs[0], 
	               m_pick_output_ids, NumPickOutputs,
	               false, /* render2screen */
		           true, x, y );

	// Read back result
	float result[4];
	// -- Intersection point
	{
		getPickResult( 0, result );
		// We only have a valid intersection if we hit the isosurface indicated
		// by an alpha value of 1.0.
		if( result[3] > 0.5 )
			info.setIntersection( result[0], result[1], result[2] );
	}	
	// -- Normal
	if( NumPickOutputs > 1 )
	{
		getPickResult( 1, result );
		// We only have a valid normal if we hit the isosurface indicated
		// by an alpha value of 1.0.
		if( result[3] > 0.5 )
			info.setNormal( result[0], result[1], result[2] );
	}
	// -- Ray start
	if( NumPickOutputs > 2 )
	{
		getPickResult( 2, result );
		info.setRayStart( result[0], result[1], result[2] );
	}
	// -- Ray end
	if( NumPickOutputs > 3 )
	{
		getPickResult( 3, result );
		info.setRayEnd( result[0], result[1], result[2] );
	}

	// Update debug textures
	if( m_debug )
		renderDebugTextures();

	endRender();

	// Return result 	
	return info;
}


//------------------------------------------------------------------------------
//	lookup table functions
//------------------------------------------------------------------------------

void VolumeRendererRaycast::setLookupTable( LookupTable* lut )
{
	m_lut = lut;

	if( !lut )
		return;

	updateLookupTable();
}

void VolumeRendererRaycast::updateLookupTable()
{
	const int numColors = 256;

	// Sanity check
	if( !m_lut )
	{
		std::cout << "Warning: Lookup table for raycaster invalid!\n";
		return;
	}

	// WORKAROUND: Reload from disk
	if( !m_lut->reload() )
	{
		std::cout << "Warning: Lookup table could not be loaded from disk!\n";
	}

	// Change LUT stepsize for exp-integration
	m_lut->setStepsize( m_raycast_shader.get_stepsize() );
		
	// Get (new) LUT
	std::vector<float> buffer;
	m_lut->getTable( buffer, numColors );

	// Download to GPU
	m_lut_tex.SetWrapMode( GL_CLAMP_TO_EDGE );
	m_lut_tex.SetFilterMode( GL_LINEAR );
	m_lut_tex.Image( 0, GL_RGBA32F, numColors, 0, GL_RGBA, GL_FLOAT, 
		             (void*)(&buffer[0]) );
}