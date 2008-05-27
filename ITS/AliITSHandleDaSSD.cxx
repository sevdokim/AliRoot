/**************************************************************************
 * Copyright(c) 2007-2009, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

/* $Id$ */

///////////////////////////////////////////////////////////////////////////////
///
/// This class provides ITS SSD data handling
/// used by DA. 
//  Author: Oleksandr Borysov
//  Date: 19/05/2008
///////////////////////////////////////////////////////////////////////////////

#include <Riostream.h> 
#include "AliITSHandleDaSSD.h"
#include <math.h>
#include <limits.h>
#include "event.h"
#include "TFile.h"
#include "TString.h"
#include "TObjArray.h"
#include "AliLog.h"
#include "AliITSNoiseSSD.h"
#include "AliITSPedestalSSD.h"
#include "AliITSBadChannelsSSD.h"
#include "AliITSRawStreamSSD.h"
#include "AliRawReaderDate.h"
#include "AliITSRawStreamSSD.h"
#include "AliITSChannelDaSSD.h"


ClassImp(AliITSHandleDaSSD)

using namespace std;


const Int_t    AliITSHandleDaSSD::fgkNumberOfSSDModules = 1698;       // Number of SSD modules in ITS
const Int_t    AliITSHandleDaSSD::fgkNumberOfSSDModulesPerDdl = 108;  // Number of SSD modules in DDL
const Int_t    AliITSHandleDaSSD::fgkNumberOfSSDModulesPerSlot = 12;  // Number of SSD modules in Slot
const Int_t    AliITSHandleDaSSD::fgkNumberOfSSDDDLs = 16;            // Number of SSD modules in Slot
const Float_t  AliITSHandleDaSSD::fgkPedestalThresholdFactor = 3.0;   // Defalt value for fPedestalThresholdFactor 
const Float_t  AliITSHandleDaSSD::fgkCmThresholdFactor = 3.0;         // Defalt value for fCmThresholdFactor

//______________________________________________________________________________
AliITSHandleDaSSD::AliITSHandleDaSSD() :
  fRawDataFileName(NULL),
  fNumberOfModules(0),
  fModules(NULL),
  fModIndProcessed(0),
  fModIndRead(0),
  fModIndex(NULL),
  fNumberOfEvents(0),
  fStaticBadChannelsMap(NULL),
  fDDLModuleMap(NULL),
  fLdcId(0),
  fRunId(0),
  fPedestalThresholdFactor(fgkPedestalThresholdFactor),
  fCmThresholdFactor(fgkCmThresholdFactor) 
{
// Default constructor
}


//______________________________________________________________________________
AliITSHandleDaSSD::AliITSHandleDaSSD(Char_t *rdfname) :
  fRawDataFileName(NULL),
  fNumberOfModules(0),
  fModules(NULL),
  fModIndProcessed(0),
  fModIndRead(0),
  fModIndex(NULL),
  fNumberOfEvents(0),
  fStaticBadChannelsMap(NULL),
  fDDLModuleMap(NULL),
  fLdcId(0),
  fRunId(0),
  fPedestalThresholdFactor(fgkPedestalThresholdFactor) ,
  fCmThresholdFactor(fgkCmThresholdFactor) 
{
  if (!Init(rdfname)) AliError("AliITSHandleDaSSD::AliITSHandleDaSSD() initialization error!");
}


//______________________________________________________________________________
AliITSHandleDaSSD::AliITSHandleDaSSD(const AliITSHandleDaSSD& ssdadldc) :
  TObject(ssdadldc),
  fRawDataFileName(ssdadldc.fRawDataFileName),
  fNumberOfModules(ssdadldc.fNumberOfModules),
  fModules(NULL),
  fModIndProcessed(ssdadldc.fModIndProcessed),
  fModIndRead(ssdadldc.fModIndRead),
  fModIndex(NULL),
  fNumberOfEvents(ssdadldc.fNumberOfEvents),
  fStaticBadChannelsMap(ssdadldc.fStaticBadChannelsMap),
  fDDLModuleMap(ssdadldc.fDDLModuleMap),
  fLdcId(ssdadldc.fLdcId),
  fRunId(ssdadldc.fRunId),
  fPedestalThresholdFactor(ssdadldc.fPedestalThresholdFactor),
  fCmThresholdFactor(ssdadldc.fCmThresholdFactor) 
{
  // copy constructor
  if ((ssdadldc.fNumberOfModules > 0) && (ssdadldc.fModules)) {
    fModules = new (nothrow) AliITSModuleDaSSD* [ssdadldc.fNumberOfModules];
    if (fModules) {
      for (Int_t modind = 0; modind < ssdadldc.fNumberOfModules; modind++) {
        if (ssdadldc.fModules[modind]) {
	  fModules[modind] = new AliITSModuleDaSSD(*(ssdadldc.fModules[modind]));
	  if (!fModules[modind]) { 
	    AliError("AliITSHandleDaSSD: Error copy constructor");
            for (Int_t i = (modind - 1); i >= 0; i--) delete fModules[modind];
	    delete [] fModules;
	    fModules = NULL;
	    break;
	  }
	} else fModules[modind] = NULL; 
      }	  
    } else {
      AliError(Form("AliITSHandleDaSSD: Error allocating memory for %i AliITSModulesDaSSD* objects!", ssdadldc.fNumberOfModules));
      fNumberOfModules = 0;
      fModules = NULL;
    }
  }
}


//______________________________________________________________________________
AliITSHandleDaSSD& AliITSHandleDaSSD::operator = (const AliITSHandleDaSSD& ssdadldc)
{
// assignment operator
  if (this == &ssdadldc)  return *this;
  if (fModules) {
    for (Int_t i = 0; i < fNumberOfModules; i++) if (fModules[i]) delete fModules[i];
    delete [] fModules; 
    fModules = NULL;
  }
  if (fModIndex) { delete [] fModIndex; fModIndex = NULL; }
  if ((ssdadldc.fNumberOfModules > 0) && (ssdadldc.fModules)) {
    fModules = new (nothrow) AliITSModuleDaSSD* [ssdadldc.fNumberOfModules];
    if (fModules) {
      for (Int_t modind = 0; modind < ssdadldc.fNumberOfModules; modind++) {
        if (ssdadldc.fModules[modind]) {
	  fModules[modind] = new AliITSModuleDaSSD(*(ssdadldc.fModules[modind]));
	  if (!fModules[modind]) { 
	    AliError("AliITSHandleDaSSD: Error assignment operator");
            for (Int_t i = (modind - 1); i >= 0; i--) delete fModules[modind];
	    delete [] fModules;
	    fModules = NULL;
	    break;
	  }
	} else fModules[modind] = NULL; 
      }	  
    } else {
      AliError(Form("AliITSHandleDaSSD: Error allocating memory for %i AliITSModulesDaSSD* objects!", ssdadldc.fNumberOfModules));
      fNumberOfModules = 0;
      fModules = NULL;
    }
  }
  return *this;
}


//______________________________________________________________________________
AliITSHandleDaSSD::~AliITSHandleDaSSD()
{
// Default destructor 
  if (fModules) 
  {
    for (Int_t i = 0; i < fNumberOfModules; i++)
    { 
      if (fModules[i]) delete fModules[i];
    }
    delete [] fModules;
  }
  if (fModIndex) delete [] fModIndex;
  if (fStaticBadChannelsMap) { fStaticBadChannelsMap->Delete(); delete fStaticBadChannelsMap; }
  if (fDDLModuleMap) delete [] fDDLModuleMap;
}



//______________________________________________________________________________
void AliITSHandleDaSSD::Reset()
{
// Delete array of AliITSModuleDaSSD* objects. 
  if (fModules) {
    for (Int_t i = 0; i < fNumberOfModules; i++) if (fModules[i]) delete fModules[i];
    delete [] fModules;
    fModules = NULL;
  }
  if (fModIndex) { delete [] fModIndex; fModIndex = NULL; }
/*
  if (fStaticBadChannelsMap) {
    fStaticBadChannelsMap->Delete();
    delete fStaticBadChannelsMap;
    fStaticBadChannelsMap = NULL; 
  }    
  if (fDDLModuleMap) { delete [] fDDLModuleMap; fDDLModuleMap = NULL; }
*/
  fRawDataFileName = NULL;
  fModIndProcessed = fModIndRead = 0;
  fNumberOfEvents = 0;
  fLdcId = fRunId = 0;
  fPedestalThresholdFactor = fgkPedestalThresholdFactor;
  fCmThresholdFactor = fgkCmThresholdFactor;
}



//______________________________________________________________________________
Bool_t AliITSHandleDaSSD::Init(Char_t *rdfname, const Char_t *configfname)
{
// Read raw data file and set initial and configuration parameters
  Long_t physeventind = 0, strn = 0, nofstripsev, nofstrips = 0;
  Int_t  nofeqipmentev, nofeqipment = 0, eqn = 0; 
  AliRawReaderDate  *rawreaderdate = NULL;
  UChar_t *data = NULL;
  Long_t datasize = 0, eqbelsize = 1;

  if (configfname) ReadConfigurationFile(configfname);
  rawreaderdate = new AliRawReaderDate(rdfname, 0);
  if (!(rawreaderdate->GetAttributes() || rawreaderdate->GetEventId())) {
    AliError(Form("AliITSHandleDaSSD: Error reading raw data file %s by RawReaderDate", rdfname));
    MakeZombie();
    return kFALSE;
  }
  if (rawreaderdate->NextEvent()) {
    fRunId = rawreaderdate->GetRunNumber(); 
    rawreaderdate->RewindEvents();
  } else { MakeZombie(); return kFALSE; }
  if (fModules) Reset();
  rawreaderdate->SelectEvents(-1);
  rawreaderdate->Select("ITSSSD");  
  nofstrips = 0;
  while (rawreaderdate->NextEvent()) {
    if ((rawreaderdate->GetType() != PHYSICS_EVENT) && (rawreaderdate->GetType() != CALIBRATION_EVENT)) continue;
    nofstripsev = 0;
    nofeqipmentev = 0;
    while (rawreaderdate->ReadNextData(data)) {
      fLdcId = rawreaderdate->GetLDCId();
      nofeqipmentev += 1;
      datasize = rawreaderdate->GetDataSize();
      eqbelsize = rawreaderdate->GetEquipmentElementSize();
      if ( datasize % eqbelsize ) {
        AliError(Form("AliITSHandleDaSSD: Error Init(%s): event data size %i is not an integer of equipment data size %i", 
	                            rdfname, datasize, eqbelsize));
        MakeZombie();
	return kFALSE;
      }
      nofstripsev += (Int_t) (datasize / eqbelsize);
    }
    if (physeventind++) {
      if (nofstrips != nofstripsev) AliWarning(Form("AliITSHandleDaSSD: number of strips varies from event to event, ev = %i, %i", 
                                                     physeventind, nofstripsev));
      if (nofeqipment != nofeqipmentev) AliWarning("AliITSHandleDaSSD: number of DDLs varies from event to event");
    }
    nofstrips = nofstripsev;
    nofeqipment = nofeqipmentev;
    if (strn < nofstrips)  strn = nofstrips;
    if (eqn < nofeqipment) eqn = nofeqipment;
  }
  delete rawreaderdate;
  if ((physeventind > 0) && (strn > 0))
  {
    fNumberOfEvents = physeventind;
    fRawDataFileName = rdfname;
    fModIndex = new (nothrow) Int_t [fgkNumberOfSSDModulesPerDdl * eqn];
    if (fModIndex) 
      for (Int_t i = 0; i < fgkNumberOfSSDModulesPerDdl * eqn; i++) fModIndex[i] = -1; 
    else AliWarning(Form("AliITSHandleDaSSD: Error Init(%s): Index array for %i modules was not created", 
                                     rdfname, fgkNumberOfSSDModulesPerDdl * eqn));
    if (SetNumberOfModules(fgkNumberOfSSDModulesPerDdl * eqn)) {
      TString str = TString::Format("Max number of equipment: %i, max number of channels: %i\n", eqn, strn);
      DumpInitData(str.Data());
      return kTRUE;
    }  
  }
  MakeZombie();
  return kFALSE;
}


//______________________________________________________________________________
Bool_t AliITSHandleDaSSD::ReadConfigurationFile(const Char_t * /* configfname */) 
const {
// Dowload configuration parameters from configuration file or database
  return kFALSE;
}



//______________________________________________________________________________
AliITSModuleDaSSD* AliITSHandleDaSSD::GetModule (const UChar_t ddlID, const UChar_t ad, const UChar_t adc) const
{
// Retrieve AliITSModuleDaSSD object from the array
   if (!fModules) return NULL;
   for (Int_t i = 0; i < fNumberOfModules; i++) {
     if (fModules[i]) {
       if (    (fModules[i]->GetDdlId() == ddlID)
            && (fModules[i]->GetAD() == ad)
            && (fModules[i]->GetADC() == adc))
	    return fModules[i];
     }	    
   }
   return NULL;
}


Int_t AliITSHandleDaSSD::GetModuleIndex (const UChar_t ddlID, const UChar_t ad, const UChar_t adc) const
{
// Retrieve the position of AliITSModuleDaSSD object in the array
   if (!fModules) return -1;
   for (Int_t i = 0; i < fNumberOfModules; i++) {
     if (fModules[i]) {
       if (    (fModules[i]->GetDdlId() == ddlID)
            && (fModules[i]->GetAD() == ad)
            && (fModules[i]->GetADC() == adc))
	    return i;
     }	    
   }
   return -1;
}



//______________________________________________________________________________
AliITSChannelDaSSD* AliITSHandleDaSSD::GetStrip (const UChar_t ddlID, const UChar_t ad, const UChar_t adc, const UShort_t stripID) const
{
// Retrieve AliITSChannalDaSSD object from the array
  for (Int_t modind = 0; modind < fNumberOfModules; modind++) {
    if (    (fModules[modind]->GetDdlId() == ddlID)
         && (fModules[modind]->GetAD() == ad)
         && (fModules[modind]->GetADC() == adc)  ) 
        {
       return fModules[modind]->GetStrip(stripID);
    }
  }
  AliError(Form("AliITSHandleDaSSD: Error GetStrip (%i, %i, %i, %i), strip not found, returns NULL!",
                ddlID, ad, adc, stripID));
  return NULL;
}



//______________________________________________________________________________
Bool_t AliITSHandleDaSSD::SetModule(AliITSModuleDaSSD *const module, const Int_t index)
{ 
// Assign array element with AliITSModuleDaSSD object
   if ((index < fNumberOfModules) && (index >= 0)) 
     { 
        if (fModules[index]) delete fModules[index];
	fModules[index] = module;
	return kTRUE;
      }
   else return kFALSE;   		       
}



//______________________________________________________________________________
Bool_t AliITSHandleDaSSD::SetNumberOfModules (const Int_t numberofmodules)
{
// Allocates memory for AliITSModuleDaSSD objects
  if (numberofmodules > fgkNumberOfSSDModules)
    AliWarning(Form("AliITSHandleDaSSD: the number of modules %i you use exceeds the maximum %i for ALICE ITS SSD", 
                     numberofmodules, fgkNumberOfSSDModules));
  if (fModules) {
    for (Int_t i = 0; i < fNumberOfModules; i++) if (fModules[i]) delete fModules[i];
    delete [] fModules; 
    fModules = NULL;
  }
  fModules = new (nothrow) AliITSModuleDaSSD* [numberofmodules];
  if (fModules) {
    fNumberOfModules = numberofmodules;
    memset(fModules, 0, sizeof(AliITSModuleDaSSD*) * numberofmodules);
    return kTRUE;
  } else {
     AliError(Form("AliITSHandleDaSSD: Error allocating memory for %i AliITSModulesDaSSD* objects!", numberofmodules));
     fNumberOfModules = 0;
     fModules = NULL;
  }
  return kFALSE;
}



//______________________________________________________________________________
Bool_t AliITSHandleDaSSD::ReadStaticBadChannelsMap(const Char_t *filename)
{
// Reads Static Bad Channels Map from the file
  TFile *bcfile;
  if (!filename) {
    AliWarning("No file name is specified for Static Bad Channels Map!");
    return kFALSE;
  } 
  bcfile = new TFile(filename, "READ");
  if (bcfile->IsZombie()) {
    AliWarning(Form("Error reading file %s with Static Bad Channels Map!", filename));
    return kFALSE;
  }
  bcfile->GetObject("BadChannels;1", fStaticBadChannelsMap);
  if (!fStaticBadChannelsMap) {
    AliWarning("Error fStaticBadChannelsMap == NULL!");
    bcfile->Close();
    delete bcfile;
    return kFALSE;
  }
  bcfile->Close();
  delete bcfile;
  return kTRUE;
}



Bool_t AliITSHandleDaSSD::ReadDDLModuleMap(const Char_t *filename)
{
// Reads the SSD DDL Map from the file
  ifstream             ddlmfile;
  AliRawReaderDate    *rwr = NULL;
  AliITSRawStreamSSD  *rsm = NULL;
  void                *event = NULL;
  if (fDDLModuleMap) { delete [] fDDLModuleMap; fDDLModuleMap = NULL;}
  fDDLModuleMap = new (nothrow) Int_t [fgkNumberOfSSDDDLs * fgkNumberOfSSDModulesPerDdl];
  if (!fDDLModuleMap) {
    AliWarning("Error allocation memory for DDL Map!");
    return kFALSE;
  }    
  if (!filename) {
    AliWarning("No file name is specified for SSD DDL Map, using the one from AliITSRawStreamSSD!");
    rwr = new AliRawReaderDate(event);
    rsm = new AliITSRawStreamSSD(rwr);
    rsm->Setv11HybridDDLMapping();
    for (Int_t ddli = 0; ddli < fgkNumberOfSSDDDLs; ddli++)
      for (Int_t mi = 0; mi < fgkNumberOfSSDModulesPerDdl; mi++)
        fDDLModuleMap[(ddli * fgkNumberOfSSDModulesPerDdl + mi)] = rsm->GetModuleNumber(ddli, mi);
    if (rsm) delete rsm;
    if (rwr) delete rwr;	
    return kTRUE;
  } 
  ddlmfile.open(filename, ios::in);
  if (!ddlmfile.is_open()) {
    AliWarning(Form("Error reading file %s with SSD DDL Map!", filename));
    if (fDDLModuleMap) { delete [] fDDLModuleMap; fDDLModuleMap = NULL;}
    return kFALSE;
  }
  Int_t ind = 0;
  while((!ddlmfile.eof()) && (ind < (fgkNumberOfSSDDDLs * fgkNumberOfSSDModulesPerDdl))) {
    ddlmfile >> fDDLModuleMap[ind++];
  }
  if (ind != (fgkNumberOfSSDDDLs * fgkNumberOfSSDModulesPerDdl))
    AliWarning(Form("Only %i (< %i) entries were read from DDL Map!", ind, (fgkNumberOfSSDDDLs * fgkNumberOfSSDModulesPerDdl)));
  ddlmfile.close();
  return kTRUE;
}



//______________________________________________________________________________
Int_t AliITSHandleDaSSD::ReadCalibrationDataFile (char* fileName, const Long_t eventsnumber)
{
// Reads raw data from file
  if (!Init(fileName)){
    AliError("AliITSHandleDaSSD: Error ReadCalibrationDataFile");
    return kFALSE;
  }
  fNumberOfEvents = eventsnumber;
  return ReadModuleRawData (fNumberOfModules);  
}



//______________________________________________________________________________
Int_t AliITSHandleDaSSD::ReadModuleRawData (const Int_t modulesnumber)
{
// Reads raw data from file
  AliRawReader        *rawreaderdate = NULL;
  AliITSRawStreamSSD  *stream = NULL;
  AliITSModuleDaSSD   *module;
  AliITSChannelDaSSD  *strip;
  Long_t            eventind = 0;
  Int_t             nofeqipmentev, equipid, prequipid;
  UShort_t          modind;
  if (!(rawreaderdate = new AliRawReaderDate(fRawDataFileName, 0))) return 0;
  if (!fModules) {
    AliError("AliITSHandleDaSSD: Error ReadModuleRawData: no structure was allocated for data");
    return 0;
  }
  if (!fDDLModuleMap) if (!ReadDDLModuleMap()) AliWarning("DDL map is not defined, ModuleID will be set to 0!");
  stream = new AliITSRawStreamSSD(rawreaderdate);
  stream->Setv11HybridDDLMapping();
  rawreaderdate->SelectEvents(-1);
  modind = 0;
  while (rawreaderdate->NextEvent()) {
    if ((rawreaderdate->GetType() != PHYSICS_EVENT) && (rawreaderdate->GetType() != CALIBRATION_EVENT)) continue;
    nofeqipmentev = 0;
    prequipid = -1;
    while (stream->Next()) {
      equipid    = rawreaderdate->GetEquipmentId(); 
      if ((equipid != prequipid) && (prequipid >= 0)) nofeqipmentev += 1;
      prequipid = equipid;
      Int_t     equiptype  = rawreaderdate->GetEquipmentType();
      UChar_t   ddlID      = (UChar_t)rawreaderdate->GetDDLID();
      UChar_t   ad      = stream->GetAD();
      UChar_t   adc     = stream->GetADC();
      UShort_t  stripID = stream->GetSideFlag() ? AliITSChannelDaSSD::GetMaxStripIdConst() - stream->GetStrip() : stream->GetStrip();
      Short_t   signal  = stream->GetSignal();

      Int_t indpos = (nofeqipmentev * fgkNumberOfSSDModulesPerDdl)
                   + ((ad - 1) * fgkNumberOfSSDModulesPerSlot) + (adc < 6 ? adc : (adc - 2));
      Int_t modpos = fModIndex[indpos];
      if (((modpos > 0) && (modpos < fModIndRead)) || ((modpos < 0) && (modind == modulesnumber))) continue;
      if ((modpos < 0) && (modind < modulesnumber)) {
        module = new AliITSModuleDaSSD(AliITSModuleDaSSD::GetStripsPerModuleConst());
        Int_t mddli;
        if (fDDLModuleMap) mddli = RetrieveModuleId(ddlID, ad, adc);
        else  mddli = 0;
	if (!module->SetModuleIdData (ddlID, ad, adc, mddli)) return 0;
//	if (!module->SetModuleIdData (ddlID, ad, adc, RetrieveModuleId(ddlID, ad, adc))) return 0;
        module->SetModuleRorcId (equipid, equiptype);
	module->SetCMFeromEventsNumber(fNumberOfEvents);
	modpos = fModIndRead + modind;
	modind += 1;
	fModules[modpos] = module;
	fModIndex[indpos] = modpos;
      } 
      if (stripID < AliITSModuleDaSSD::GetStripsPerModuleConst()) {
        if (!(strip = fModules[modpos]->GetStrip(stripID))) {
          strip = new AliITSChannelDaSSD(stripID, fNumberOfEvents);
          fModules[modpos]->SetStrip(strip, stripID);
        }
        strip->SetSignal(eventind, signal);
      } else  fModules[modpos]->SetCMFerom(signal, (stripID - AliITSModuleDaSSD::GetStripsPerModuleConst()), eventind);
    }
    if (++eventind > fNumberOfEvents) break;
  }
  delete stream;
  delete rawreaderdate;
  if (modind) cout << "The memory was allocated for " <<  modind << " modules." << endl;
  fModIndRead += modind;
  if (modind < modulesnumber) RelocateModules();
  return modind;
}



//______________________________________________________________________________
Bool_t AliITSHandleDaSSD::RelocateModules()
{
// Relocate memory for AliITSModuleDaSSD object array
  Int_t         nm = 0;
  AliITSModuleDaSSD **marray = NULL;
  for (Int_t modind = 0; modind < fNumberOfModules; modind++)
    if (fModules[modind]) nm += 1;
  if (nm == fNumberOfModules) return kTRUE;
  marray = new (nothrow) AliITSModuleDaSSD* [nm];
  if (!marray) {
    AliError(Form("AliITSHandleDaSSD: Error relocating memory for %i AliITSModuleDaSSD* objects!", nm));
    return kFALSE;
  }
  nm = 0;
  for (Int_t modind = 0; modind < fNumberOfModules; modind++)  
    if (fModules[modind]) marray[nm++] = fModules[modind];
  delete [] fModules;
  fModules = marray;
  fNumberOfModules = fModIndRead = nm;
  return kTRUE;
}



//______________________________________________________________________________
Bool_t AliITSHandleDaSSD::AddFeromCm(AliITSModuleDaSSD *const module)
// Restore the original signal value adding CM calculated and subtracted in ferom
{
  AliITSChannelDaSSD *strip;
  Short_t            *signal, *cmferom;

  if (!module) return kFALSE;
  for (Int_t chipind = 0; chipind < AliITSModuleDaSSD::GetChipsPerModuleConst(); chipind++) {
    if (!(cmferom = module->GetCMFerom(chipind))) {
      AliWarning(Form("AliITSHandleDaSSD: There is no Ferom CM values for chip %i, module %i!", chipind, module->GetModuleId()));
      continue;
    }
    for (Int_t strind = (chipind * AliITSModuleDaSSD::GetStripsPerChip()); 
               strind < ((chipind + 1) * AliITSModuleDaSSD::GetStripsPerChip()); strind++) {
      if (!(strip = module->GetStrip(strind))) continue;
      if (!(signal = strip->GetSignal())) continue;
//      if (strip->GetEventsNumber() != module->GetEventsNumber()) return kFALSE;
      Long_t ovev = 0;
      for (Long_t ev = 0; ev < strip->GetEventsNumber(); ev++) {
        if (SignalOutOfRange(signal[ev]) || SignalOutOfRange(cmferom[ev])) ovev += 1;
        else {
          Short_t signal1 = signal[ev] + cmferom[ev];
          strip->SetSignal(ev, signal1);
	}  
      }	
    } 
  }  
  return kTRUE;
}



//______________________________________________________________________________
Bool_t AliITSHandleDaSSD::CalculatePedestal(AliITSModuleDaSSD *const module)
{
// Calculates Pedestal
  AliITSChannelDaSSD *strip;
  Float_t             pedestal, noise;
  Short_t            *signal;
  Long_t              ovev, ev, n;
  if (!module) return kFALSE;
  for (Int_t strind = 0; strind < module->GetNumberOfStrips(); strind++) {
    if (!(strip = module->GetStrip(strind))) continue;
    if (!(signal = strip->GetSignal())) {
      AliError(Form("AliITSHandleDaSSD: Error CalculatePedestal(): there are no events data for module[%i] strip[%i]->GetSignal()",
                     module->GetModuleId(), strind));
      continue;	
    }
//************* pedestal first pass ****************
    pedestal = 0.0f;
    ovev = 0l;
    for (ev = 0; ev < strip->GetEventsNumber(); ev++)
      if (SignalOutOfRange(signal[ev])) ovev += 1;
      else pedestal = ((ev - ovev)) ? pedestal + (signal[ev] - pedestal) / static_cast<Float_t>(ev - ovev + 1) : signal[ev];
    if (ev == ovev) pedestal = AliITSChannelDaSSD::GetUndefinedValue();
    strip->SetPedestal(pedestal);	
//************* noise *******************************
    Double_t nsum = 0.0L;
    ovev = 0l;
    for (ev = 0; ev < strip->GetEventsNumber(); ev++) {
      if (SignalOutOfRange(signal[ev])) ovev += 1;
      else nsum += pow((signal[ev] - strip->GetPedestal()), 2);
    } 
    if ((n = strip->GetEventsNumber() - ovev - 1) > 0) noise =  sqrt(nsum / (Float_t)(n));
    else  noise = AliITSChannelDaSSD::GetUndefinedValue();
    strip->SetNoise(noise);
//************* pedestal second pass ****************
    pedestal = 0.0f;
    ovev = 0l;
    for (ev = 0; ev < strip->GetEventsNumber(); ev++)
      if (   SignalOutOfRange(signal[ev]) 
          || TMath::Abs(signal[ev] - strip->GetPedestal()) > (fPedestalThresholdFactor * strip->GetNoise())) ovev += 1;
      else pedestal = ((ev - ovev)) ? pedestal + (signal[ev] - pedestal) / static_cast<Float_t>(ev - ovev + 1) : signal[ev];
    if (ev == ovev) pedestal = AliITSChannelDaSSD::GetUndefinedValue();
    strip->SetPedestal(pedestal);	
    strip->SetOverflowNumber(ovev);	
  }
  return kTRUE;
}


//______________________________________________________________________________
Bool_t AliITSHandleDaSSD::CalculateNoise(AliITSModuleDaSSD *const module)
{
// Calculates Noise
  AliITSChannelDaSSD *strip;
  Short_t    *signal;
  Float_t     noise;
  Long_t      ovev, n;
  if (!module) return kFALSE;
  for (Int_t strind = 0; strind < module->GetNumberOfStrips(); strind++) {
    if (!(strip = module->GetStrip(strind))) continue;
    if (!(signal = strip->GetSignal())) {
      strip->SetNoise(AliITSChannelDaSSD::GetUndefinedValue());
      AliError(Form("AliITSHandleDaSSD: Error CalculateNoise(): there are no events data for module[%i] strip[%i]->GetSignal()",
                     module->GetModuleId(), strind));
      continue;
    }
    Double_t nsum = 0.0L;
    ovev = 0l;
    for (Long_t ev = 0; ev < strip->GetEventsNumber(); ev++) {
      if (SignalOutOfRange(signal[ev])) ovev += 1;
      else nsum += pow((signal[ev] - strip->GetPedestal()), 2);
    } 
    if ((n = strip->GetEventsNumber() - ovev - 1) > 0) noise =  sqrt(nsum / (Float_t)(n));
    else  noise = AliITSChannelDaSSD::GetUndefinedValue();
    strip->SetNoise(noise);
  }
  return kTRUE;
}



//______________________________________________________________________________
Bool_t AliITSHandleDaSSD::CalculateNoiseCM(AliITSModuleDaSSD *const module)
{
// Calculates Noise with CM correction
  AliITSChannelDaSSD  *strip = NULL;
  Short_t     *signal;
  Float_t      noise;
  Long_t       ovev, n;
  if (!CalculateCM(module)) { 
    AliError("Error: AliITSHandleDaSSD::CalculateCM() returned kFALSE");
    return kFALSE;
  }  
  for (Int_t strind = 0; strind < module->GetNumberOfStrips(); strind++) {
    if (!(strip = module->GetStrip(strind))) continue; //return kFALSE;
    if (!(signal = strip->GetSignal())) {
      strip->SetNoiseCM(AliITSChannelDaSSD::GetUndefinedValue());
      AliError(Form("SSDDAModule: Error CalculateNoiseWithoutCM(): there are no events data for module[%i] strip[%i]->GetSignal()", 
                     module->GetModuleId(), strind));
      continue; //return kFALSE;
    }
    Int_t chipind = strind / AliITSModuleDaSSD::GetStripsPerChip();
    Double_t nsum = 0.0L;
    ovev = 0l;
    for (Long_t ev = 0; ev < strip->GetEventsNumber(); ev++) {
      if (SignalOutOfRange(signal[ev])) ovev += 1;
      else nsum += pow((signal[ev] - strip->GetPedestal() - module->GetCM(chipind, ev)), 2);
    } 
    if ((n = strip->GetEventsNumber() - ovev - 1) > 0) noise =  sqrt(nsum / (Float_t)(n));
    else  noise = AliITSChannelDaSSD::GetUndefinedValue();
    strip->SetNoiseCM(noise);
  }
  return kTRUE;
}



//______________________________________________________________________________
Bool_t AliITSHandleDaSSD::CalculateCM(AliITSModuleDaSSD *const module)
{
// Calculates CM
  AliITSChannelDaSSD  *strip = NULL;
  Short_t             *signal;
  Long_t               ovstr, n;
  Int_t                stripind;
  module->SetNumberOfChips(AliITSModuleDaSSD::GetChipsPerModuleConst());
  for (Int_t chipind = 0; chipind < module->GetNumberOfChips(); chipind++) {
    stripind = chipind * module->GetStripsPerChip();
    module->GetCM()[chipind].Set(fNumberOfEvents);
    module->GetCM()[chipind].Reset(0.0f);
    for (Long_t ev = 0; ev < fNumberOfEvents; ev++) {
    // calculate firs approximation of CM.
      Double_t cm0 = 0.0L;
      ovstr = 0l;
      for (Int_t strind = stripind; strind < (stripind + AliITSModuleDaSSD::GetStripsPerChip()); strind++) {
        if (!(strip = module->GetStrip(strind))) continue; //return kFALSE; 
        if (!(signal = strip->GetSignal())) {
          AliError(Form("AliITSHandleDaSSD: Error CalculateCM: there are no events data for module[%i] strip[%i]->GetSignal()", 
	                module->GetModuleId(), strind));
          return kFALSE;
        }
        if ((signal[ev] >= AliITSChannelDaSSD::GetOverflowConst()) || 
	    (strip->GetPedestal() == AliITSChannelDaSSD::GetUndefinedValue())) ovstr += 1;
        else cm0 += (signal[ev] - strip->GetPedestal());
      }
      if ((n = AliITSModuleDaSSD::GetStripsPerChip() - ovstr)) cm0 /= (Float_t)(n);
      else { module->SetCM(0.0f, chipind, ev); continue; }
    // calculate avarage (cm - (signal - pedestal)) over the chip
      Double_t cmsigma = 0.0L;
      ovstr = 0l;
      for (Int_t strind = stripind; strind < (stripind + AliITSModuleDaSSD::GetStripsPerChip()); strind++) {
        if (!(strip = module->GetStrip(strind))) continue;
        if (!(signal = strip->GetSignal())) {
          AliError(Form("AliITSHandleDaSSD: Error CalculateCM: there are no events data for module[%i] strip[%i]->GetSignal()\n",
	                 module->GetModuleId(), strind));
          return kFALSE;
        }
        if ((signal[ev] >= AliITSChannelDaSSD::GetOverflowConst()) || 
	    (strip->GetPedestal() == AliITSChannelDaSSD::GetUndefinedValue())) ovstr += 1;
        else cmsigma += pow((cm0 - (signal[ev] - strip->GetPedestal())), 2);
      }
      if ((n = AliITSModuleDaSSD::GetStripsPerChip() - ovstr)) cmsigma = sqrt(cmsigma / (Float_t)(n));
      else { module->SetCM(0.0f, chipind, ev); continue; }
   // calculate cm with threshold
      Double_t cmsum = 0.0L;
      ovstr = 0l;
      for (Int_t strind = stripind; strind < (stripind + AliITSModuleDaSSD::GetStripsPerChip()); strind++) {
        if (!(strip = module->GetStrip(strind))) continue;
        signal = strip->GetSignal();
        if (   (signal[ev] >= AliITSChannelDaSSD::GetOverflowConst())
	    || (strip->GetPedestal() == AliITSChannelDaSSD::GetUndefinedValue()) 
	    || (TMath::Abs(cm0 - (signal[ev] - strip->GetPedestal())) > (fCmThresholdFactor * cmsigma)) ) ovstr += 1;
        else cmsum += (signal[ev] - strip->GetPedestal());
      }
      if ((n = AliITSModuleDaSSD::GetStripsPerChip() - ovstr)) cmsum /= (Float_t)(n);
      else cmsum = 0.0L;
      if (!module->SetCM(cmsum, chipind, ev));
    } 
  }
  return kTRUE; 
}


//______________________________________________________________________________
Bool_t AliITSHandleDaSSD::ProcessRawData(const Int_t nmread)
{
// Performs calculation of calibration parameters (pedestal, noise, ...) 
  Int_t nm = 0;
  if (nmread <= 0) return kFALSE;
  if (!fModules) return kFALSE;
  while ((nm = ReadModuleRawData(nmread)) > 0) {
    cout << "Processing next " << nm << " modules;" << endl;  
    for (Int_t modind = fModIndProcessed; modind < fModIndRead; modind++) {
      if (!fModules[modind]) {
        AliError(Form("AliITSHandleDaSSD: Error ProcessRawData(): No AliITSModuleDaSSD object with index %i is allocated in AliITSHandleDaSSD\n",
	               modind));
        return kFALSE;
      }
      AddFeromCm(fModules[modind]);
      CalculatePedestal(fModules[modind]);
      CalculateNoise(fModules[modind]);
      CalculateNoiseCM(fModules[modind]);
    }
    DeleteSignal();
    DeleteCM();
    DeleteCMFerom();
    fModIndProcessed = fModIndRead;
    cout << fModIndProcessed << " - done" << endl;
  }
  return kTRUE;  
}


//______________________________________________________________________________
Short_t AliITSHandleDaSSD::RetrieveModuleId(const UChar_t ddlID, const UChar_t ad, const UChar_t adc) const
{
// Retrieve ModuleId from the DDL map which is defined in AliITSRawStreamSSD class
  if (!fDDLModuleMap) return 0;
  Int_t mddli = ((ad - 1) * fgkNumberOfSSDModulesPerSlot) + (adc < 6 ? adc : (adc - 2));
  if ((ddlID < fgkNumberOfSSDDDLs) && (mddli < fgkNumberOfSSDModulesPerDdl)) {
    mddli = fDDLModuleMap[ddlID * fgkNumberOfSSDModulesPerDdl + mddli];
  }  
  else {
    AliWarning(Form("Module index  = %d or ddlID = %d is out of range 0 is rturned", ddlID, mddli));
    mddli = 0;
  }
  if (mddli > SHRT_MAX) return SHRT_MAX;
  else return (Short_t)mddli;
}



//______________________________________________________________________________
TObjArray* AliITSHandleDaSSD::GetCalibrationSSDLDC() const
{
// Fill in the array for OCDB 
  TObjArray *ldcc;
  TObject   *modcalibobj;
  if (!fModules) return NULL;
  ldcc = new TObjArray(fNumberOfModules, 0);
  for (Int_t i = 0; i < fNumberOfModules; i++) {
    if (!fModules[i]) {
      delete ldcc;
      return NULL;
    }
    modcalibobj = dynamic_cast<TObject*>(fModules[i]->GetCalibrationNoise());
    ldcc->AddAt(modcalibobj, i);
  }
  ldcc->Compress();
  return ldcc;
}


//______________________________________________________________________________
Bool_t AliITSHandleDaSSD::SaveCalibrationSSDLDC(Char_t*& dafname) const
{
// Save Calibration data locally
  TObjArray      *ldcn, *ldcp, *ldcbc;
  TObject        *modobjn, *modobjp, *modobjbc;
  Char_t         *tmpfname;
  TString         dadatafilename("");
  if (!fModules) return kFALSE;
  ldcn = new TObjArray(fNumberOfModules, 0);
  ldcn->SetName("Noise");
  ldcp = new TObjArray(fNumberOfModules, 0);
  ldcp->SetName("Pedestal");
  ldcbc = new TObjArray(fNumberOfModules, 0);
  ldcbc->SetName("BadChannels");
  for (Int_t i = 0; i < fNumberOfModules; i++) {
    if (!fModules[i]) {
      delete ldcn;
      return kFALSE;
    }
    modobjn = dynamic_cast<TObject*>(fModules[i]->GetCalibrationNoise());
    modobjp = dynamic_cast<TObject*>(fModules[i]->GetCalibrationPedestal());
    modobjbc = dynamic_cast<TObject*>(fModules[i]->GetCalibrationBadChannels());
    ldcn->AddAt(modobjn, i);
    ldcp->AddAt(modobjp, i);
    ldcbc->AddAt(modobjbc, i);
  }
  ldcn->Compress();
  ldcp->Compress();
  ldcbc->Compress();
  if (dafname) dadatafilename.Form("%s/", dafname);
  dadatafilename += TString::Format("ITSSSDda_%i.root", fLdcId);
  tmpfname = new Char_t[dadatafilename.Length()+1];
  dafname = strcpy(tmpfname, dadatafilename.Data());
  TFile *fileRun = new TFile (dadatafilename.Data(),"RECREATE");
  if (fileRun->IsZombie()) {
    AliError(Form("AliITSHandleDaSSD: SaveCalibrationSSDLDC() error open file %s", dadatafilename.Data()));
    ldcn->Delete();
    delete fileRun;
    delete ldcn;
    delete ldcp;
    delete ldcbc;
    return kFALSE;
  }
  fileRun->WriteTObject(ldcn);
  fileRun->WriteTObject(ldcp);
  if (fStaticBadChannelsMap) fileRun->WriteTObject(fStaticBadChannelsMap);
  else fileRun->WriteTObject(ldcbc);
  fileRun->Close();
  ldcn->Delete();
  delete fileRun;
  delete ldcn;
  delete ldcp;
  delete ldcbc;
  return kTRUE;
}



//______________________________________________________________________________
Bool_t AliITSHandleDaSSD::DumpModInfo(const Float_t meannosethreshold) const
{
// Dump calibration parameters
  AliITSModuleDaSSD    *mod;
  AliITSChannelDaSSD   *strip;
  if (!fModules) {
    cout << "SSDDALDC::DumpModInfo():  Error, no modules are allocated!" << endl;
    return kFALSE;
  }  
  cout << "Modules with MeanNoise > " << meannosethreshold << endl;
  for (Int_t i = 0; i < fNumberOfModules; i++) {
    if (!(mod = fModules[i])) continue;
    Float_t  maxnoise = 0.0f, meannoise = 0.0f, maxped = 0.0f;
    Int_t    maxstrind = 0, novfstr = 0;  
    for (Int_t strind = 0; strind < mod->GetNumberOfStrips(); strind++) {
      if (!(strip = mod->GetStrip(strind))) {novfstr++;  continue; }
      if (strip->GetNoiseCM() >= AliITSChannelDaSSD::GetUndefinedValue() ) {novfstr++;  continue; }
      if (maxnoise < strip->GetNoiseCM()) { 
        maxnoise = strip->GetNoiseCM();
        maxstrind = strind;
      }	
      meannoise = (strind - novfstr) ? meannoise + (strip->GetNoiseCM() - meannoise) / static_cast<Float_t>(strind - novfstr + 1) 
                                    : strip->GetNoiseCM();
      if (TMath::Abs(maxped) < TMath::Abs(strip->GetPedestal())) maxped = strip->GetPedestal();			    
    } 
    if (meannoise > meannosethreshold)
      cout << "Mod: " << i << ";  DDl: " << (int)mod->GetDdlId() << ";  AD: " << (int)mod->GetAD()  
                           << ";  ADC: " << (int)mod->GetADC() << "; MaxPed = " << maxped
			   << ";  MeanNoise = " << meannoise 
			   << ";  NOfStrips = " << (mod->GetNumberOfStrips() - novfstr) << endl;
  }
  return kTRUE;
}



//______________________________________________________________________________
Bool_t AliITSHandleDaSSD::PrintModCalibrationData(const UChar_t ddlID, const UChar_t ad, const UChar_t adc, const Char_t *fname) const
{
// Print Module calibration data whether in file on in cout
   AliITSChannelDaSSD  *strip;
   ofstream             datafile;
   ostream             *outputfile;
   if (!fname) { outputfile = &cout; }
   else {
     datafile.open(fname, ios::out);
     if  (datafile.fail()) return kFALSE;
     outputfile = dynamic_cast<ostream*>(&datafile); 
   }
   *outputfile  << "DDL = " << (int)ddlID << ";  AD = " << (int)ad << ";  ADC = " << (int)adc << endl;
   for (Int_t strind = 0; strind < AliITSModuleDaSSD::GetStripsPerModuleConst(); strind++) {
     if ( (strip = GetStrip(ddlID, ad, adc, strind)) ) {
       *outputfile << "Str = " << strind << ";  ped = " << strip->GetPedestal() 
                   << ";  noise = " << strip->GetNoiseCM() << endl;
     }
     else continue;
   }
  if (datafile.is_open()) datafile.close(); 
  return kTRUE;
}



//______________________________________________________________________________
void AliITSHandleDaSSD::DumpInitData(const Char_t *str) const
{
// Print general information retrieved from raw data file
  cout << "Raw data file: " << fRawDataFileName << endl
       << "LDC: " << (Int_t)fLdcId << "; RunId: " << fRunId << endl
       << "Number of physics events: " << fNumberOfEvents << endl
       << str;
}


//______________________________________________________________________________
Bool_t AliITSHandleDaSSD::AllocateSimulatedModules(const Int_t copymodind)
{
// Used to allocate simulated modules to test the performance  
  AliITSModuleDaSSD *module;
  UChar_t      ad, adc, ddlID;
  ad = adc = ddlID = 0;
  if (!(fModules[copymodind])) return kFALSE;
  for (Int_t modind = 0; modind < fNumberOfModules; modind++) {
    if (!fModules[modind]) {
      module = new AliITSModuleDaSSD(AliITSModuleDaSSD::GetStripsPerModuleConst());
      if ((adc < 5) || ((adc > 7) && (adc < 13)) ) adc += 1;
      else if (adc == 5) adc = 8;
      else if (adc == 13) {
        adc = 0;
        if (ad < 8) ad += 1;
        else {
          ad = 0;
          ddlID +=1;
        }
      }
      if (!module->SetModuleIdData (ddlID, ad, adc, modind)) return kFALSE;
      fModules[modind] = module;
      for (Int_t strind = 0; strind < module->GetNumberOfStrips(); strind++) {
        AliITSChannelDaSSD *cstrip = fModules[copymodind]->GetStrip(strind);
        Long_t      eventsnumber = cstrip->GetEventsNumber();
        AliITSChannelDaSSD *strip = new AliITSChannelDaSSD(strind, eventsnumber);
        for (Long_t evind = 0; evind < eventsnumber; evind++) {
          Short_t sign = *cstrip->GetSignal(evind);
          if (!strip->SetSignal(evind, sign)) AliError(Form("AliITSHandleDaSSD: Copy events error! mod = %i, str = %i", modind, strind));
        }
	module->SetStrip(strip, strind);
      }
    }
    else {
      ddlID = fModules[modind]->GetDdlId();
      ad    = fModules[modind]->GetAD();
      adc   = fModules[modind]->GetADC();
    }
  }
  for (UShort_t modind = 0; modind < fNumberOfModules; modind++) fModules[modind]->SetModuleId(modind + 1080);  
  return kTRUE;
}
