// AnalyzeTool.cpp
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//=============================================================================
// INCLUDES
//=============================================================================
#include <OpenSim/Common/XMLDocument.h>
#include "AnalyzeTool.h"
#include <OpenSim/Common/IO.h>
#include <OpenSim/Common/GCVSplineSet.h>
#include <OpenSim/Common/VectorGCVSplineR1R3.h>
#include <OpenSim/Simulation/Control/ControlLinear.h>
#include <OpenSim/Simulation/Control/ControlSet.h>
#include <OpenSim/Simulation/Manager/Manager.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/BodySet.h>
#include <OpenSim/Simulation/Model/ForceSet.h>
#include <OpenSim/Analyses/MuscleAnalysis.h>
#include <OpenSim/Analyses/MuscleAnalysisV1.h> //MM
#include <OpenSim/Simulation/Model/PrescribedForce.h>
//#include <OpenSim/Analyses/MomentArmAnalysis.h>

using namespace OpenSim;
using namespace std;


//=============================================================================
// CONSTRUCTOR(S) AND DESTRUCTOR
//=============================================================================
//_____________________________________________________________________________
/**
 * Destructor.
 */
AnalyzeTool::~AnalyzeTool()
{
}
//_____________________________________________________________________________
/**
 * Default constructor.
 */
AnalyzeTool::AnalyzeTool() :
	AbstractTool(),
	_statesFileName(_statesFileNameProp.getValueStr()),
	_coordinatesFileName(_coordinatesFileNameProp.getValueStr()),
	_speedsFileName(_speedsFileNameProp.getValueStr()),
	_lowpassCutoffFrequency(_lowpassCutoffFrequencyProp.getValueDbl()),
    _loadModelAndInput(false),
	_printResultFiles(true)
{
	setType("AnalyzeTool");
	setNull();
}
//_____________________________________________________________________________
/**
 * Construct from file.
 *
 * The object is constructed from the root element of the XML document.
 * The type of object is the tag name of the XML root element.
 *
 * @param aFileName File name of the document.
 */
AnalyzeTool::AnalyzeTool(const string &aFileName, bool aLoadModelAndInput) :
	AbstractTool(aFileName, false),
	_statesFileName(_statesFileNameProp.getValueStr()),
	_coordinatesFileName(_coordinatesFileNameProp.getValueStr()),
	_speedsFileName(_speedsFileNameProp.getValueStr()),
	_lowpassCutoffFrequency(_lowpassCutoffFrequencyProp.getValueDbl()),
    _loadModelAndInput(aLoadModelAndInput),
	_printResultFiles(true)
{
	setType("AnalyzeTool");
	setNull();
	updateFromXMLDocument();

	if(aLoadModelAndInput) {
		loadModel(aFileName);
		// Append to or replace model forces with those (i.e. actuators) specified by the analysis
		updateModelForces(*_model, aFileName);
		setModel(*_model);	
		setToolOwnsModel(true);

	}
}
//_____________________________________________________________________________
/**
 * Construct with a passed in model.
 *
 * Typically used from the GUI where the model is readily available.
 * This special constructor avoid many steps/generalities in th AnalyzeTool 
 * Analyses are added to the model beforehand.
 *
 * @param aModel model in the GUI.
 * 
 */
AnalyzeTool::AnalyzeTool(Model& aModel) :
	AbstractTool(),
	_statesFileName(_statesFileNameProp.getValueStr()),
	_coordinatesFileName(_coordinatesFileNameProp.getValueStr()),
	_speedsFileName(_speedsFileNameProp.getValueStr()),
	_lowpassCutoffFrequency(_lowpassCutoffFrequencyProp.getValueDbl()),
    _loadModelAndInput(false),
	_printResultFiles(true)
{
	setType("AnalyzeTool");
	setNull();
	setModel(aModel);
	// By default add a muscleAnalysis and a MomentArmAnalysis and turn them off if they
	// they have not been included already
	AnalysisSet& analysisSet = aModel.updAnalysisSet();
	if (analysisSet.getIndex("MuscleAnalysis")==-1){
		MuscleAnalysis* muscleAnalysis = new MuscleAnalysis(&aModel);
		muscleAnalysis->setOn(false);
		aModel.addAnalysis(muscleAnalysis);
	}

	//if (analysisSet.getIndex("MomentArmAnalysis")==-1){
	//	MomentArmAnalysis* momentArmAnalysis = new MomentArmAnalysis(aModel);
	//	momentArmAnalysis->setOn(false);
	//	aModel->addAnalysis(momentArmAnalysis);
	//}
}

//_____________________________________________________________________________
/**
 * Copy constructor.
 *
 * Copy constructors for all Tools only copy the non-XML variable
 * members of the object; that is, the object's DOMnode and XMLDocument
 * are not copied but set to NULL.  The reason for this is that for the
 * object and all its derived classes to establish the correct connection
 * to the XML document nodes, the the object would need to reconstruct based
 * on the XML document not the values of the object's member variables.
 *
 * There are three proper ways to generate an XML document for an Tool:
 *
 * 1) Construction based on XML file (@see Tool(const char *aFileName)).
 * In this case, the XML document is created by parsing the XML file.
 *
 * 2) Construction by Tool(const XMLDocument *aDocument).
 * This constructor explictly requests construction based on an
 * XML document.  In this way the proper connection between an object's node
 * and the corresponding node within the XML document is established.
 * This constructor is a copy constructor of sorts because all essential
 * Tool member variables should be held within the XML document.
 * The advantage of this style of construction is that nodes
 * within the XML document, such as comments that may not have any
 * associated Tool member variable, are preserved.
 *
 * 3) A call to generateXMLDocument().
 * This method generates an XML document for the Tool from scratch.
 * Only the essential document nodes are created (that is, nodes that
 * correspond directly to member variables.).
 *
 * @param aTool Object to be copied.
 * @see Tool(const XMLDocument *aDocument)
 * @see Tool(const char *aFileName)
 * @see generateXMLDocument()
 */
AnalyzeTool::
AnalyzeTool(const AnalyzeTool &aTool) :
	AbstractTool(aTool),
	_statesFileName(_statesFileNameProp.getValueStr()),
	_coordinatesFileName(_coordinatesFileNameProp.getValueStr()),
	_speedsFileName(_speedsFileNameProp.getValueStr()),
	_lowpassCutoffFrequency(_lowpassCutoffFrequencyProp.getValueDbl()),
    _loadModelAndInput(false)
{
	setType("AnalyzeTool");
	setNull();
	*this = aTool;
}

//_____________________________________________________________________________
/**
 * Virtual copy constructor.
 */
Object* AnalyzeTool::
copy() const
{
	AnalyzeTool *object = new AnalyzeTool(*this);
	return(object);
}

//_____________________________________________________________________________
/**
 * Set all member variables to their null or default values.
 */
void AnalyzeTool::
setNull()
{
	setupProperties();

	_statesFileName = "";
	_coordinatesFileName = "";
	_speedsFileName = "";
	_lowpassCutoffFrequency = -1.0;

	_statesStore = NULL;

	_printResultFiles = true;
    _replaceForceSet = false;
}
//_____________________________________________________________________________
/**
 * Connect properties to local pointers.
 */
void AnalyzeTool::setupProperties()
{
	string comment;


	comment = "Storage file (.sto) containing the time history of states for the model. "
				 "This file often contains multiple rows of data, each row being a time-stamped array of states. "
				 "The first column contains the time.  The rest of the columns contain the states in the order "
				 "appropriate for the model. In a storage file, unlike a motion file (.mot), non-uniform time spacing "
				 "is allowed.  If the user-specified initial time for a simulation does not correspond exactly to "
				 "one of the time stamps in this file, inerpolation is NOT used because it is sometimes necessary to "
				 "an exact set of states for analyses.  Instead, the closest earlier set of states is used.";
	_statesFileNameProp.setComment(comment);
	_statesFileNameProp.setName("states_file");
	_propertySet.append( &_statesFileNameProp );

	comment = "Motion file (.mot) or storage file (.sto) containing the time history of the generalized coordinates for the model. "
				 "These can be specified in place of the states file.";
	_coordinatesFileNameProp.setComment(comment);
	_coordinatesFileNameProp.setName("coordinates_file");
	_propertySet.append( &_coordinatesFileNameProp );

	comment = "Storage file (.sto) containing the time history of the generalized speeds for the model. "
				 "If coordinates_file is used in place of states_file, these can be optionally set as well to give the speeds. "
				 "If not specified, speeds will be computed from coordinates by differentiation.";
	_speedsFileNameProp.setComment(comment);
	_speedsFileNameProp.setName("speeds_file");
	_propertySet.append( &_speedsFileNameProp );

	comment = "Low-pass cut-off frequency for filtering the coordinates_file data (currently does not apply to states_file or speeds_file). "
				 "A negative value results in no filtering. The default value is -1.0, so no filtering.";
	_lowpassCutoffFrequencyProp.setComment(comment);
	_lowpassCutoffFrequencyProp.setName("lowpass_cutoff_frequency_for_coordinates");
	_propertySet.append( &_lowpassCutoffFrequencyProp );

}


//=============================================================================
// OPERATORS
//=============================================================================
//_____________________________________________________________________________
/**
 * Assignment operator.
 *
 * @return Reference to this object.
 */
AnalyzeTool& AnalyzeTool::
operator=(const AnalyzeTool &aTool)
{
	// BASE CLASS
	AbstractTool::operator=(aTool);

	// MEMEBER VARIABLES
	_statesFileName = aTool._statesFileName;
	_coordinatesFileName = aTool._coordinatesFileName;
	_speedsFileName = aTool._speedsFileName;
	_lowpassCutoffFrequency= aTool._lowpassCutoffFrequency;
	_statesStore = aTool._statesStore;
	_printResultFiles = aTool._printResultFiles;
	return(*this);
}


//_____________________________________________________________________________
/**
 * aUStore is optional.
 * Assumes coordinates and speeds are already in radians.
 * Fills in zeros for actuator and contact set states.
 */
Storage *AnalyzeTool::
createStatesStorageFromCoordinatesAndSpeeds(const Model& aModel, const Storage& aQStore, const Storage& aUStore)
{
	int nq = aModel.getNumCoordinates();
	int nu = aModel.getNumSpeeds();
	int ny = aModel.getNumStateVariables();

	if(aQStore.getSmallestNumberOfStates() != nq)
		throw Exception("AnalyzeTool.initializeFromFiles: ERROR- Coordinates storage does not have correct number of coordinates.",__FILE__,__LINE__);
	if(aUStore.getSmallestNumberOfStates() != nu)
		throw Exception("AnalyzeTool.initializeFromFiles: ERROR- Speeds storage does not have correct number of coordinates.",__FILE__,__LINE__);
	if(aQStore.getSize() != aUStore.getSize())
		throw Exception("AnalyzeTool.initializeFromFiles: ERROR- The coordinates storage and speeds storage should have the same number of rows, but do not.",__FILE__,__LINE__);

	Array<string> stateNames("");
	stateNames = aModel.getStateVariableNames();
	stateNames.insert(0, "time");

	Storage *statesStore = new Storage(512,"states");
	statesStore->setColumnLabels(stateNames);
	Array<double> y(0.0,ny);
	for(int index=0; index<aQStore.getSize(); index++) {
		double t;
		aQStore.getTime(index,t);
		aQStore.getData(index,nq,&y[0]);
		aUStore.getData(index,nu,&y[nq]);
		statesStore->append(t,ny,&y[0]);
	}

	return statesStore;
}

//_____________________________________________________________________________
/**
 * Set the states storage.  A states storage is required to run the analyze
 * tool.  The rows of a states storage consist of time-stamped vectors of
 * all the model states.  Time is in the first column and is assumed to
 * increasing monotonically.
 *
 * @param Pointer to storage file containing the time history of model
 * states.
 */
void AnalyzeTool::
setStatesStorage(Storage& aStore)
{
	_statesStore = &aStore;
}
//_____________________________________________________________________________
/**
 * Get the states storage.
 *
 * @return Pointer to the states storage.
 */
Storage& AnalyzeTool::
getStatesStorage()
{
	return *_statesStore;
}


//=============================================================================
// UTILITY
//=============================================================================
//_____________________________________________________________________________
/**
 * Initialize the controls, states, and external loads from
 * files.  The file names are stored in the property set.  The file names
 * can either come from the XML setup file, or they can be set explicitly.
 * Either way, this method should be called to read all the needed information
 * in from file.
 */
void AnalyzeTool::
loadStatesFromFile(SimTK::State& s)
{
	delete _statesStore; _statesStore = NULL;
	if(_statesFileNameProp.isValidFileName()) {
		if(_coordinatesFileNameProp.isValidFileName()) cout << "WARNING: Ignoring " << _coordinatesFileNameProp.getName() << " since " << _statesFileNameProp.getName() << " is also set" << endl;
		if(_speedsFileNameProp.isValidFileName()) cout << "WARNING: Ignoring " << _speedsFileNameProp.getName() << " since " << _statesFileNameProp.getName() << " is also set" << endl;
		cout<<"\nLoading states from file "<<_statesFileName<<"."<<endl;
		Storage temp(_statesFileName);
		_statesStore = new Storage();
		_model->formStateStorage(temp, *_statesStore);
	} else {
		if(!_coordinatesFileNameProp.isValidFileName()) 
			throw Exception("AnalyzeTool.initializeFromFiles: Either a states file or a coordinates file must be specified.",__FILE__,__LINE__);

		cout<<"\nLoading coordinates from file "<<_coordinatesFileName<<"."<<endl;
		Storage coordinatesStore(_coordinatesFileName);

		if(_lowpassCutoffFrequency>=0) {
			cout<<"\n\nLow-pass filtering coordinates data with a cutoff frequency of "<<_lowpassCutoffFrequency<<"..."<<endl<<endl;
			//coordinatesStore.pad(60);
			//coordinatesStore.lowpassFIR(50,_lowpassCutoffFrequency);
			//coordinatesStore.smoothSpline(5,_lowpassCutoffFrequency);
			coordinatesStore.pad(coordinatesStore.getSize()/2);
			coordinatesStore.lowpassIIR(_lowpassCutoffFrequency);
		}

		Storage *qStore=NULL, *uStore=NULL;

 		_model->getSimbodyEngine().formCompleteStorages( s, coordinatesStore,qStore,uStore);

		if(_speedsFileName!="") {
			delete uStore;
			cout<<"\nLoading speeds from file "<<_speedsFileName<<"."<<endl;
			uStore = new Storage(_speedsFileName);
		}

		_model->getSimbodyEngine().convertDegreesToRadians(*qStore);
		_model->getSimbodyEngine().convertDegreesToRadians(*uStore);

		_statesStore = createStatesStorageFromCoordinatesAndSpeeds(*_model, *qStore, *uStore);

		delete qStore;
		delete uStore;
	}

	cout<<"Found "<<_statesStore->getSize()<<" state vectors with time stamps ranging "
		 <<"from "<<_statesStore->getFirstTime()<<" to "<<_statesStore->getLastTime()<<"."<<endl;
}

void AnalyzeTool::
setStatesFromMotion(const SimTK::State& s, const Storage &aMotion, bool aInDegrees)
{
	cout<<endl<<"Creating states from motion storage"<<endl;

	// Make a copy in case we need to convert to degrees and/or filter
	Storage motionCopy(aMotion);

	if(!aInDegrees) _model->getSimbodyEngine().convertRadiansToDegrees(motionCopy);

	if(_lowpassCutoffFrequency>=0) {
		cout<<"\nLow-pass filtering coordinates data with a cutoff frequency of "<<_lowpassCutoffFrequency<<"..."<<endl;
		//motionCopy.pad(60);
		//motionCopy.lowpassFIR(50,_lowpassCutoffFrequency);
		//motionCopy.smoothSpline(5,_lowpassCutoffFrequency);
		motionCopy.pad(motionCopy.getSize()/2);
		motionCopy.lowpassIIR(_lowpassCutoffFrequency);
	}

	Storage *qStore=NULL, *uStore=NULL;
    _model->getSimbodyEngine().formCompleteStorages(s,motionCopy,qStore,uStore);

	_model->getSimbodyEngine().convertDegreesToRadians(*qStore);
	_model->getSimbodyEngine().convertDegreesToRadians(*uStore);

	_statesStore = createStatesStorageFromCoordinatesAndSpeeds(*_model, *qStore, *uStore);

	delete qStore;
	delete uStore;
}

//_____________________________________________________________________________
/**
 * Verify that the controls and states are consistent with the
 * model.
 */
void AnalyzeTool::
verifyControlsStates()
{
	int ny = _model->getNumStateVariables();

	// DO WE HAVE STATES?
	// States
	if(_statesStore==NULL) {
		string msg = "analyzeTool.verifyControlsStates: ERROR- a storage object containing "
							"the time histories of states was not specified.";
		throw Exception(msg,__FILE__,__LINE__);
	}

	// States
	
	int NY = _statesStore->getSmallestNumberOfStates();
	if(NY!=ny) {
		string msg = "AnalyzeTool.verifyControlsStates: ERROR- Number of states in " + _statesFileName;
		msg += " doesn't match number of states in model " + _model->getName() + ".";
		throw Exception(msg,__FILE__,__LINE__);
	}
}

//_____________________________________________________________________________
/**
 * Turn On/Off writing result storages to files.
 */
void AnalyzeTool::
setPrintResultFiles(bool aToWrite)
{
	_printResultFiles=aToWrite;
}
//=============================================================================
// RUN
//=============================================================================
//_____________________________________________________________________________
/**
 * Run the investigation.
 */
bool AnalyzeTool::run()
{
	return run(false);
}
bool AnalyzeTool::run(bool plotting)
{
	//cout<<"Running analyze tool "<<getName()<<"."<<endl;

	// CHECK FOR A MODEL
	if(_model==NULL) {
		string msg = "ERROR- A model has not been set.";
		cout<<endl<<msg<<endl;
		throw(Exception(msg,__FILE__,__LINE__));
	}

	// Use the Dynamics Tool API to handle external loads instead of outdated AbstractTool
	bool externalLoads = createExternalLoads(_externalLoadsFileName, *_model);

//printf("\nbefore AnalyzeTool.run() initSystem \n");
	// Call initSystem except when plotting
	SimTK::State& s = (!plotting)? _model->initSystem(): _model->updMultibodySystem().updDefaultState();
    _model->getMultibodySystem().realize(s, SimTK::Stage::Position );
//printf("after AnalyzeTool.run() initSystem \n\n");

	if(_loadModelAndInput) {
		loadStatesFromFile(s);
	}


	// Do the maneuver to change then restore working directory 
	// so that the parsing code behaves properly if called from a different directory.
	string saveWorkingDirectory = IO::getCwd();
	if (_document)	// When the tool is created live from GUI it has no file/document association
		IO::chDir(IO::getParentDirectory(getDocumentFileName()));

	bool completed = true;

	try {

	// VERIFY THE CONTROL SET, STATES, AND PSEUDO STATES ARE TENABLE
	verifyControlsStates();

	// SET OUTPUT PRECISION
	IO::SetPrecision(_outputPrecision);

	// ANALYSIS SET
	AnalysisSet& analysisSet = _model->updAnalysisSet();
	if(analysisSet.getSize()<=0) {
		string msg = "AnalysisTool.run: ERROR- no analyses have been set.";
		throw Exception(msg,__FILE__,__LINE__);
	}

	// Call helper function to process analysis
	/*Array<double> bounds;
	bounds.append(_ti);
	bounds.append(_tf);
	const_cast<Storage &>(aStatesStore).interpolateAt(bounds);*/
	double ti,tf;
	int iInitial = _statesStore->findIndex(_ti);
	int iFinal = _statesStore->findIndex(_tf);
	_statesStore->getTime(iInitial,ti);
	_statesStore->getTime(iFinal,tf);

	// It is rediculous too start before the specified time! So check we aren't doing something stupid.
	//while(ti < _ti){
	//	_statesStore->getTime(++iInitial,ti);
	//}

	cout<<"Executing the analyses from "<<ti<<" to "<<tf<<"..."<<endl;
	run(s, *_model, iInitial, iFinal, *_statesStore, _solveForEquilibriumForAuxiliaryStates);
	_model->getMultibodySystem().realize(s, SimTK::Stage::Position );
	} catch (Exception &x) {
		x.print(cout);
		completed = false;
		IO::chDir(saveWorkingDirectory);
		throw Exception(x.what(),__FILE__,__LINE__);
	}

	// PRINT RESULTS
	// TODO: give option to write partial results if not completed
	if (completed && _printResultFiles)
		printResults(getName(),getResultsDir()); // this will create results directory if necessary

	IO::chDir(saveWorkingDirectory);

	return completed;
}

//=============================================================================
// HELPER
//=============================================================================
void AnalyzeTool::run(SimTK::State& s, Model &aModel, int iInitial, int iFinal, const Storage &aStatesStore, bool aSolveForEquilibrium)
{
	AnalysisSet& analysisSet = aModel.updAnalysisSet();

	for(int i=0;i<analysisSet.getSize();i++) {
		analysisSet.get(i).setStatesStore(aStatesStore);
	}

	// TODO: some sort of filtering or something to make derivatives smoother?
	GCVSplineSet statesSplineSet(5,&aStatesStore);

	// PERFORM THE ANALYSES
	double tPrev=0.0,t=0.0,dt=0.0;
    int ny = s.getNY();
    Array<double> dydt(0.0,ny);
    Array<double> yFromStorage(0.0,ny);

	for(int i=iInitial;i<=iFinal;i++) {
		tPrev = t;
		aStatesStore.getTime(i,s.updTime()); // time
		t = s.getTime();
        aModel.setAllControllersEnabled(true);

		aStatesStore.getData(i,s.getNY(),&s.updY()[0]); // states

		// Adjust configuration to match constraints and other goals
		aModel.assemble(s);

		// computeEquilibriumForAuxiliaryStates before realization as it may affect forces
		if(aSolveForEquilibrium) aModel.computeEquilibriumForAuxiliaryStates(s);

		// Make sure model is atleast ready to provide kinematics
		aModel.getMultibodySystem().realize(s, SimTK::Stage::Velocity);

		if(i==iInitial) {
			analysisSet.begin(s);
		} else if(i==iFinal) {
 	        analysisSet.end(s);
		// Step
		} else {
			analysisSet.step(s,i);
		}
	}
}

//_____________________________________________________________________________
/**
 * Override default implementation by object to intercept and fix the XML node
 * underneath the tool to match current version
 */

void AnalyzeTool::updateFromXMLNode(SimTK::Xml::Element& aNode, int versionNumber)
{
	std::string controlsFileName ="";
	if ( versionNumber < XMLDocument::getLatestVersion()){
		// Replace names of properties
		if (versionNumber<20001){
			//AbstractTool::updateFromXMLNode(aNode, versionNumber);
			// if external loads .mot file has been speified, create 
			// an XML file corresponding to it and set it as new external loads file
			SimTK::Xml::element_iterator iter = aNode.element_begin("external_loads_file");
			if (iter != aNode.element_end()){	

				SimTK::String oldFile;
				iter->getValueAs(oldFile);
				if (oldFile!="" && oldFile!="Unassigned"){
					// get names of bodies for external loads and create an xml file for the forceSet 
					string body1, body2;
					SimTK::Xml::element_iterator iterB1 = aNode.element_begin("external_loads_body1");
					SimTK::Xml::element_iterator iterB2 = aNode.element_begin("external_loads_body2");
					if (iterB1 != aNode.element_end() && iterB2 != aNode.element_end()){
						try {
							string newFileName="";//;
							_externalLoadsFileName = newFileName;
						}
						catch(Exception& e){
							cout << "Old External Loads file " << oldFile << "Could not be used... Ignoring." << endl;
						}
					}
				}
			}
		}
	}
	Object::updateFromXMLNode(aNode, versionNumber);
}
/*
std::string AnalyzeTool::createExternalLoadsFile(const std::string& oldFile, 
										  const std::string& body1, 
										  const std::string& body2)
{
	ForceSet fs;
	bool oldFileValid = !(oldFile=="" || oldFile=="Unassigned");

	std::string savedCwd;
	if(_document) {
		savedCwd = IO::getCwd();
		IO::chDir(IO::getParentDirectory(_document->getFileName()));
	}
	if (oldFileValid){
		if(!ifstream(oldFile.c_str(), ios_base::in).good()) {
			if(_document) IO::chDir(savedCwd);
			string msg =
				"Object: ERR- Could not open file " + oldFile+ ". It may not exist or you don't have permission to read it.";
			throw Exception(msg,__FILE__,__LINE__);
		}
	}
	Storage dataFile(oldFile);
	const Array<string>& labels=dataFile.getColumnLabels();
	bool body1Valid = !(body1=="" || body1=="Unassigned");
	bool body2Valid = !(body2=="" || body2=="Unassigned");
	std::string forceLabels[9] = {"ground_force_vx", "ground_force_vy", "ground_force_vz", 
		"ground_force_px", "ground_force_py", "ground_force_pz", 
		"ground_torque_x", "ground_torque_y", "ground_torque_z"};
	// We'll create upto 2 PrescribedForces
	if (body1Valid && body2Valid){
		// Find first occurance of ground_force_vx
		int indices[9][2];
		for(int i=0; i<9; i++){
			indices[i][0]= labels.findIndex(forceLabels[i]);
			for(int j=indices[i][0]+1; j<labels.getSize(); j++){
				if (labels[j]==forceLabels[i]){
					indices[i][1]=j;
					break;
				}
			}
		}
		for(int f=0; f<2; f++){
			PrescribedForce* pf = new PrescribedForce();
			pf->setBodyName((f==0)?body1:body2);
			char pad[3];
			sprintf(pad,"%d", f+1);
			std::string suffix = "ExternalForce_"+string(pad);
			pf->setName(suffix);
			// Create 9 new dummy GCVSplines and assign names
			GCVSpline** splines= new GCVSpline *[9];
			for(int func=0; func<9; func++){
				splines[func] = new GCVSpline();
				char columnNumber[5];
				sprintf(columnNumber, "#%d", indices[func][f]);
				splines[func]->setName(columnNumber);
			}
			pf->setForceFunctions(splines[0], splines[1], splines[2]);
			pf->setPointFunctions(splines[3], splines[4], splines[5]);
			pf->setTorqueFunctions(splines[6], splines[7], splines[8]);
			fs.append(pf);
		}
		fs.setDataFileName(oldFile);
		std::string newName=oldFile.substr(0, oldFile.length()-4)+".xml";
		fs.print(newName);
		return newName;
	}
	else {
			if(_document) IO::chDir(savedCwd);
			string msg =
				"Object: ERR- Only one body is specified in " + oldFile+ ".";
			throw Exception(msg,__FILE__,__LINE__);
	}
	if(_document) IO::chDir(savedCwd);
}
*/