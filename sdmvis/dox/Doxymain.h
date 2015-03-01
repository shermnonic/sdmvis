/**
\mainpage sdmvis

\section about About

sdmvis is a shape space visualization system based on a dense deformation model 
for volumetric data.

This software is part of the project "Explorative Shape Analysis", see the 
following project page for further details:

http://cg.cs.uni-bonn.de/en/projects/shape-analysis/semantically-steered-visual-analysis-of-shape-spaces/

\image html teaser.png

\section authors Authors

This program is developed by Max Hermann <hermann@cs.uni-bonn.de>.

Development is supported in parts by:
Vitalis Wiens <Vitalis.Wiens@rwth-aachen.de>

(c) 2010-2013 by University of Bonn,<br>
    Institute of Computer Science II,<br>
	Computer Graphics Group.		
*/

/**
	@defgroup Utils
	Utility functions
	
	@defgroup Core
	Core numerical functions
*/

/**
	\addtogroup Utils
	@{
	
	\namespace GL
	\brief OpenGL/GLSL utility functions
	
	\namespace Misc
	\brief Miscellaneous utilities like number parsing and file directory operations.
	
	\namespace MichaelsTrackball
	\brief (not used by sdmvis, part of e7 GLUT engine)
	
	@}
*/

/**
	\addtogroup Core
	@{

	\namespace mattools
	\brief Matrix utility functions to process large raw data matrices.	
	This toolkit supports out-of-core access and is designed to work with
	large data matrices. For numerical computations on smaller matrices see
	\a rednum.
	
	\namespace rednum
	\brief Very small numeric toolkit providing SVD and PCA functions.
	It is based on ublas matrix and vector classes.
	Disk IO functions are provided, mostly for Matlab interoperability.
	This toolkit is designed for small matrices, for larger out-of-core 
	processing see \a mattools.
	
	@}
*/
