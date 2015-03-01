#ifndef VTKCAMERASERIALIZER_H
#define VTKCAMERASERIALIZER_H

class vtkCamera;

bool saveVTKCamera( vtkCamera* camera, const char* filename );
bool loadVTKCamera( vtkCamera* camera, const char* filename );

#endif // VTKCAMERASERIALIZER_H
