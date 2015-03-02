#ifndef LOOKUPTABLE_H
#define LOOKUPTABLE_H

#define LOOKUPTABLE_XML_SUPPORT

#include <vector>
#include <string>

/// Lookup table to realize a transfer function with premultiplied alpha and 
/// sampling rate correction for volume raycasting (optional).
/// Currently only linear interpolation of RGBA values is available, so for non
/// RGB color maps one has to provide a dense sampling.
class LookupTable
{
public:
	/// Transfer function entry, mapping intensity to RGBA color
	struct Datum
	{
		Datum()
			: intensity(0.0) { rgba[0]=rgba[1]=rgba[2]=rgba[3]=0.0; }
		Datum( double in_, double r_, double g_, double b_, double a_ )
			: intensity(in_) { rgba[0]=r_; rgba[1]=g_; rgba[2]=b_; rgba[3]=a_; }
		double intensity;
		double rgba[4];

		/// Sort according to intensity
		bool operator < ( const Datum& other ) const
		{
			return (intensity < other.intensity);
		}
	};

	/// Strategies for precomputed opacity correction
	/// All strategies despite NoCorrection also do some sort of permultiplied
	/// alpha (aka opacity weighted alpha [Wittenbrink1998].
	enum CorrectionStrategies {
		/// Pass interpolated color+alpha as is 
		/// (assuming that correction is done in shader).
		NoCorrection, 
		/// As in GPU Gems: 
		///   RGB = A*RGB and A = 1 - pow(1 - A0, s0/s) 
		/// for sampling rate s and reference sampling rate s0.
		SampleRateCorrection,
		SampleRateCorrectionVTK,
		/// Heuristic by Max Hermann and Roland Ruiters: 
		///   RGB = RGBA*5.0*t and A = 1- exp( -A * t ) 
		/// for stepsize t.
		FixedExponentialCorrection,
		
	};

	/// Default c'tor
	LookupTable():
		m_referenceSampleRate( 1.0 / 0.002 ),
		m_sampleRate( 1.0 / 0.001 ),
		m_correctionStrategy( SampleRateCorrectionVTK ),
		m_name("UnnammedLookupTable")
		{}

	/// @{ Setup lookup table
	void add( double intensity, double r, double g, double b, double a );
	void add( Datum d );
	void clear() { m_data.clear(); }
	void rescaleIntensityRange( double min_, double max_ );
	void setName( std::string name ) { m_name = name; }
	std::string getName() const { return m_name; }
	///}@

	///@{ Lookup table IO
	bool read( const char* filename ); // Guess format from extension
	bool readTable( const char* filename ); // Load our custom format
	bool reload();
#ifdef LOOKUPTABLE_XML_SUPPORT
	bool readParaviewColorMap( const char* filename ); // Load Paraview XML
#endif
	///@}

	///@{ Setup correction
	void setCorrectionStrategy( int s ) { m_correctionStrategy = s; }
	int  getCorrectionStrategy() const { return m_correctionStrategy; }
	void setStepsize( double d ) { m_sampleRate = 1.0 / d; }
	void setSampleRate( double s ) { m_sampleRate = s; }
	void setReferenceStepSize( double d0 ) { m_referenceSampleRate = 1.0 / d0; }
	void setRefernceSampleRate( double s0 ) { m_referenceSampleRate = s0; }
	///@}

	///@{ Create corrected lookup table
	void getTable( std::vector<float>& buffer, int numEntries, bool applyCorrection=true ) const;
	///@}

protected:
	typedef std::vector< Datum > Table;

	/// Return interpolated and corrected RGBA datum for given intensity
	Datum getDatum( double intensity, bool applyCorrection=true ) const;

	void add( Table& table, Datum d );

private:
	Table m_data; // Assume table to be sorted wrt intensity

	double m_referenceSampleRate;
	double m_sampleRate;

	int m_correctionStrategy;

	std::string m_name;
	std::string m_filename; // Filename for reload() functionality
};

#endif // LOOKUPTABLE_H
