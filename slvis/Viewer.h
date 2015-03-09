#ifndef VIEWER_H
#define VIEWER_H

#include "glbase.h" // alternatively include <GL/GLConfig.h>

#include <QGLViewer/qglviewer.h>
#include <QList>

#include "PointSamples.h"
#include "StreamlineRenderer.h"
#include "VolumeTextureManager.h"

class GLSLProgram;
class QAction;

/**
	Stand alone viewer widget for prototyping streamline rendering.	
*/
class Viewer : public QGLViewer
{
	Q_OBJECT
	
public:
	Viewer( QWidget* parent=0 );

	QList<QAction*> getActions() { return m_actions; }

public slots:
	void destroyGL();
	void reloadShaders();
	
	void loadTemplate();
	void loadDeformation();	
	void loadSeedPoints();

	bool loadTemplate( QString filename );
	bool loadDeformation( QString filename );
	bool loadSeedPoints( QString filename );

protected:
	/// Load MHD volume from disk, returns NULL on error otherwise returns 
	/// texture pointer of texture already added to texture manager and uploaded
	/// to GPU.
	GL::GLTexture* loadVolume( QString filename );

	///@{ QGLViewer implementation
	void draw();
	void init();
	QString helpString() const;
	///@}

	/// Set base dir shown in file dialogs, usually called by last loadXXX().
	void setBaseDir( QString filename );

	void updateBoundingBox();

private:
	QList<QAction*> m_actions;
	StreamlineRenderer   m_slr;
	VolumeTextureManager m_vtm;
	PointSamples         m_seed;
	QString m_baseDir;

	QAction* m_actEnableShader;
};

#endif // VIEWER_H
