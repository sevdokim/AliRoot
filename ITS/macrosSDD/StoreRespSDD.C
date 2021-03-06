#if !defined(__CINT__) || defined(__MAKECINT__)
#include "AliCDBManager.h"
#include "AliITSresponseSDD.h"
#include "AliCDBMetaData.h"
#include "AliCDBStorage.h"
#include "AliCDBId.h"
#include "AliCDBPath.h"
#include "AliCDBEntry.h"
#endif

void StoreRespSDD(Int_t firstRun=0, Int_t lastRun=AliCDBRunRange::Infinity()){
  ///////////////////////////////////////////////////////////////////////
  // Macro to generate and store the calibration files for SDD         //
  // Generates:                                                        //
  //  1 file with the AliITSrespionseSDD object (RespSDD)              //
  ///////////////////////////////////////////////////////////////////////
  
  if(!AliCDBManager::Instance()->IsDefaultStorageSet()) {
    AliCDBManager::Instance()->SetDefaultStorage("local://OCDB");
  }
  

  AliCDBMetaData *md = new AliCDBMetaData();
  md->SetObjectClassName("AliITSresponse");
  md->SetResponsible("Francesco Prino");
  md->SetBeamPeriod(0);
  md->SetComment("Simulated data");


  AliCDBId idRespSDD("ITS/Calib/RespSDD",firstRun, lastRun);
  AliITSresponseSDD* rd = new AliITSresponseSDD();
  rd->SetSideATimeZero(485);
  rd->SetSideCTimeZero(485);
//   rd->SetLayer3ATimeZero(235);
//   rd->SetLayer3CTimeZero(287);
//   rd->SetLayer4ATimeZero(202);
//   rd->SetLayer4CTimeZero(230);
  for(Int_t iMod=240; iMod<500; iMod++){
    rd->SetADCtokeV(iMod,2.97);
  }
  AliCDBManager::Instance()->GetDefaultStorage()->Put(rd, idRespSDD, md);  
}
