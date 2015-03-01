#ifndef LOOKUPTABLE_H
#define LOOKUPTABLE_H
#include <vector>
#include <string>

class LookupTable
{
	struct Datum
	{
		float alpha;
		float rgba[4];
	};

public:
	LookupTable():
		m_stepsize( 0.001f )
		{}

	bool read( const char* filename );
	bool reload();
	void getTable( std::vector<float>& buffer, int numEntries ) const;
	void setStepsize( float s ) { m_stepsize = s; }

protected:
	Datum getDatum( float alpha ) const;

private:
	std::string m_filename; // Filename for reload() functionality
	std::vector< Datum > m_data; // Assume list to be sorted wrt first value
	float m_stepsize; // Ray-cast stepsize for exponential alpha influence
};

#endif // LOOKUPTABLE_H
