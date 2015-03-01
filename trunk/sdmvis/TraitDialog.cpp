#include "TraitDialog.h"
#include <QtGui>
#include <fstream>
#include <vector>
#ifdef SDMVIS_MATLAB_ENABLED
#include "Matlab.h"
#endif

  #define GET_FILE_SIZE( size, fs ) \
	fs.seekg( 0, ios::end );      \
	size = (size_t)fs.tellg();    \
	fs.seekg( 0, ios::beg );

// helper class
class ComboDelegate : public QAbstractItemDelegate
{
public:
	ComboDelegate( QStringList comboItems, int restrictoToColumn, QObject* parent=0 );

	void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
	QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;
	QWidget* createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
	void setEditorData( QWidget* editor, const QModelIndex& index ) const;
	void setModelData( QWidget* editor, QAbstractItemModel* model, const QModelIndex& index ) const;
	void updateEditorGeometry( QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index ) const;

protected:
	QStringList m_comboItems;
	int m_restrictToColumn;
};

int loadLabelsFromTextfile( std::vector<int>& labels, const char* filename )
{
	using namespace std;
	ifstream f( filename );
	if( !f.is_open() )
	{
		cerr << "Error: Could not open labels textfile \"" << filename << "\"!" << endl;
		return -1;
	}

	while( f.good() )
	{
		string line;
		getline( f, line );

		int val = atoi( line.c_str() );
		labels.push_back( val );		
	}

	f.close();
	return 1; // success
}

//------------------------------------------------------------------------------
//	TraitDialog
//------------------------------------------------------------------------------

TraitDialog::TraitDialog( QWidget* parent )
	: QDialog(parent)
{
	// --- model ----

	m_model = new QStandardItemModel(0,2);
	m_treeView = new QTreeView;
	
	m_groupDelegate = new ComboDelegate( 
		QStringList() << tr("Class A") << tr("Class B"), 1 );
	

	
	setNames( QStringList() );

	
	// --- actions ---

	QPushButton* butOpenNames    = new QPushButton(tr("Open names..."));
	QPushButton* butOpenMatrix   = new QPushButton(tr("Open eigenvector matrix..."));
	QPushButton* butLoadLabels   = new QPushButton(tr("Load labels..."));
	QPushButton* butComputeTrait = new QPushButton(tr("Compute trait..."));
	//butComputeTrait->setEnabled(false);
	QPushButton* butDefaultWeights = new QPushButton(tr("Default weights"));

	connect( butOpenNames   , SIGNAL(clicked()), this, SLOT(openNames()) );
	connect( butOpenMatrix  , SIGNAL(clicked()), this, SLOT(openMatrix()) );
	connect( butLoadLabels  , SIGNAL(clicked()), this, SLOT(loadLabels()) );
	connect( butComputeTrait, SIGNAL(clicked()), this, SLOT(computeTrait()) );
	connect(butDefaultWeights,SIGNAL(clicked()), this, SLOT(defaultWeights()) );

	// --- widgets ---

	// number of components
	m_ncompSpinBox = new QSpinBox;
	m_ncompSpinBox->setMinimum( 1 );
	m_ncompSpinBox->setMaximum( 10 ); // depends on number of names
	m_ncompSpinBox->setSingleStep( 1 );
	QLabel* ncompLabel = new QLabel(tr("Number of components"));
	ncompLabel->setBuddy( m_ncompSpinBox );
	QHBoxLayout* lncomp = new QHBoxLayout;
	lncomp->addWidget( ncompLabel );
	lncomp->addWidget( m_ncompSpinBox );

	// weights C,w1,w2	
	m_weightC  = new QDoubleSpinBox;
	m_weightW1 = new QDoubleSpinBox;
	m_weightW2 = new QDoubleSpinBox;
	QLabel* weightCLabel = new QLabel(tr("Weight C:"));
	QLabel* weightW1Label = new QLabel(tr("Class A weight w1:"));
	QLabel* weightW2Label = new QLabel(tr("Class B weight w2:"));
	QDoubleSpinBox* weights[3] = { m_weightC, m_weightW1, m_weightW2 };
	QLabel* weightLabels[3] = { weightCLabel, weightW1Label, weightW2Label };
	QGridLayout* lweights = new QGridLayout;
	for( int i=0; i < 3; ++i ) {
		weights[i]->setRange( 0.0, 1000.0 );
		weights[i]->setSingleStep( 0.1 );
		weights[i]->setValue( 1.0 );
		weightLabels[i]->setBuddy( weights[i] );
		lweights->addWidget( weightLabels[i], i,0 );
		lweights->addWidget( weights[i], i,1 );
	}
	lweights->addWidget( butDefaultWeights );

	// scale dimension w/ eigenvalues?
	m_lambdaScaling = new QCheckBox(tr("Lambda scaling"));
	m_lambdaScaling->setChecked( false );
	
	// additional options (scale, number of components)
	QVBoxLayout* lopt = new QVBoxLayout;
	lopt->addLayout( lncomp );
	lopt->addWidget( m_lambdaScaling );

	m_updateMatlab = new QCheckBox(tr("Update Matlab"));
	m_updateMatlab->setChecked( true );

	// --- layout ---

	// table layout
	QVBoxLayout* ltable = new QVBoxLayout;
	QLabel* tableLabel = new QLabel(tr("Dataset classification:"));
	ltable->addWidget( tableLabel );
	ltable->addWidget( m_treeView );

	QGroupBox* weightGroup = new QGroupBox(tr("C-SVM weights"));
	weightGroup->setLayout( lweights );

	QGroupBox* optGroup = new QGroupBox(tr("Options"));
	optGroup->setLayout( lopt );

	QVBoxLayout* lb = new QVBoxLayout;
	//lb->addWidget( butOpenNames ); // obsolete, names are provided by SDMVisMainWindow config
	lb->addWidget( butOpenMatrix );
	lb->addWidget( butLoadLabels );
	lb->addStretch( 1 );
	lb->addWidget( optGroup );
	lb->addWidget( weightGroup );
	lb->addStretch( 3 );
#ifdef SDMVIS_MATLAB_ENABLED
	lb->addWidget( m_updateMatlab );
#endif
	lb->addWidget( butComputeTrait );

#if 0
	// splitter layout
	QSplitter* splitter = new QSplitter( this );
	QWidget* controls = new QWidget;
	controls->setLayout( lb );
	splitter->addWidget( m_treeView );
	splitter->addWidget( controls );
	splitter->setStretchFactor( 0, 2 );
	splitter->setStretchFactor( 1, 1 );
	QHBoxLayout* layout = new QHBoxLayout;
	layout->addWidget( splitter );
	setLayout( layout );
#else	
	// fixed layout
	QHBoxLayout* layout = new QHBoxLayout;
	layout->addLayout( ltable );
	layout->addLayout( lb );
	setLayout( layout );
#endif
	
	setWindowTitle( tr("sdmvis - Trait dialog") );
	
	// set the window geometry to w=600; h=400 
	this->resize(600,400);
}

void TraitDialog::setNames( QStringList names )
{
	m_names = names;
	m_model->clear();
	QStringList numbers;
	for( int r=0; r < names.size(); ++r )
	{
		QStandardItem* itemName = new QStandardItem( names.at(r) );
		itemName->setEditable( false );

		// group item (text is ignored and overwritten by delegate?)
		QStandardItem* itemGroup = new QStandardItem( tr("(Class assigned here)") );
		itemGroup->setEditable( true );
		
	
			
		m_model->setItem( r, 0, itemName );
		m_model->setItem( r, 1, itemGroup );
		

		numbers << QString::number(r+1);
	}

	QStringList headers;
	headers << tr("Name") << tr("Group");
	m_model->setHorizontalHeaderLabels( headers );
	m_model->setVerticalHeaderLabels( numbers );
	
	

	m_treeView->setModel( m_model );
	m_treeView->setItemDelegateForColumn( 1, m_groupDelegate );
	
	if( names.size() > 0 ) {
		m_ncompSpinBox->setMaximum( names.size() );
		m_ncompSpinBox->setValue( names.size()-1 );
	}
	
	m_treeView->resizeColumnToContents( 0 );
}

void TraitDialog::openNames()
{
	using namespace std;

	// get filename
	QString filename = QFileDialog::getOpenFileName( this, tr("Open names..."),
		m_baseDir, tr("Textfile (*.txt)") );	
	if( filename.isEmpty() )
		return;

	// open textfile
	ifstream f( filename.toAscii() );
	if( !f.is_open() ) {
		QMessageBox::warning( this, tr("sdmvis: Error opening names"),
			tr("Could not open %1").arg(filename) );
		return;
	}

	// read names (line by line)
	QStringList names;
	while( f.good() )
	{
		string line;
		getline( f, line );
		names << QString::fromStdString(line);
	}
	f.close();

	setNames( names );
}

void TraitDialog::loadLabels()
{
	using namespace std;

	// get filename
	QString filename = QFileDialog::getOpenFileName( this, 
		tr("Open labels..."),
		m_baseDir, tr("Labels textfile (*.txt)") );
	if( filename.isEmpty() )
		return;

	// load labels
	vector<int> labels;
	if( !loadLabelsFromTextfile( labels, filename.toAscii() ) )	{
		QMessageBox::warning( this, tr("sdmvis: Error loading labels"),
			tr("Could not load labels from %1").arg(filename) );
		return;
	}
	if( labels.size() != m_model->rowCount() ) {
		QMessageBox::warning( this, tr("sdmvis: Error loading labels"),
			tr("Mistmatch of number of names and number of labels!") );
		return;
	}

	// assign labels
	for( int r=0; r < m_model->rowCount(); ++r )
	{
		// convert labels -1,+1 to group indices 0,1
		int group = labels.at(r) <= 0 ? 0 : 1;
		m_model->item(r, 1)->setData( group, Qt::DisplayRole );
	}
	m_treeView->update();
}

void TraitDialog::openMatrix()
{
	using namespace std;

	// get filename
	QString filename = QFileDialog::getOpenFileName( this, 
		tr("Open eigenvector matrix..."),
		m_baseDir, tr("Raw matrix (*.mat)") );	
	if( filename.isEmpty() )
		return;

	// open matrix
	ifstream f( filename.toAscii(), ios::binary );
	if( !f.is_open() ) {
		QMessageBox::warning( this, tr("sdmvis: Error opening matrix"),
			tr("Could not open %1").arg(filename) );
		return;
	}

	// guess dimensionality from number of names
	// (must have number of names columns and element type is float32)
	size_t size; GET_FILE_SIZE(size,f);
	size_t elementsize = 4; // float32 = 4 byte
	int nrows = m_model->rowCount(),
	    ncols = (size/elementsize) / nrows;
	if( (size/elementsize) % nrows != 0 ) {
		QMessageBox::warning( this, tr("sdmvis: Error matrix size mismatch"),
			tr("Matrix file hase %1 elements which does not match the number of"
			   "names %2!").arg(size/elementsize).arg(nrows) );
		f.close();
		return;
	}

	// load matrix
	float* buf = new float[nrows*ncols];
	f.read( (char*)buf, size );
	f.close();

	m_V.resize( nrows, ncols );
	rednum::matrix_from_rawbuffer<float,Matrix,double>( m_V, buf );

	cout << "Eigenvector matrix " << nrows << " x " << ncols << " set" << endl;
	delete [] buf;
}

void TraitDialog::defaultWeights()
{
	// FIXME: compute/verify weights via line search and cross correlation!!

	// get labels and group sizes (although labels are not needed here)
	std::vector<int> labels;
	int nA(0), nB(0);
	if( !getClassification( labels, nA, nB ) )
		return;

	// weights (100 / group size)
	double wA = 100. / (double)nA,
		   wB = 100. / (double)nB;

	m_weightC ->setValue( 1.0 );
	m_weightW1->setValue( wA );
	m_weightW2->setValue( wB );
}

bool TraitDialog::getClassification( std::vector<int>& labels, int& nA, int& nB )
{
	using namespace std;

	if( m_model->rowCount() <= 1 ) 
	{
		QMessageBox::warning(this,tr("sdmvis: Warning"),
			tr("Classification needs 2 or more datasets!"));
		return false;
	}

	labels.clear(); labels.resize( m_model->rowCount() );
	nA = 0;
	nB = 0;

	// get classification	
	cout << "Classification:" << endl << "---------------" << endl;
	for( int r=0; r < m_model->rowCount(); ++r )
	{
		int group = m_model->item(r, 1)->data( Qt::DisplayRole ).toInt();
				// 0 - group A
				// 1 - group B

		labels.at(r) = (group==0) ? -1 : +1;
		if( group==0 ) nA++; else nB++;
		
		QString name = m_model->item(r, 0)->data( Qt::DisplayRole ).toString();
		cout << r << ": group " << (group==0?"A":"B") << "  (" << name.toStdString() << ")" << endl;
	}

	// sanity check
	if( nA==0 || nB==0 ) {
		QMessageBox::warning(this,tr("sdmvis: Warning"),
			tr("Names are not assigned to different groups!"));
		return false;
	}

	return true;
}

void TraitDialog::computeTrait()
{
	using namespace std;

	// ------------------------ Setup problem----------------------------------

	int nrows = m_model->rowCount();
	vector<int> labels( nrows );
	int nA(0), nB(0); // group sizes

	// get labels and group sizes
	if( !getClassification( labels, nA, nB ) )
		return;

	// instance matrix
	if( m_V.size1() != nrows )	{
		QMessageBox::warning(this,tr("sdmvis: Warning"),
			tr("Mismatch number of names and number of columns in eigenvector matrix!"));
		return;
	}
	// reduce to selected number of components (ie columns)
	Matrix V_k = m_V;
	int ncols = m_ncompSpinBox->value(); // was: m_V.size2()
	assert( ncols>0 && ncols<=(int)m_V.size2() );
	V_k.resize( V_k.size1(), ncols );

	vector<double> data;
	rednum::matrix_to_stdvector<double,Matrix>( V_k, data );
	//~~~ double* data = rednum::matrix_to_rawbuffer<double,Matrix>( V_k );
	cout << "Reduced SVM instance matrix to " << ncols << " columns." << endl;

	// setup SVM
	m_svm.clear();

	// default parameters for our application
	svm_parameter param = m_svm.params();
	param.svm_type = C_SVC;
	param.kernel_type = LINEAR;
	m_svm.set_params( param );

	// user weights
	double wA = m_weightW1->value(),
		   wB = m_weightW2->value();
	m_svm.params().C = m_weightC->value();
	m_svm.add_weight( -1, wA );
	m_svm.add_weight( +1, wB );
	cout << "Weight C = " << m_svm.params().C << endl
		 << "Weight class -1 = " << wA << endl
		 << "Weight class +1 = " << wB << endl;		  

	m_svm.setup_problem( nrows, ncols, &labels[0], &data[0] );
	//~~~ delete [] data; data=NULL;

	// ------------------------ Compute trait ---------------------------------

	// cross validation
	double crossval = m_svm.cross_validation( 10 );
	cout << "SVM 10-fold cross validation error (not normalized) = " << crossval << endl;

	// train SVM
	cout << "Training SVM..." << endl;
	if( !m_svm.train() )
	{
		QMessageBox::warning(this,tr("sdmvis: Error in SVM training"),
			tr("Support Vector Machine training failed!\nError: %1")
			.arg( m_svm.getErrmsg().c_str() ));
		return;
	}

	// get model SV, coefficients
	Matrix SV( m_svm.nrows(), m_svm.ncols() );
	Vector sv_coef( m_svm.nrows() );
	rednum::matrix_from_rawbuffer<double,Matrix,double>( SV     , m_svm.sv() );
	rednum::vector_from_rawbuffer<double,Vector,double>( sv_coef, m_svm.sv_coef() );

	namespace ublas = boost::numeric::ublas;

	// compute normal vector w = SV'*sv_coef  and distance b
	Vector w = ublas::prod( ublas::trans(SV), sv_coef );
	double b = - m_svm.model()->rho[0];

	if( m_svm.model()->label[0] == -1 ) {
		// see libsvm faq for rationale here
		w *= -1.;
		b *= -1.;
	}

	// project w into column space of V
	Vector v_w = ublas::prod( V_k, w );          // FIXME: normalization?!

	
	// store members
	//m_svm_w = w;
	//m_svm_b = b;
	m_trait = v_w;

	// ------------------------ Save results ----------------------------------

	// save projector
	QString projector_filename = QFileDialog::getSaveFileName( this,
		tr("Save trait vector (projector)..."),
		m_baseDir, tr("Raw matrix (*.mat)") );
	if( !projector_filename.isEmpty() )
	{
		cout << "Saving " << v_w.size() << " x 1 V-space trait to \"" << projector_filename.toStdString() << "\"..." << endl;
		rednum::save_vector<float,Vector>( v_w, projector_filename.toAscii() );

		// auto-naming for further automatically generated files
		QString basename = projector_filename.left( projector_filename.size() - 4 );

		// save resulting SVM model	
		{
			QString model_filename = basename + ".svm";
			cout << "Saving resulting SVM model to \"" << model_filename.toStdString() << "\"..." << endl;
			m_svm.save_model( model_filename.toAscii() );
		}

		// save hyperplane normal and distance as raw vector 
		// (for eventual use in Matlab; last component is distance to origin)
		{
			QString normal_filename = basename + "-normal.mat";
			cout << "Saving hyperplane normal+distance to \"" << normal_filename.toStdString() << "\"..." << endl;
			Vector wb = w; 
			wb.resize( wb.size() + 1 );
			wb(wb.size()-1) = b;
			rednum::save_vector<float,Vector>( wb, normal_filename.toAscii() );
		}

		// save plain trait vector (padded by zeros to full eigenspace dimension)
		{
			QString w_filename = basename + "-w.mat";
			cout << "Saving plain trait vector to \"" << w_filename.toStdString() << "\"..." << endl;
			
			Vector wfull = w;			
			wfull.resize( nrows );
			for( int i=w.size(); i < nrows; ++i )
				wfull(i) = 0.0;

			cout << "w_full = ( " << endl;
			for( int i=0; i < wfull.size(); ++i )
				cout << wfull(i) << endl;
			cout << ")" << endl;

			rednum::save_vector<float,Vector>( wfull, w_filename.toAscii() );
		}
	}

	// ------------------------ Matlab plots ----------------------------------

#ifdef SDMVIS_MATLAB_ENABLED
	if( m_updateMatlab->isChecked() )
	{
		static Matlab g_matlab;
		static bool g_matlabRunning = false;

		QString names_filename("temp_names.txt");
		QString mdir( "..\\sdmvis\\matlab\\" ), // FIXME: hardcoded script directory
				odir( ".\\" ), // FIXME: hardcoded output directory (relative to mdir)
				tmpnames( mdir+odir+names_filename );

		// HACK: Write tmp_names.txt to be loaded in Matlab engine via loadNames()
		ofstream tmp( tmpnames.toAscii() );
		if( tmp.is_open() )
		{
			for( int i=0; i < m_names.size(); ++i )
				tmp << (m_names[i].toStdString()) << endl;
			tmp.close();
		}
		else
			cout << "Warning: Could not open \"" << tmpnames.toStdString() << "\"!" << endl;

		// Matlab init
		if( !g_matlab.ep ) {
			g_matlab.setSilent( false );
			if( g_matlab.init() ) {
				g_matlab.cd( mdir.toAscii() ); 
				g_matlabRunning = true;
			}
		}

		// Matlab plots
		if( g_matlabRunning )
		{
			double* raw_V_k = rednum::matrix_to_rawbuffer<double,Matrix>( V_k );
			double* raw_w   = rednum::vector_to_rawbuffer<double,Vector>( w );
			assert( raw_V_k );
			assert( raw_w );

			cout << "Putting variables to Matlab engine" << endl;

			QString tmpnamescmd = QString("names_filename='") + names_filename + QString("';\n"); 
			g_matlab.eval( tmpnamescmd.toAscii() );

			uploadToMatlab( g_matlab.ep, "w", w.size(),1,              raw_w );
			uploadToMatlab( g_matlab.ep, "V", V_k.size1(),V_k.size2(), raw_V_k );
			uploadToMatlab( g_matlab.ep, "b", 1,1,                     &b );
			uploadToMatlab( g_matlab.ep, "labels", labels.size(),1 ,   &labels[0] );

			////////////////////////////////////////////////////////////////////
			// TODO: 
			// - working directory? (for eventual figure and .mat outputs)
			////////////////////////////////////////////////////////////////////

			delete [] raw_V_k;
			delete [] raw_w;

			g_matlab.eval( (tr("[names_full names] = loadNames('%1');").arg("temp_names.txt")).toAscii() );
			g_matlab.eval( (tr("hint='%1';").arg("foo")).toAscii() );
			g_matlab.eval( (tr("outdir='%1';").arg(odir)).toAscii() );
			g_matlab.eval( "scriptSVMAnalysis" );
		}
	}
#endif // SDMVIS_MATLAB_ENABLED
}



//------------------------------------------------------------------------------
//	ComboDelegate
//------------------------------------------------------------------------------

ComboDelegate::ComboDelegate( QStringList comboItems, int restrictToColumn, QObject* parent )
	: QAbstractItemDelegate(parent),
	  m_comboItems(comboItems),
	  m_restrictToColumn(restrictToColumn)
{

}

void ComboDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
	if( index.column() == m_restrictToColumn )
	{
		int itemIdx = index.data().toInt();
		QString itemText("(Error: invalid item)");
		if( itemIdx>=0 && itemIdx < m_comboItems.size() )
			itemText = m_comboItems.at(itemIdx);

		QStyleOptionComboBox styleopt;
		styleopt.rect = option.rect;
		styleopt.currentText = itemText;
		

		QApplication::style()->drawControl( QStyle::CE_ComboBoxLabel, &styleopt, painter );

		}
	//else
	//	QStyledItemDelegate::paint( painter, option, index );
}

QSize ComboDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{	
	return QSize(42,20); //QSize(42,23); // ??
}

QWidget* ComboDelegate::createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
	QComboBox* combo = new QComboBox( parent );
	combo->addItems( m_comboItems );
	combo->installEventFilter( const_cast<ComboDelegate*>(this) );
	return combo;
}

void ComboDelegate::updateEditorGeometry( QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
	editor->setGeometry( option.rect );
}

void ComboDelegate::setEditorData( QWidget* editor, const QModelIndex& index ) const
{
	int value = index.model()->data( index, Qt::DisplayRole ).toInt();
	// clamp index
	int itemIdx = (value<=0) ? 0 : (value>=m_comboItems.size()) ? 0 : value;
	static_cast<QComboBox*>( editor )->setCurrentIndex( itemIdx );
}

void ComboDelegate::setModelData( QWidget* editor, QAbstractItemModel* model, const QModelIndex& index ) const
{
	model->setData( index, static_cast<QComboBox*>( editor )->currentIndex() );
}
