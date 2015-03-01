# mat - SDMVis numerical toolboox

set( REDNUMMAT_SOURCES
	numerics.h
	rednum.h
	svm.cpp
	svm.h
	SVMTrain.h
	SVMTrain.cpp
	
	mattools.h
	mattools.cpp
)

add_library(mat
	${REDNUMMAT_SOURCES}
)
