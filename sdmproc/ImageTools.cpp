#include "ImageTools.h"

#include <iostream>
#include <string>
#include <boost/algorithm/string.hpp>

#include <vtkAlgorithm.h>
#include <vtkCallbackCommand.h>
#include <vtkGridTransform.h>
#include <vtkImageReslice.h>
#include <vtkCommand.h>
#include <vtkMetaImageReader.h>
#include <vtkXMLImageDataReader.h>
#include <vtkMetaImageWriter.h>
#include <vtkXMLImageDataWriter.h>

#define VTKNEW( CLASS, NAME ) \
			vtkSmartPointer<CLASS> NAME = vtkSmartPointer<CLASS>::New();


/// Returns file extension as upper-case and empty string if none found
std::string getExtension( std::string filename )
{
	using namespace std;

	// Get file extension (required to deduce format)
	size_t sep = filename.rfind('.');
	if( sep == string::npos )
	{
		// Return empty string if no extension found
		cerr << "Error: No file extensions recognized in filename!\n";
		return string();
	}

	// Return uppercase extensino
	return boost::to_upper_copy(filename.substr(sep+1));
}


//-----------------------------------------------------------------------------
//	ImageProcessingCallback
//-----------------------------------------------------------------------------

void ImageProcessingCallback( vtkObject* caller, unsigned long eventId, 
	                           void* , void* )
{
	vtkAlgorithm* algo = dynamic_cast<vtkAlgorithm*>( caller );
	if( algo )
	{
		double progress = algo->GetProgress();
		std::cout << "Image processing " << algo->GetProgress() * 100 << "% \r";

		if( eventId == vtkCommand::EndEvent )
			std::cout << "Image processing finished      \n";
	}
}


//=============================================================================
//	ImageWarp
//=============================================================================
ImageWarp::
	ImageWarp( vtkImageData* source, vtkImageData* displacement, bool inverse, 
	           int interp )
{
	VTKNEW( vtkGridTransform,       warp     )
	VTKNEW( vtkImageReslice,        reslice  )
	VTKNEW( vtkCallbackCommand,     progress )	
	
	// Progress callback
	progress->SetCallback( ImageProcessingCallback );
	reslice->AddObserver( vtkCommand::ProgressEvent, progress );
	reslice->AddObserver( vtkCommand::EndEvent,      progress );

	// Setup warp
	warp->SetDisplacementGrid( displacement );
	warp->SetInterpolationMode( interp );

	if( inverse )
	{
		// Assuming input warpfield is already inverse / backwards mapping.
		// nop
	}
	else
	{
		// Assuming input warpfield to be forward mapping, i.e. inverse for 
		// backward mapping will be estimated on the fly.
		warp->Inverse();
	}

	reslice->SetInput( source );
	reslice->SetInterpolationMode( interp );
	reslice->SetResliceTransform( warp );

	warp   ->Update();
	reslice->Update();
	m_result = reslice->GetOutput();

	//std::cout << (warp->GetInverseFlag()==0 ? "plain" : "inverse") <<"\n";
}

vtkSmartPointer<vtkImageData> ImageWarp::
	getResult()
{
	return m_result;
}


//=============================================================================
//	ImageLoad
//=============================================================================
ImageLoad::ImageLoad( const char* c_filename )
	: m_isLoaded(false)
{
	using namespace std;

	// Get file extension (abort if none found)
	string ext = getExtension( c_filename );
	if( ext.empty() )
		return;

	// Try to load image
	vtkSmartPointer<vtkImageData> img;
	if( ext == "MHD" || ext == "MHA" )
	{
		// MetaImage format
		VTKNEW( vtkMetaImageReader, reader )
		reader->SetFileName( c_filename );
		reader->Update();
		img = reader->GetOutput();
	}
	else
	if( ext == "VTI" )
	{
		// VTK format
		VTKNEW( vtkXMLImageDataReader, reader )
		reader->SetFileName( c_filename );
		reader->Update();
		img = reader->GetOutput();
	}
	else
	{
		// Abort if format unknown
		cerr << "Error: Unsupported image format " << ext << "!\n";
		return;
	}

	// Check if image was succesfully read form disk
	if( !img )
	{
		cerr << "Error: Fatal error on loading image " << c_filename << "!\n";
		return;
	}
	int extent[6];
	img->GetExtent( extent );
	if( extent[1] - extent[0] <= 0 )
	{
		cerr << "Error: Failed to load image " << c_filename << "!\n";
		return;
	}

	// Set result
	m_result = img;
	m_isLoaded = true;
}



//=============================================================================
//	ImageSave
//=============================================================================
ImageSave::ImageSave( vtkImageData* img, const char* c_filename, 
	                  bool compress )
	: m_isSaved(false)
{
	using namespace std;

	// Get file extension (abort if none found)
	string ext = getExtension( c_filename );
	if( ext.empty() )
	{
		// If no extension is given use MHD as default
		ext = "MHD";
		return;
	}

	// Try to load image
	if( ext == "MHD" || ext == "MHA" )
	{
		// MetaImage format
		VTKNEW( vtkMetaImageWriter, writer )
		writer->SetFileName( c_filename );
		writer->SetCompression( compress );
		writer->SetInput( img );
		writer->Write();
	}
	else
	if( ext == "VTI" )
	{
		// VTK format
		VTKNEW( vtkXMLImageDataWriter, writer )
		writer->SetFileName( c_filename );
		writer->SetInput( img );
		writer->Write();
	}
	else
	{
		// Abort if format unknown (should not happen with default extension)
		cerr << "Error: Unsupported image format " << ext << "!\n";
		return;
	}

	m_isSaved = true;
}
