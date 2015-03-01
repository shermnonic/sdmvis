#include "VarVisWidget.h"
#include "VarVisRender.h"
#include "VolumeControls.h"

#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkMatrix4x4.h>
#include <vtkMatrix3x3.h>
#include <vtkPlane.h>
#include <vtkPointPicker.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkPlaneSource.h>
#include <vtkLineSource.h>
#include <vtkImplicitPlaneWidget.h>
VarVisWidget::VarVisWidget()
{
	left  =vtkSmartPointer<vtkActor>  ::New();
	right =vtkSmartPointer<vtkActor>  ::New();
	up    =vtkSmartPointer<vtkActor>  ::New();
	down  =vtkSmartPointer<vtkActor>  ::New();
}

void VarVisWidget::keyPressEvent(QKeyEvent *e)
{
	if (m_renderS->getKeyPressEventsEnabled())
	{
		
		if (e->key()==Qt::Key_Left) // show original Volume / Mesh
		{
			m_renderS->getText()->SetVisibility(true);
			if (!m_renderS->getShowVolumeMeshes())
			{
				m_renderS->getAnalyseVolumen().at(0)->SetVisibility(true);
				m_renderS->getAnalyseVolumen().at(1)->SetVisibility(false);
				if (m_renderS->getDifferenceVolumePresense())
					m_renderS->getAnalyseVolumen().at(2)->SetVisibility(false);
				
			}
			else
			{
			m_renderS->getAnalyseMeshActor().at(0)->SetVisibility(true);
			m_renderS->getAnalyseMeshActor().at(1)->SetVisibility(false);
			if (m_renderS->getDifferenceVolumePresense())
				m_renderS->getAnalyseMeshActor().at(2)->SetVisibility(false);
			
			}
			m_renderS->setIndexOfVolume(0);
			m_volumeControls->setCheckBoxes(true,false,false);
		}
		if (e->key()==Qt::Key_Down && m_renderS->getDifferenceVolumePresense()) //show Difference Volume/ Mesh
		{
			m_volumeControls->setCheckBoxes(false,false,true);	
			m_renderS->getText()->SetVisibility(true);
			if (!m_renderS->getShowVolumeMeshes())
			{
				m_renderS->getAnalyseVolumen().at(0)->SetVisibility(false);
				m_renderS->getAnalyseVolumen().at(1)->SetVisibility(false);
				if (m_renderS->getDifferenceVolumePresense())
					m_renderS->getAnalyseVolumen().at(2)->SetVisibility(true);
					
			}
			else
			{
				m_renderS->getAnalyseMeshActor().at(0)->SetVisibility(false);
				m_renderS->getAnalyseMeshActor().at(1)->SetVisibility(false);
				if (m_renderS->getDifferenceVolumePresense())
				{
					m_renderS->getAnalyseMeshActor().at(2)->SetVisibility(true);
					
					m_volumeControls->setCheckBoxes(false,false,true);			
				}
			
			}
			m_renderS->setIndexOfVolume(2);
			
		}
		if (e->key()==Qt::Key_Right)  // Show Registed Volume / Mesh
		{
			m_volumeControls->setCheckBoxes(false,true,false);	
			m_renderS->getText()->SetVisibility(true);
			if (!m_renderS->getShowVolumeMeshes())
			{
				m_renderS->getAnalyseVolumen().at(0)->SetVisibility(false);
				m_renderS->getAnalyseVolumen().at(1)->SetVisibility(true);
					if (m_renderS->getDifferenceVolumePresense())
					m_renderS->getAnalyseVolumen().at(2)->SetVisibility(false);
			}
			else
			{
			m_renderS->getAnalyseMeshActor().at(0)->SetVisibility(false);
			m_renderS->getAnalyseMeshActor().at(1)->SetVisibility(true);
			m_renderS->getAnalyseMeshActor().at(1)->GetProperty()->SetOpacity(1);
			if (m_renderS->getDifferenceVolumePresense())
				m_renderS->getAnalyseMeshActor().at(2)->SetVisibility(false);
			
			}
			m_renderS->setIndexOfVolume(1);
		}
		if (e->key()==Qt::Key_Up) // turn all OFF 
		{
			m_volumeControls->setCheckBoxes(false,false,false);	
			m_renderS->getText()->SetVisibility(false);
			if (!m_renderS->getShowVolumeMeshes())
			{
				m_renderS->getAnalyseVolumen().at(0)->SetVisibility(false);
				m_renderS->getAnalyseVolumen().at(1)->SetVisibility(false);
				if (m_renderS->getDifferenceVolumePresense())
					m_renderS->getAnalyseVolumen().at(2)->SetVisibility(false);
			}
			else
			{
				m_renderS->getAnalyseMeshActor().at(0)->SetVisibility(false);
				m_renderS->getAnalyseMeshActor().at(1)->SetVisibility(false);
				if (m_renderS->getDifferenceVolumePresense())
					m_renderS->getAnalyseMeshActor().at(2)->SetVisibility(false);
				
			}
		}
		
		m_renderS->getRenderWindow()->Render();
	}
}

void VarVisWidget::mousePressEvent(QMouseEvent* e)
{

  // Emit a mouse press event for anyone who might be interested
  emit mouseEvent(e);
  Qt::KeyboardModifiers somemodefier=e->modifiers();
	if (e->modifiers() == Qt::ControlModifier && e->buttons()== Qt::LeftButton) // Control  key pressed 
	{
		// create Plane	
	
		if(!(m_renderS->getROI().empty()))
		{

			double * sphereCenter=m_renderS->getROI().at(m_renderS->getRoiIndex())->GetCenter();
			m_renderS->setCenter(sphereCenter[0],sphereCenter[1],sphereCenter[2]);
			
			double camPosition[3];
			m_renderS->getRenderer()->GetActiveCamera()->GetPosition(camPosition);

			double distanceVector[3];
			double XVector[3];
			double YVector[3];
			double transformedXVector[3];
			double transformedYVector[3];


			/*m_renderS->m_center[0]=0;
			m_renderS->m_center[1]=0;
			m_renderS->m_center[2]=0; */
			// distance camera to object
			double * aktual_center= m_renderS->getCenter();
			distanceVector[0]=aktual_center[0]-camPosition[0];
			distanceVector[1]=aktual_center[1]-camPosition[1];
			distanceVector[2]=aktual_center[2]-camPosition[2];

			XVector[0]=1.f;
			XVector[1]=0.f;
			XVector[2]=0.f;

			YVector[0]=0.f;
			YVector[1]=1.f;
			YVector[2]=0.f;

			vtkCamera* cam = m_renderS->getRenderer()->GetActiveCamera();
//			cam->UseHorizontalViewAngleOff();
			// get rotation part of modelview matrix
			vtkMatrix4x4 * camMatrix = cam->GetViewTransformMatrix();
			vtkMatrix3x3 * R = vtkMatrix3x3::New();
			for(int i=0;i<3;++i)
				for(int j=0;j<3;++j)
					R->SetElement(i,j, camMatrix->GetElement(i,j) );
			
			// inverse rotation
			vtkMatrix3x3 * Rinv = vtkMatrix3x3::New();
			vtkMatrix3x3::Invert( R, Rinv );

			// x-axis in view space (always pointing right on display)
			Rinv->MultiplyPoint( XVector, transformedXVector );
			Rinv->MultiplyPoint( YVector, transformedYVector );

			// estimate center of viewport in depth of object
			// (simply project distance vector onto view normal)
		  double viewNormal[3], pointLeft[3],pointMid[3],pointRight[3],pointUp[3],
				   dist = 0;
			cam->GetViewPlaneNormal( viewNormal );
			for( int i=0; i < 3; ++i ) {
				dist += distanceVector[i]*viewNormal[i];
			}

			// compute width of frustum in specific depth
		  
//		  cam->UseHorizontalViewAngleOn();
		  double horizontalViewAngel= cam->GetViewAngle();

		  double halfwidth  = dist * tan( horizontalViewAngel / 2 );
		  // create scaleFactor
		  double scaleHeight;
		  int *windowSize= (m_renderS->getRenderWindow()->GetSize());
		  scaleHeight=(double)windowSize[1]/windowSize[0];
		 
		  // create points for line

			for( int i=0; i < 3; ++i ) 
			{
				pointLeft[i]  = camPosition[i] + dist*viewNormal[i] - 0.5*0.6*halfwidth*transformedXVector[i];
				pointMid[i]   = camPosition[i] + dist*viewNormal[i];
				pointRight[i] = camPosition[i] + dist*viewNormal[i]+ 0.5*0.6*halfwidth*transformedXVector[i] ;
				pointUp[i]    = camPosition[i] + dist*viewNormal[i]+0.5*0.6*scaleHeight*halfwidth*transformedYVector[i];
			}


		// calculate new aufpunkt with vectors
			double tempAufpunkt[3];
			for( int i=0; i < 3; ++i ) 
			{
				tempAufpunkt[i]=pointMid[i]+(pointLeft[i]-pointMid[i])+(pointUp[i]-pointMid[i]);
			}
			m_renderS->setAufpunkt(tempAufpunkt[0],tempAufpunkt[1],tempAufpunkt[2]);

		  // calculate width  Vector
		   double widthVector[3];
		   for( int i=0; i < 3; ++i )
			   widthVector[i]=pointRight[i]-pointLeft[i];
		   m_renderS->setWidthVector(widthVector[0],widthVector[1],widthVector[2]);
		  

		  // calculate height  Vector
		   double heightVector[3];
		   for( int i=0; i < 3; ++i )
			   heightVector[i]=2*(pointMid[i]-pointUp[i]);
			
		   m_renderS->setHeightVector(heightVector[0],heightVector[1],heightVector[2]);


		 
		  
		  vtkSmartPointer< vtkLineSource> aLine = vtkSmartPointer<vtkLineSource>::New();
		  aLine->SetPoint1( pointLeft);
		  aLine->SetPoint2( pointMid );
		  vtkSmartPointer<vtkPolyData> plane = aLine->GetOutput();

		  vtkSmartPointer<vtkPolyDataMapper> mapper =
			vtkSmartPointer<vtkPolyDataMapper>::New();
			mapper->SetInput(plane);
			
			
			left->SetMapper(mapper);
			left->GetProperty()->SetColor(1,0,0);

			 


		  vtkSmartPointer< vtkLineSource> aLine2 = vtkSmartPointer<vtkLineSource>::New();
		  aLine2->SetPoint1( pointMid);
		  aLine2->SetPoint2( pointRight );
		  vtkSmartPointer<vtkPolyData> plane2 = aLine2->GetOutput();
		  vtkSmartPointer<vtkPolyDataMapper> mapper2 =
		  vtkSmartPointer<vtkPolyDataMapper>::New();
		  mapper2->SetInput(plane2);

		  
		  right->SetMapper(mapper2);
		  

		  vtkSmartPointer< vtkLineSource> aLine3 = vtkSmartPointer<vtkLineSource>::New();
		  aLine3->SetPoint1( pointMid);
		  aLine3->SetPoint2( pointUp );
		  vtkSmartPointer<vtkPolyData> plane3 = aLine3->GetOutput();
		  vtkSmartPointer<vtkPolyDataMapper> mapper3 =
		  vtkSmartPointer<vtkPolyDataMapper>::New();
		  mapper3->SetInput(plane3);

		 up->SetMapper(mapper3);
		 up->GetProperty()->SetColor(0,1,0);

		  vtkSmartPointer< vtkLineSource> aLine4 = vtkSmartPointer<vtkLineSource>::New();
		  aLine4->SetPoint1( pointUp);
		  aLine4->SetPoint2( m_renderS->getAufpunkt() );
		  vtkSmartPointer<vtkPolyData> plane4 = aLine4->GetOutput();
		  vtkSmartPointer<vtkPolyDataMapper> mapper4 =
		  vtkSmartPointer<vtkPolyDataMapper>::New();
		  mapper4->SetInput(plane4);

		  down->SetMapper(mapper4);
		  down->GetProperty()->SetColor(0,0,1);




		  m_renderS->getRenderer()->AddActor(left);
		  m_renderS->getRenderer()->AddActor(right);
		  m_renderS->getRenderer()->AddActor(up);
		  m_renderS->getRenderer()->AddActor(down);

		  left->SetVisibility(false);
		  up->SetVisibility(false);
		  down->SetVisibility(false);
		  right->SetVisibility(false);

		//   std::cout<<"horizontalViewAngel: " << horizontalViewAngel<< " verticalViewAngel"<< viewAngle<<std::endl;

			double faktorx= (double)e->x()/this->width();
			double faktory= (double)e->y()/this->height();
			double center[3];
			

			center[0]=tempAufpunkt[0]+faktorx*widthVector[0]+faktory*heightVector[0];
			center[1]=tempAufpunkt[1]+faktorx*widthVector[1]+faktory*heightVector[1];
			center[2]=tempAufpunkt[2]+faktorx*widthVector[2]+faktory*heightVector[2];

			m_renderS->getROI().at(m_renderS->getRoiIndex())->SetCenter(center[0], center[1], center[2]);
			m_renderS->getROI().at(m_renderS->getRoiIndex())->Update();
			m_renderS->getRenderWindow()->Render();
			
		}
		return;
	}
  vtkRenderWindowInteractor* iren = NULL;
  if(this->mRenWin)
    {
    iren = this->mRenWin->GetInteractor();
    }
  
  if(!iren || !iren->GetEnabled())
    {
    return;
    }

 if (m_renderS->getSilhouetteVisible())
  {
	for (unsigned iA=0;iA<m_renderS->getSilhActors().size();iA++)
		m_renderS->getSilhActors()[iA]->SetVisibility(false);
	m_renderS->getSilhouetteActor()->SetVisibility(false);
	
	update();
  }

  // give interactor the event information
  iren->SetEventInformationFlipY(e->x(), e->y(), 
                             // (e->modifiers() & Qt::ControlModifier) > 0 ? 1 : 0, 
                              (e->modifiers() & Qt::ShiftModifier ) > 0 ? 1 : 0,
                              0,
                              e->type() == QEvent::MouseButtonDblClick ? 1 : 0);

  // invoke appropriate vtk event
  switch(e->button())
    {
    case Qt::LeftButton:
      iren->InvokeEvent(vtkCommand::LeftButtonPressEvent, e);
      break;

    case Qt::MidButton:
      iren->InvokeEvent(vtkCommand::MiddleButtonPressEvent, e);
      break;

    case Qt::RightButton:
      iren->InvokeEvent(vtkCommand::RightButtonPressEvent, e);
      break;

    default:
      break;
    }
}

void VarVisWidget::wheelEvent(QWheelEvent* e)
{

if (e->modifiers() == Qt::ControlModifier ) // Control key pressed 
	{

		if(e->delta() > 0)
		{
			if ( !(m_renderS->getROI().empty()) &&
				m_renderS->getROI().at(m_renderS->getRoiIndex())->GetRadius()<100 )
			{
				m_renderS->getROI().at(m_renderS->getRoiIndex())->SetRadius(
						m_renderS->getROI().at(m_renderS->getRoiIndex())->GetRadius()+5);
					
			
			}
				

		}
		else
		{
			if ( !(m_renderS->getROI().empty())&& 
				   m_renderS->getROI().at(m_renderS->getRoiIndex())->GetRadius()>6 )
			{

				m_renderS->getROI().at(m_renderS->getRoiIndex())->SetRadius(
						m_renderS->getROI().at(m_renderS->getRoiIndex())->GetRadius()-5);
					
			
			}
				

		}
			m_renderS->getROI().at(m_renderS->getRoiIndex())->Update();
			m_renderS->getRenderWindow()->Render();
		return;
	}


  vtkRenderWindowInteractor* iren = NULL;
  if(this->mRenWin)
    {
    iren = this->mRenWin->GetInteractor();
    }
  
  if(!iren || !iren->GetEnabled())
    {
    return;
    }

// VTK supports wheel mouse events only in version 4.5 or greater
  // give event information to interactor
  iren->SetEventInformationFlipY(e->x(), e->y(), 
                             //(e->modifiers() & Qt::ControlModifier) > 0 ? 1 : 0, 
                             (e->modifiers() & Qt::ShiftModifier ) > 0 ? 1 : 0);
  
  // invoke vtk event
  // if delta is positive, it is a forward wheel event
  if(e->delta() > 0)
    {
    iren->InvokeEvent(vtkCommand::MouseWheelForwardEvent, e);
    }
  else
    {
    iren->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, e);
    }
}
void VarVisWidget::mouseMoveEvent(QMouseEvent* e)
{
	if (e->modifiers() == Qt::ControlModifier && e->buttons()== Qt::LeftButton) // alt key pressed 
	{

		if ( !(m_renderS->getROI().empty()) )
		{
			double faktorx= (double)e->x()/this->width();
			double faktory= (double)e->y()/this->height();
			double center[3];
			double *tempAufpunkt=m_renderS->getAufpunkt();
			double *widthVector=m_renderS->getWidthVector();
			double *heightVector=m_renderS->getHeightVector();


			center[0]=tempAufpunkt[0]+faktorx*widthVector[0]+faktory*heightVector[0];
			center[1]=tempAufpunkt[1]+faktorx*widthVector[1]+faktory*heightVector[1];
			center[2]=tempAufpunkt[2]+faktorx*widthVector[2]+faktory*heightVector[2];

			m_renderS->getROI().at(m_renderS->getRoiIndex())->SetCenter(center[0], center[1], center[2]);
			m_renderS->getROI().at(m_renderS->getRoiIndex())->Update();

			m_renderS->getRenderWindow()->Render();
		}
		return;
	}

  vtkRenderWindowInteractor* iren = NULL;
  if(this->mRenWin)
    {
    iren = this->mRenWin->GetInteractor();
    }
  
  if(!iren || !iren->GetEnabled())
    {
    return;
    }
  
  // give interactor the event information
  iren->SetEventInformationFlipY(e->x(), e->y(), 
                             //(e->modifiers() & Qt::ControlModifier) > 0 ? 1 : 0, 
                             (e->modifiers() & Qt::ShiftModifier ) > 0 ? 1 : 0);
  
  // invoke vtk event
  iren->InvokeEvent(vtkCommand::MouseMoveEvent, e);
	
 
}





void VarVisWidget::mouseReleaseEvent(QMouseEvent* e)
{

	oldX=e->x();
	oldY=e->y();

	vtkRenderWindowInteractor* iren = NULL;
  if(this->mRenWin)
    {
    iren = this->mRenWin->GetInteractor();
    }
  
  if(!iren || !iren->GetEnabled())
    {
    return;
    }
  
  // give vtk event information
  iren->SetEventInformationFlipY(e->x(), e->y(), 
                             (e->modifiers() & Qt::ControlModifier) > 0 ? 1 : 0, 
                             (e->modifiers() & Qt::ShiftModifier ) > 0 ? 1 : 0);
  
  // invoke appropriate vtk event
  switch(e->button())
    {
    case Qt::LeftButton:
      iren->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, e);
      break;

    case Qt::MidButton:
      iren->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent, e);
      break;

    case Qt::RightButton:
      iren->InvokeEvent(vtkCommand::RightButtonReleaseEvent, e);
      break;

    default:
      break;
    }
  
  if (m_renderS->getSilhouetteVisible())
  {
	for (unsigned iA=0;iA<m_renderS->getSilhActors().size();iA++)
		m_renderS->getSilhActors()[iA]->SetVisibility(true);
	m_renderS->getSilhouetteActor()->SetVisibility(true);
	
	update();
  }

}








