#include "VolumeManager.h"
#include <VolumeRendering/VolumeData.h>
#include <VolumeRendering/VolumeUtils.h>  // load_volume(), create_volume_tex()
#include <iostream>

using namespace std;

int VolumeManager::s_verbosity = 1;

//==============================================================================
//	VolumeManager :: Volume
//==============================================================================
VolumeManager::Volume::~Volume()
{
	/*
	clear();
	if( m_volume && m_delete_volume ) 
		delete m_volume;
	*/
}

void VolumeManager::Volume::clear_cpu()
{
	if( m_volume )
	{
		if( m_delete_volume )
			m_volume->clear();
		m_dataptr = NULL;
	}
}

void VolumeManager::Volume::clear()
{
	clear_cpu();
	if( is_downloaded_to_gpu() )
	{
		// correct to destroy texture here?
		m_texture->Destroy();
		delete m_texture; m_texture=NULL;
	}
	// FIXME: We should delete the header as well?!
	// if( m_delete_volume ) delete m_volume;
}

bool VolumeManager::Volume::load()
{
	// load volume from disk
	m_volume = load_volume( m_filename.c_str(), s_verbosity, &m_dataptr );
	return (m_dataptr != NULL);
}

bool VolumeManager::Volume::set( VolumeDataHeader* volume, bool takeOwnerShip )
{
	m_volume = volume;
	m_dataptr = m_volume->rawPtr();
	m_delete_volume = takeOwnerShip;
	return (m_dataptr != NULL);
}

bool VolumeManager::Volume::download()
{
	// create texture if not existant
	if( !m_texture )
	{
		m_texture = new GL::GLTexture();
	}

	// download 3D texture
	m_downloaded_to_gpu 
		= create_volume_tex( *m_texture, m_volume, m_dataptr, s_verbosity );
	return m_downloaded_to_gpu;
}

//==============================================================================
//	VolumeManager
//==============================================================================

void VolumeManager::clear()
{
	for( size_t i=0; i < m_vols.size(); ++i )
		m_vols.at(i).clear();
	m_vols.clear();
}

bool VolumeManager::contains_volume( std::string fname ) const
{
	for( VolumeArray::const_iterator it=m_vols.begin(); it!=m_vols.end(); ++it )
	{
		Volume w = *it;
		// return first match
		if( w.filename() == fname )
			return true;
	}
	return false;
}

VolumeManager::Volume VolumeManager::get_volume( std::string fname ) const
{
	static Volume emptyVolume("");

	for( VolumeArray::const_iterator it=m_vols.begin(); it!=m_vols.end(); ++it )
	{
		Volume w = *it;
		// return first match
		if( w.filename() == fname )
			return w;
	}
	// return empty vol if not found (SHOULD NOT HAPPEN!)
	assert(false);
	return emptyVolume;
}

bool VolumeManager::add_volume( Volume vol, int opts )
{
	// check if vol can be added (duplicate identifier filename?)
	if( contains_volume(vol.filename()) )
	{
		cerr << "VolumeManager::loadVolume : Could not set volume, because a "
			    "volume with the same name is already present!" << endl;
		return false;
	}

	// download?
	if( opts & (int)DownloadToGPU )
	{
		// download 3D texture
		if( !vol.download() )
		{
			// on failure clear also memory
			cerr << "VolumeManager::loadVolume : Could not download volume data"
				    " to GPU!" << endl;
			vol.clear();
			return false;
		}
	}

	// free CPU memory?
	if( opts & (int)ReleaseCPUMemory )
	{
		vol.clear_cpu();
	}

	m_vols.push_back( vol );
	return true;
}

bool VolumeManager::loadVolume( std::string mhdFilename, int opts )
{
	int verbosity = 3;

	Volume w( mhdFilename );

	// load volume from disk
	if( !w.load() )
	{
		cerr << "VolumeManager::loadVolume : Could not load volume " 
			 << mhdFilename << endl;
		return false;
	}

	// add to array of volumes
	if( !add_volume( w, opts ) )
	{
		// this should not happen
		cerr << "VolumeManager::loadVolume : Fatal error loading volume!\n";
		w.clear();
		return false;
	}
	return true;
}

bool VolumeManager::setVolume( std::string idName, VolumeDataHeader* volume, 
							   bool takeOwnerShip, int opts )
{
	Volume w( idName );

	if( contains_volume(idName) )
	{
		cerr << "VolumeManager::setVolume : Could not set volume, because a "
			    "volume with the same name is already present!" << endl;
		return false;		
	}

	if( !w.set(volume,takeOwnerShip) )
	{
		cerr << "VolumeManager::setVolume : Could not set volume " << idName << endl;
		return false;
	}

	// add to array of volumes
	if( !add_volume( w, opts ) )
	{
		// this should not happen
		cerr << "VolumeManager::setVolume : Fatal error loading volume!\n";
		w.clear();
		return false;
	}

	return true;
}