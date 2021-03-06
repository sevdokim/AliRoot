// $Id$
//-*- Mode: C++ -*-
#ifndef ALIHLTTRIGGERTRDCLUSTERMULT_H
#define ALIHLTTRIGGERTRDCLUSTERMULT_H
//* This file is property of and copyright by the ALICE HLT Project        * 
//* ALICE Experiment at CERN, All rights reserved.                         *
//* See cxx source for full Copyright notice                               *

/// @file   AliHLTTriggerPhosMip.h
/// @author Svein Lindal
/// @date   2009-08-17
/// @brief  HLT Minimum Ionizing Particle (MIP) trigger for PHOS

#include "AliHLTTrigger.h"

/**
 * @class  AliHLTTriggerPhosMip
 * HLT trigger component for Minimum Ionizing Particles (MIPs) in PHOS.
 * 
 * Triggers on PHOS clusters containing total energy in the range
 * fEMin < energy < fEMax spread out between fewer than fNCellsMax cells.
 *
 * <h2>General properties:</h2>
 *
 * Component ID: \b PhosMipTrigger                             <br>
 * Library: \b libAliHLTTrigger.so                                        <br>
 * Input Data Types:  kAliHLTDataTypeESDObject, kAliHLTDataTypeESDTree    <br>
 * Output Data Types: ::kAliHLTAnyDataType                                <br>
 *
 * <h2>Mandatory arguments:</h2>
 * <!-- NOTE: ignore the \li. <i> and </i>: it's just doxygen formatting -->
 *
 * <h2>Optional arguments:</h2>
 * <!-- NOTE: ignore the \li. <i> and </i>: it's just doxygen formatting -->
 *
 * <h2>Configuration:</h2>
 * <!-- NOTE: ignore the \li. <i> and </i>: it's just doxygen formatting -->
 * \li -emin     <i> e   </i> <br>
 *      minimum required energy of the cluster
 *
* \li -emax    <i> e   </i> <br>
 *      maximum energy of the cluster
 *
 * \li -ncells     <i> e   </i> <br>
 *      Maximum number of cells in cluster
 *
 * 
 * By default, configuration is loaded from OCDB, can be overridden by
 * component arguments.
 *
 * <h2>Default CDB entries:</h2>
 * HLT/ConfigHLT/PhosMipTrigger: TObjString storing the arguments
 *
 * <h2>Performance:</h2>
 * 
 *
 * <h2>Memory consumption:</h2>
 * 
 *
 * <h2>Output size:</h2>
 * 
 *
 * \ingroup alihlt_trigger_components
 */

class TClonesArray;
class AliHLTTriggerTrdClusterMultiplicity : public AliHLTTrigger
{
public:
  AliHLTTriggerTrdClusterMultiplicity();
  ~AliHLTTriggerTrdClusterMultiplicity();


  /// inherited from AliHLTTrigger: name of this trigger
  virtual const char* GetTriggerName() const;
  /// inherited from AliHLTComponent: create an instance
  virtual AliHLTComponent* Spawn();

 protected:
  /// inherited from AliHLTComponent: handle the initialization
  int DoInit(int argc, const char** argv);

  /// inherited from AliHLTComponent: handle cleanup
  int DoDeinit();

  /// inherited from AliHLTComponent: handle re-configuration event
  int Reconfigure(const char* cdbEntry, const char* chainId);

  /// inherited from AliHLTTrigger: calculate the trigger
  virtual int DoTrigger();

  int Configure(const char* arguments);

  /// Not implemented.
  AliHLTTriggerTrdClusterMultiplicity(const AliHLTTriggerTrdClusterMultiplicity&);
  /// Not implemented.
  AliHLTTriggerTrdClusterMultiplicity& operator = (const AliHLTTriggerTrdClusterMultiplicity&);

  /// Variables for triggger configuration
  Int_t fClusterMult;
  TClonesArray* fClusterArray;

  /// the default configuration entry for this component

  ClassDef(AliHLTTriggerTrdClusterMultiplicity, 0)
};

#endif
