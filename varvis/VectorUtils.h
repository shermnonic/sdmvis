#ifndef VECTORUTILS_H
#define VECTORUTILS_H

/// Some vector calculus utility functions.
/// Used so far by VectorToVertexNormalFilter and VectorToMeshColorFilter.
namespace VectorUtils
{
	/// Split a vector into its perpendicular and tangential component w.r.t.
	/// a surface described by its normal.
	struct TangentialPerpendicularDecomposition
	{
		double dotProduct;
		double magnitude;
		double v_perp[3];
		double v_tan[3];
		
		TangentialPerpendicularDecomposition() 
		: dotProduct(0.0), magnitude(0.0)
		{
			v_perp[0] = v_perp[1] = v_perp[2] = 0.0;
			v_tan[0] = v_tan[1] = v_tan[2] = 0.0;
		}
			
		TangentialPerpendicularDecomposition( const double* vector, const double* normal )
		{
			compute( vector, normal );
		}
			
		void compute( const double* vector, const double* normal )
		{
			// Projection of vector onto surface normal
			dotProduct = vector[0]*normal[0] + vector[1]*normal[1] + vector[2]*normal[2];

			// Vector perpendicular to surface
			v_perp[0] = dotProduct*normal[0];
			v_perp[1] = dotProduct*normal[1];
			v_perp[2] = dotProduct*normal[2];

			// Vector tangential to surface
			v_tan[0] = vector[0] - v_perp[0];
			v_tan[1] = vector[1] - v_perp[1];
			v_tan[2] = vector[2] - v_perp[2];

			// Magnitude of perpendicular part
			magnitude = sqrt(v_perp[0]*v_perp[0] + v_perp[1]*v_perp[1] + v_perp[2]*v_perp[2]);
		}
	};	
};

#endif
