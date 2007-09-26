/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
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
//.
// HMPID base class to reconstruct an event
//.
//.
//.
#include "AliHMPIDReconstructor.h" //class header
#include "AliHMPID.h"              //Reconstruct() 
#include "AliHMPIDCluster.h"       //Dig2Clu()
#include "AliHMPIDParam.h"         //FillEsd() 
#include <AliCDBEntry.h>           //ctor
#include <AliCDBManager.h>         //ctor
#include <AliESDEvent.h>           //FillEsd()
#include <AliRawReader.h>          //Reconstruct() for raw digits
#include "AliHMPIDRawStream.h"     //ConvertDigits()
ClassImp(AliHMPIDReconstructor)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
AliHMPIDReconstructor::AliHMPIDReconstructor():AliReconstructor(),fUserCut(0),fDaqSig(0),fDig(0),fClu(0)
{
//
//ctor
//
  AliHMPIDParam::Instance();                                                        //geometry loaded for reconstruction
  fUserCut = new Int_t[7];
  fClu=new TObjArray(AliHMPIDParam::kMaxCh+1); fClu->SetOwner(kTRUE);
  fDig=new TObjArray(AliHMPIDParam::kMaxCh+1); fDig->SetOwner(kTRUE);
  
  for(int i=AliHMPIDParam::kMinCh;i<=AliHMPIDParam::kMaxCh;i++){ 
    fDig->AddAt(new TClonesArray("AliHMPIDDigit"),i);
    TClonesArray *pClus = new TClonesArray("AliHMPIDCluster");
    pClus->SetUniqueID(i);
    fClu->AddAt(pClus,i);
  }
  
  AliCDBEntry *pUserCutEnt =AliCDBManager::Instance()->Get("HMPID/Calib/UserCut");    //contains TObjArray of 14 TObject with n. of sigmas to cut charge 
  if(pUserCutEnt) {
    TObjArray *pUserCut = (TObjArray*)pUserCutEnt->GetObject(); 
    for(Int_t iCh=AliHMPIDParam::kMinCh;iCh<=AliHMPIDParam::kMaxCh;iCh++){                  //chambers loop 
      fUserCut[iCh] = pUserCut->At(iCh)->GetUniqueID();
      Printf("HMPID: UserCut successfully loaded for chamber %i -> %i ",iCh,fUserCut[iCh]);
    }
  }

  
  AliCDBEntry *pDaqSigEnt =AliCDBManager::Instance()->Get("HMPID/Calib/DaqSig");  //contains TObjArray of TObjArray 14 TMatrixF sigmas values for pads 
  if(!pDaqSigEnt) AliFatal("No pedestals from DAQ!");
  fDaqSig = (TObjArray*)pDaqSigEnt->GetObject();
  for(Int_t iCh=AliHMPIDParam::kMinCh;iCh<=AliHMPIDParam::kMaxCh;iCh++){                  //chambers loop 
    Printf(" HMPID: DaqSigCut successfully loaded for chamber %i -> %i ",iCh,(Int_t)fDaqSig->At(iCh)->GetUniqueID());
  }
}//AliHMPIDReconstructor
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void AliHMPIDReconstructor::Dig2Clu(TObjArray *pDigAll,TObjArray *pCluAll,Bool_t isTryUnfold)
{
// Finds all clusters for a given digits list provided not empty. Currently digits list is a list of all digits for a single chamber.
// Puts all found clusters in separate lists, one per clusters. 
// Arguments: pDigAll     - list of digits for all chambers 
//            pCluAll     - list of clusters for all chambers
//            isTryUnfold - flag to choose between CoG and Mathieson fitting  
//  Returns: none    
  TMatrixF padMap(AliHMPIDParam::kMinPx,AliHMPIDParam::kMaxPcx,AliHMPIDParam::kMinPy,AliHMPIDParam::kMaxPcy);  //pads map for single chamber 0..159 x 0..143 
  
  for(Int_t iCh=AliHMPIDParam::kMinCh;iCh<=AliHMPIDParam::kMaxCh;iCh++){                  //chambers loop 
    TClonesArray *pDigCur=(TClonesArray*)pDigAll->At(iCh);                                //get list of digits for current chamber
    if(pDigCur->GetEntriesFast()==0) continue;                                            //no digits for current chamber
  
    padMap=(Float_t)-1;                                                                   //reset map to -1 (means no digit for this pad)  
    TClonesArray *pCluCur=(TClonesArray*)pCluAll->At(iCh);                                //get list of clusters for current chamber
    
    for(Int_t iDig=0;iDig<pDigCur->GetEntriesFast();iDig++){                              //digits loop to fill pads map
      AliHMPIDDigit *pDig= (AliHMPIDDigit*)pDigCur->At(iDig);                             //get current digit
      padMap( pDig->PadChX(), pDig->PadChY() )=iDig;                                      //fill the map, (padx,pady) cell takes digit index
    }//digits loop to fill digits map 
    
    AliHMPIDCluster clu;                                                                  //tmp cluster to be used as current
    for(Int_t iDig=0;iDig<pDigCur->GetEntriesFast();iDig++){                              //digits loop for current chamber
      AliHMPIDDigit *pDig=(AliHMPIDDigit*)pDigCur->At(iDig);                              //take current digit
      if(!(pDig=UseDig(pDig->PadChX(),pDig->PadChY(),pDigCur,&padMap))) continue;         //this digit is already taken in FormClu(), go after next digit
      FormClu(&clu,pDig,pDigCur,&padMap);                                                 //form cluster starting from this digit by recursion
      
      clu.Solve(pCluCur,isTryUnfold);                                                     //solve this cluster and add all unfolded clusters to provided list  
      clu.Reset();                                                                        //empty current cluster
    }//digits loop for current chamber
  }//chambers loop
}//Dig2Clu()
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void  AliHMPIDReconstructor::FormClu(AliHMPIDCluster *pClu,AliHMPIDDigit *pDig,TClonesArray *pDigLst,TMatrixF *pDigMap)
{
// Forms the initial cluster as a combination of all adjascent digits. Starts from the given digit then calls itself recursevly  for all neighbours.
// Arguments: pClu - pointer to cluster being formed
//   Returns: none   
  pClu->DigAdd(pDig);//take this digit in cluster

  Int_t cnt=0,cx[4],cy[4];
  
  if(pDig->PadPcX() != AliHMPIDParam::kMinPx){cx[cnt]=pDig->PadChX()-1; cy[cnt]=pDig->PadChY()  ;cnt++;}       //left
  if(pDig->PadPcX() != AliHMPIDParam::kMaxPx){cx[cnt]=pDig->PadChX()+1; cy[cnt]=pDig->PadChY()  ;cnt++;}       //right
  if(pDig->PadPcY() != AliHMPIDParam::kMinPy){cx[cnt]=pDig->PadChX()  ; cy[cnt]=pDig->PadChY()-1;cnt++;}       //down
  if(pDig->PadPcY() != AliHMPIDParam::kMaxPy){cx[cnt]=pDig->PadChX()  ; cy[cnt]=pDig->PadChY()+1;cnt++;}       //up
  
  for (Int_t i=0;i<cnt;i++){//neighbours loop
    if((pDig=UseDig(cx[i],cy[i],pDigLst,pDigMap))) FormClu(pClu,pDig,pDigLst,pDigMap);   //check if this neighbour pad fired and mark it as taken  
  }//neighbours loop  
}//FormClu()
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void AliHMPIDReconstructor::Reconstruct(TTree *pDigTree,TTree *pCluTree)const
{
//Invoked  by AliReconstruction to convert digits to clusters i.e. reconstruct simulated data
//Arguments: pDigTree - pointer to Digit tree
//           pCluTree - poitner to Cluster tree
//  Returns: none    
  AliDebug(1,"Start.");
  for(Int_t iCh=AliHMPIDParam::kMinCh;iCh<=AliHMPIDParam::kMaxCh;iCh++) {
    pCluTree->Branch(Form("HMPID%d",iCh),&((*fClu)[iCh]),4000,0);
    pDigTree->SetBranchAddress(Form("HMPID%d",iCh),&((*fDig)[iCh]));
  }   
  pDigTree->GetEntry(0);
  Dig2Clu(fDig,fClu);              //cluster finder 
  pCluTree->Fill();                //fill tree for current event
  
  for(Int_t iCh=AliHMPIDParam::kMinCh;iCh<=AliHMPIDParam::kMaxCh;iCh++){
    fDig->At(iCh)->Clear();
    fClu->At(iCh)->Clear();
  }
  
  AliDebug(1,"Stop.");      
}//Reconstruct(for simulated digits)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void AliHMPIDReconstructor::ConvertDigits(AliRawReader *pRR,TTree *pDigTree)const
{
//Invoked  by AliReconstruction to convert raw digits from DDL files to digits
//Arguments: pRR - ALICE raw reader pointer
//           pDigTree - pointer to Digit tree
//  Returns: none    
  AliDebug(1,"Start.");
//  Int_t digcnt=0;
  

  for(Int_t iCh=AliHMPIDParam::kMinCh;iCh<=AliHMPIDParam::kMaxCh;iCh++) {
    pDigTree->Branch(Form("HMPID%d",iCh),&((*fDig)[iCh]),4000,0); 
    
    Int_t iDigCnt=0;
    AliHMPIDRawStream stream(pRR);    
    while(stream.Next())
    {
      
      UInt_t ddl=stream.GetDDLNumber(); //returns 0,1,2 ... 13
      if((UInt_t)(2*iCh)==ddl || (UInt_t)(2*iCh+1)==ddl) {
       for(Int_t row = 1; row <= AliHMPIDRawStream::kNRows; row++){
        for(Int_t dil = 1; dil <= AliHMPIDRawStream::kNDILOGICAdd; dil++){
          for(Int_t pad = 0; pad < AliHMPIDRawStream::kNPadAdd; pad++){
            if(stream.GetCharge(ddl,row,dil,pad) < 1) continue; 
              AliHMPIDDigit dig(stream.GetPad(ddl,row,dil,pad),stream.GetCharge(ddl,row,dil,pad));
              if(!IsDigSurvive(&dig)) continue; 
              new((*((TClonesArray*)fDig->At(iCh)))[iDigCnt++]) AliHMPIDDigit(dig); //add this digit to the tmp list 
            }//pad
          }//dil
        }//row
      }//while
    }
    stream.Delete();
  }
  pDigTree->Fill();
  
  for(Int_t iCh=AliHMPIDParam::kMinCh;iCh<=AliHMPIDParam::kMaxCh;iCh++)fDig->At(iCh)->Clear();
  
  AliDebug(1,"Stop.");
}//Reconstruct digits from raw digits
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void AliHMPIDReconstructor::FillESD(TTree */*digitsTree*/, TTree */*clustersTree*/, AliESDEvent *pESD) const
{
// Calculates probability to be a electron-muon-pion-kaon-proton
// from the given Cerenkov angle and momentum assuming no initial particle composition
// (i.e. apriory probability to be the particle of the given sort is the same for all sorts)

  AliPID ppp; //needed
  Double_t pid[AliPID::kSPECIES],h[AliPID::kSPECIES];
   
  for(Int_t iTrk=0;iTrk<pESD->GetNumberOfTracks();iTrk++){//ESD tracks loop
    AliESDtrack *pTrk = pESD->GetTrack(iTrk);// get next reconstructed track
    if(pTrk->GetHMPIDsignal()<=0){//HMPID does not find anything reasonable for this track, assign 0.2 for all species
      for(Int_t iPart=0;iPart<AliPID::kSPECIES;iPart++) pid[iPart]=1.0/AliPID::kSPECIES;
      pTrk->SetHMPIDpid(pid);
      continue;
    } 
    Double_t pmod = pTrk->GetP();
    Double_t hTot=0;
    for(Int_t iPart=0;iPart<AliPID::kSPECIES;iPart++){
      Double_t mass = AliPID::ParticleMass(iPart);
      Double_t cosThetaTh = TMath::Sqrt(mass*mass+pmod*pmod)/(AliHMPIDParam::Instance()->MeanIdxRad()*pmod);
      if(cosThetaTh<1) //calculate the height of theoretical theta ckov on the gaus of experimental one
        h[iPart] =TMath::Gaus(TMath::ACos(cosThetaTh),pTrk->GetHMPIDsignal(),TMath::Sqrt(pTrk->GetHMPIDchi2()),kTRUE);      
      else             //beta < 1/ref. idx. => no light at all  
        h[iPart] =0 ;       
      hTot    +=h[iPart]; //total height of all theoretical heights for normalization
    }//species loop
     
    Double_t hMin=TMath::Gaus(pTrk->GetHMPIDsignal()-4*TMath::Sqrt(pTrk->GetHMPIDchi2()),pTrk->GetHMPIDsignal(),TMath::Sqrt(pTrk->GetHMPIDchi2()),kTRUE);//5 sigma protection
    
    for(Int_t iPart=0;iPart<AliPID::kSPECIES;iPart++)//species loop to assign probabilities
      if(hTot>hMin)  
        pid[iPart]=h[iPart]/hTot;
      else                               //all theoretical values are far away from experemental one
        pid[iPart]=1.0/AliPID::kSPECIES; 
    pTrk->SetHMPIDpid(pid);
  }//ESD tracks loop
}//FillESD()
