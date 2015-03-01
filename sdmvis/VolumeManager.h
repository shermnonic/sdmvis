#ifndef VOLUMEMANAGER_H
#define VOLUMEMANAGER_H

#include <vector>
#include <string>
#include <GL/GLTexture.h>

class VolumeDataHeader;

/// \todo function to download \a Volume to GPU memory after \a loadVolume()
///       (load and upload could be functions of \a Volume class itself)
class VolumeManager
{
	static int s_verbosity;

public:
	class Volume;

	/// Bit combinable options for \a loadVolume()
	enum Options {
		DownloadToGPU   = 1,
		ReleaseCPUMemory= 2
	};

	/// Load volume from disk and download to GPU memory (needs GL context)
	bool loadVolume( std::string mhdFilename, 
	                 int opts= (int)DownloadToGPU | (int)ReleaseCPUMemory );

	bool setVolume( std::string idName, 
					VolumeDataHeader* volume, bool takeOwnerShip=true,
					int opts= (int)DownloadToGPU | (int)ReleaseCPUMemory );

	/// Release all textures, free all memory
	void clear();

	Volume getVolume( std::string fname ) const { return get_volume(fname); }

	/// Volume handle with header information, data pointer and GL texture
	/// \todo destroy GLTexture inside GL context
	class Volume
	{
	private:
		bool m_downloaded_to_gpu;
		bool m_delete_volume;

		std::string       m_filename; ///< originally used filename to load volume,
		                              ///< used as unique identifier, do not change!
		VolumeDataHeader* m_volume;
		void*             m_dataptr;  ///< may be NULL, type and size as in \a volume
		GL::GLTexture*    m_texture;  ///< volume texture if downloaded to GPU

	public:
		bool is_in_cpu_memory    () const { return m_dataptr!=NULL; }
		bool is_downloaded_to_gpu() const { return m_downloaded_to_gpu; }

		/// Return original filename which is a unique identifier here
		std::string             filename() const { return m_filename; }
		const VolumeDataHeader* volume()   const { return m_volume; }
		/// Returns volume texture if downloaded to GPU, else texid is invalid
		//GL::GLTexture           texture()  const { return *m_texture; }
		
		/// Returns volume texture if downloaded to GPU, else texid is invalid
		GL::GLTexture*          texture() { return m_texture; }
		/// Return raw volume data pointer, size and element type as specified in \a volume
		void*                   dataptr() { return m_dataptr; }
		
		// Implemenation remarks:
		// - default copy c'tor should be ok for VolumeDataHeader*
		// - should be safe to use in STL containers
		// - GLTexture really save to use in STL containers??

		Volume( std::string fname )
			: m_downloaded_to_gpu(false),
			  m_delete_volume(true),
			  m_filename(fname),
			  m_volume(NULL),
			  m_dataptr(NULL),
			  m_texture(NULL)
			{}

		~Volume();

		bool empty() { return m_volume==NULL; }

		bool set( VolumeDataHeader* volume, bool takeOwnerShip=true );
		bool load();
		bool download();

		void clear_cpu();
		void clear();
	};


	bool   contains_volume( std::string fname ) const;

	
	size_t size() const { return m_vols.size(); }
	Volume getVolume( int i ) const 
	{ 
		assert( i >= 0 && i < (int)m_vols.size() );
		return m_vols[i]; 
	}


protected:
	Volume get_volume( std::string fname ) const;
	bool   add_volume( Volume warp, int opts );

private:
	typedef std::vector<Volume> VolumeArray;
	VolumeArray m_vols;
};

#endif // VOLUMEMANAGER_H
