// Max Hermann, March 22, 2010
#ifndef RAYCASTSHADER_H
#define RAYCASTSHADER_H

#include <GL/GLSLProgram.h>
#include <GL/GLError.h>
#include <GL/GLTexture.h>
#include <cassert>

/// Raycasting shader supporting displacement vectorfields (warpfields).
/// This shader performs one-pass raycasting where ray start and end positions 
/// are assumed to be given color coded by a front- and a back-texture.
/// Main purpose of this class is to bundle GLSL program and uniform parameters.
class RaycastShader
{
public:
	enum RenderMode {
		RenderDirect    =0, ///< Direct Volume Rendering (accumulate intensity along ray)
		RenderIsosurface=1, ///< Non-polygonal Isosurface (stop ray at specific intensity)
		RenderMIP       =2, ///< Maximum-intensity-projection
		RenderSilhouette=3, ///< Render simple silhouette
		NumRenderModes
	};

	enum Constants {
		MaxWarpModes  = 32,
		MaxNumOutputs = 5
	};

	/// Supported output types, see \a set_output_type().
	enum OutputTypes { 	OutputColor, 
	                    OutputHit, 
	                    OutputNormal,
	                    OutputRayStart,
	                    OutputRayEnd
	};

	enum Integrators {
		IntegrateDisplacementField = 0,
		IntegrateSVF_Euler         = 1,
		IntegrateSVF_Midpoint      = 2,
		IntegrateSVF_RK4           = 3
	};

	RaycastShader()
		: m_shader      (NULL)
		 ,m_rendermode  (RenderDirect)
		 ,m_num_channels(1)
		 ,m_colormode   (0)
		 ,m_warp        (false)
		 ,m_multiwarp   (false)
		 ,m_meanwarp    (false)
		 ,m_stepsize    (0.005f) // good default for my application
		 ,m_isovalue    (0.042f) // dito
		 ,m_alpha_scale (0.010f) // dito
		 ,m_num_warps   ( 1 )
		 ,m_num_outputs ( 1 )
		 ,m_fs_path("shader/raycast.fs.glsl")
		 ,m_vs_path("shader/raycast.vs.glsl")
		 ,m_integrator  ( IntegrateDisplacementField )
		 ,m_integrator_steps( 2 )
		{
			m_size[0] = m_size[1] = m_size[2] = 128;
			m_warp_ofs[0] = m_warp_ofs[1] = m_warp_ofs[2] = 0;
			for( int i=0; i < MaxWarpModes; ++i )
				m_lambda[i] = 0.f;
			m_output[0] = OutputColor;
			m_output[1] = OutputHit;
			m_output[2] = OutputNormal;
			m_output[3] = OutputRayStart;
			m_output[4] = OutputRayEnd;
		}
	~RaycastShader() { if(m_shader) delete m_shader; }

	/// Configure shader before calling init() with configuration setters!
	bool init( std::string fs_path = "", std::string vs_path = "" );
	void deinit(); // destruction of shader only in GL context possible

	//void set_frontbacktex( GLTexture* front, GLTexture* back );
	//void set_voltex( GLTexture* 

	/// Bind shader and set used texture units.
	/// \param firsttexunit first texture unit
	/// \verbatim
	/// Texture unit layout starting from firsttexunit:
	///  - +0 : front texture
	///  - +1 : back texture
	///  - +2 : volume texture
	///  - warp textures follow after volume texture, then meanwarp.
	///  - last texture is the depth texture.
	/// \endverbatim
	void bind( GLuint firsttexunit=0 );
	void release();

	///@{ Configuration (will be applied on call to \a init())
	void  set_volume_size( int width, int height, int depth )
		{
			m_size[0] = width;
			m_size[1] = height;
			m_size[2] = depth;
		}
	void  set_num_channels( int nc ) { m_num_channels = nc; }
	int   get_num_channels() const { return m_num_channels; }
	void  set_colormode( int mode ) { m_colormode = mode; }
	int   get_colormode() const { return m_colormode; }
	void  set_rendermode( int/*RenderMode*/ mode )
		{
			m_rendermode = mode;
		}
	/*RenderMode*/int get_rendermode() const
		{
			return m_rendermode;
		}
	void  cycle_rendermode()
		{
			m_rendermode = static_cast<RenderMode>( ((int)m_rendermode+1) % NumRenderModes );
		}
	void  set_warp_enabled( bool b ) { m_warp = b; }
	bool  get_warp_enabled() const { return m_warp; }
	void  set_multiwarp_enabled( bool b ) { m_multiwarp = b; }
	bool  get_multiwarp_enabled() const { return m_multiwarp; }
	void  set_depth_enabled( bool b ) { m_depth = b; }
	bool  get_depth_enabled( bool b ) const { return m_depth; }
	void  set_meanwarp_enabled( bool b ) { m_meanwarp = b; }
	bool  get_meanwarp_enabled() const { return m_meanwarp; }

protected:
	bool is_valid_output_index( int i ) const
	{
		if( i<0 || i>=MaxNumOutputs )
		{
			std::cerr << "Error: Output index must be between 0 and "
					    << MaxNumOutputs-1 << "!\n";
			return false;
		}
		return true;
	}

public:

	/// Specify the number of used warp fields.
	void set_num_warps( int i )
	{
		m_num_warps = i;
	}
	int get_num_warps() const { return m_num_warps; }

	/// Specify the number of outputs between 1 and 3.
	/// Set the corresponding output type with \a set_output_type().
	void set_num_outputs( int i ) 
	{ 
		if( is_valid_output_index(i) )
			m_num_outputs = i; 
	}
	/// Returns the number of outputs.
	int  get_num_outputs() const  { return m_num_outputs; }

	/// Set the type of specific output. Outputs are numbered from 0.
	void set_output_type( int i, int type ) 
		{ 
			if( is_valid_output_index(i) )
				m_output[i] = type;
		}
	/// Returns the output type of a specific output.
	int get_output_type( int i ) const
		{
			if( is_valid_output_index(i) )
				return m_output[i];
			return -1; // invalid index
		}

	/// Set the integrator type (one of \a Integrators).
	void set_integrator( int i ) { m_integrator = i; }
	/// Returns the currently set integrator type (shoule be one of \a Integrators)
	int get_integrator() const { return m_integrator; }
	
	/// Set the number of integration steps (unused for \a IntegratorDisplacementField)
	void set_integrator_steps( int s ) { m_integrator_steps = s; }
	/// Get number of integration steps (unused for \a IntegratorDisplacementField)
	int get_integrator_steps() const { return m_integrator_steps; }

	///@}  // end of configuration dox

	///@{ Uniforms
	void  set_isovalue( float iso ) { m_isovalue = iso; }
	float get_isovalue() const { return m_isovalue; }
	void  set_alpha_scale( float sc ) { m_alpha_scale = sc; }
	float get_alpha_scale() const { return m_alpha_scale; }

	void  set_lambda( int i, float s ) { assert(i>=0&&i<MaxWarpModes); m_lambda[i]=s; }
	float get_lambda( int i ) const { assert(i>=0&&i<MaxWarpModes); return m_lambda[i]; }

	void  set_stepsize( float s ) { m_stepsize=s; }
	float get_stepsize() const { return m_stepsize; }
	///@}

	///@{ Modify first warpfield (provided for backwards compatibility)
	void  set_warp_strength( float s ) { set_lambda(0,s); }
	float get_warp_strength() const { return get_lambda(0); }
	///@}

	///@{ Offset warpfield(s)
	void  set_warp_ofs( float dx, float dy, float dz )
	{
		m_warp_ofs[0] = dx;
		m_warp_ofs[1] = dy;
		m_warp_ofs[2] = dz;
	}
	void  set_warp_ofs( float* ofs )
	{
		m_warp_ofs[0] = ofs[0];
		m_warp_ofs[1] = ofs[1];
		m_warp_ofs[2] = ofs[2];
	}
	void get_warp_ofs( float* ofs ) const
	{
		ofs[0] = m_warp_ofs[0];
		ofs[1] = m_warp_ofs[1];
		ofs[2] = m_warp_ofs[2];
	}
	void get_warp_ofs( float& dx, float& dy, float& dz )
	{
		dx = m_warp_ofs[0];
		dy = m_warp_ofs[1];
		dz = m_warp_ofs[2];
	}
	///@}

private:
	GL::GLSLProgram*  m_shader;
	GLint             m_loc_warpmode[MaxWarpModes],
		              m_loc_lambda  [MaxWarpModes],
		              m_loc_voltex,
					  m_loc_meanwarptex,
	                  m_loc_fronttex,
	                  m_loc_backtex,
					  m_loc_isovalue,
					  m_loc_alpha_scale,
	                  m_loc_warp_ofs,
					  m_loc_depthtex,
					  m_loc_stepsize,
					  m_loc_luttex;
	// configuration
	int  m_size[3];
	int  m_rendermode;
	int  m_num_channels;
	int  m_colormode;
	bool m_warp;
	bool m_multiwarp;
	bool m_meanwarp;
	bool m_depth;
	int  m_num_warps;
	int  m_num_outputs;
	int  m_output[MaxNumOutputs];
	int  m_integrator;
	int  m_integrator_steps;

	// variables
	float  m_stepsize;
	float  m_isovalue;
	float  m_alpha_scale;
	float  m_lambda[MaxWarpModes];
	float  m_warp_ofs[3];

	std::string m_fs_path, 
		        m_vs_path;
};

#endif
