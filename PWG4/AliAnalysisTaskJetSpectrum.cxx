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


 
#include <TROOT.h>
#include <TSystem.h>
#include <TInterpreter.h>
#include <TChain.h>
#include <TFile.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TH3F.h>
#include <TList.h>
#include <TLorentzVector.h>
#include <TClonesArray.h>
#include  "TDatabasePDG.h"

#include "AliAnalysisTaskJetSpectrum.h"
#include "AliAnalysisManager.h"
#include "AliJetFinder.h"
#include "AliJetReader.h"
#include "AliJetReaderHeader.h"
#include "AliUA1JetHeaderV1.h"
#include "AliJet.h"
#include "AliESDEvent.h"
#include "AliAODEvent.h"
#include "AliAODHandler.h"
#include "AliAODTrack.h"
#include "AliAODJet.h"
#include "AliMCEventHandler.h"
#include "AliMCEvent.h"
#include "AliStack.h"
#include "AliGenPythiaEventHeader.h"
#include "AliJetKineReaderHeader.h"
#include "AliGenCocktailEventHeader.h"

#include "AliAnalysisHelperJetTasks.h"

ClassImp(AliAnalysisTaskJetSpectrum)

AliAnalysisTaskJetSpectrum::AliAnalysisTaskJetSpectrum(): AliAnalysisTaskSE(),
  fJetFinderRec(0x0),
  fJetFinderGen(0x0),
  fAOD(0x0),
  fBranchRec("jets"),
  fConfigRec("ConfigJets.C"),
  fBranchGen(""),
  fConfigGen(""),
  fUseAODInput(kFALSE),
  fUseExternalWeightOnly(kFALSE),
  fAnalysisType(0),
  fExternalWeight(1),    
  fh1PtHard(0x0),
  fh1PtHard_NoW(0x0),  
  fh1PtHard_Trials(0x0),
  fh1PtHard_Trials_NoW(0x0),
  fh1NGenJets(0x0),
  fh1NRecJets(0x0),
  fHistList(0x0)
{
  // Default constructor
    for(int ij  = 0;ij<kMaxJets;++ij){
    fh1E[ij] =  fh1PtRecIn[ij] = fh1PtRecOut[ij] = fh1PtGenIn[ij] = fh1PtGenOut[ij] = 0;
    fh2PtFGen[ij] =  fh2Frag[ij] = fh2FragLn[ij] = 0;
    fh3PtRecGenHard[ij] =  fh3PtRecGenHard_NoW[ij] = fh3RecEtaPhiPt[ij] = fh3RecEtaPhiPt_NoGen[ij] =fh3RecEtaPhiPt_NoFound[ij] =  fh3MCEtaPhiPt[ij] = 0;
  }
  
}

AliAnalysisTaskJetSpectrum::AliAnalysisTaskJetSpectrum(const char* name):
  AliAnalysisTaskSE(name),
  fJetFinderRec(0x0),
  fJetFinderGen(0x0),
  fAOD(0x0),
  fBranchRec("jets"),
  fConfigRec("ConfigJets.C"),
  fBranchGen(""),
  fConfigGen(""),
  fUseAODInput(kFALSE),
  fUseExternalWeightOnly(kFALSE),
  fAnalysisType(0),
  fExternalWeight(1),    
  fh1PtHard(0x0),
  fh1PtHard_NoW(0x0),  
  fh1PtHard_Trials(0x0),
  fh1PtHard_Trials_NoW(0x0),
  fh1NGenJets(0x0),
  fh1NRecJets(0x0),
  fHistList(0x0)
{
  // Default constructor
  for(int ij  = 0;ij<kMaxJets;++ij){
    fh1E[ij] = fh1PtRecIn[ij] = fh1PtRecOut[ij] = fh1PtGenIn[ij] = fh1PtGenOut[ij] = 0;
    fh2PtFGen[ij] =  fh2Frag[ij] = fh2FragLn[ij] = 0;

    fh3PtRecGenHard[ij] =  fh3PtRecGenHard_NoW[ij] = fh3RecEtaPhiPt[ij] = fh3RecEtaPhiPt_NoGen[ij] =fh3RecEtaPhiPt_NoFound[ij] =  fh3MCEtaPhiPt[ij] = 0;
  }

  DefineOutput(1, TList::Class());  
}




void AliAnalysisTaskJetSpectrum::UserCreateOutputObjects()
{

  //
  // Create the output container
  //

  
  // Connect the AOD

  if(fUseAODInput){
    fAOD = dynamic_cast<AliAODEvent*>(InputEvent());
    if(!fAOD){
      Printf("%s:%d AODEvent not found in Input Manager %d",(char*)__FILE__,__LINE__,fUseAODInput);
      return;
    }    
  }
  else{
    //  assume that the AOD is in the general output...
    fAOD  = AODEvent();
    if(!fAOD){
      Printf("%s:%d AODEvent not found in the Output",(char*)__FILE__,__LINE__);
      return;
    }    
  }
 


  if (fDebug > 1) printf("AnalysisTaskJetSpectrum::UserCreateOutputObjects() \n");

  OpenFile(1);
  if(!fHistList)fHistList = new TList();

  Bool_t oldStatus = TH1::AddDirectoryStatus();
  TH1::AddDirectory(kFALSE);

  //
  //  Histogram
    
  const Int_t nBinPt = 100;
  Double_t binLimitsPt[nBinPt+1];
  for(Int_t iPt = 0;iPt <= nBinPt;iPt++){
    if(iPt == 0){
      binLimitsPt[iPt] = 0.0;
    }
    else {// 1.0
      binLimitsPt[iPt] =  binLimitsPt[iPt-1] + 2;
    }
  }
  const Int_t nBinEta = 22;
  Double_t binLimitsEta[nBinEta+1];
  for(Int_t iEta = 0;iEta<=nBinEta;iEta++){
    if(iEta==0){
      binLimitsEta[iEta] = -1.1;
    }
    else{
      binLimitsEta[iEta] = binLimitsEta[iEta-1] + 0.1;
    }
  }

  const Int_t nBinPhi = 360;
  Double_t binLimitsPhi[nBinPhi+1];
  for(Int_t iPhi = 0;iPhi<=nBinPhi;iPhi++){
    if(iPhi==0){
      binLimitsPhi[iPhi] = 0;
    }
    else{
      binLimitsPhi[iPhi] = binLimitsPhi[iPhi-1] + 1/(Float_t)nBinPhi * TMath::Pi()*2;
    }
  }

  const Int_t nBinFrag = 25;


  fh1PtHard = new TH1F("fh1PtHard","PYTHIA Pt hard;p_{T,hard}",nBinPt,binLimitsPt);

  fh1PtHard_NoW = new TH1F("fh1PtHard_NoW","PYTHIA Pt hard no weight;p_{T,hard}",nBinPt,binLimitsPt);

  fh1PtHard_Trials = new TH1F("fh1PtHard_Trials","PYTHIA Pt hard weight with trials;p_{T,hard}",nBinPt,binLimitsPt);

  fh1PtHard_Trials_NoW = new TH1F("fh1PtHard_Trials_NoW","PYTHIA Pt hard weight with trials;p_{T,hard}",nBinPt,binLimitsPt);

  fh1NGenJets  = new TH1F("fh1NGenJets","N generated jets",20,-0.5,19.5);

  fh1NRecJets = new TH1F("fh1NRecJets","N reconstructed jets",20,-0.5,19.5);


  for(int ij  = 0;ij<kMaxJets;++ij){
    fh1E[ij] = new TH1F(Form("fh1E_j%d",ij),"Jet Energy;E_{jet} (GeV);N",nBinPt,binLimitsPt);
    fh1PtRecIn[ij] = new TH1F(Form("fh1PtRecIn_j%d",ij),"rec p_T input ;p_{T,rec}",nBinPt,binLimitsPt);
    fh1PtRecOut[ij] = new TH1F(Form("fh1PtRecOut_j%d",ij),"rec p_T output jets;p_{T,rec}",nBinPt,binLimitsPt);
    fh1PtGenIn[ij] = new TH1F(Form("fh1PtGenIn_j%d",ij),"found p_T input ;p_{T,gen}",nBinPt,binLimitsPt);
    fh1PtGenOut[ij] = new TH1F(Form("fh1PtGenOut_j%d",ij),"found p_T output jets;p_{T,gen}",nBinPt,binLimitsPt);


    fh2PtFGen[ij] = new TH2F(Form("fh2PtFGen_j%d",ij),"Pt Found vs. gen;p_{T,rec} (GeV/c);p_{T,gen} (GeV/c)",
			     nBinPt,binLimitsPt,nBinPt,binLimitsPt);






    fh3PtRecGenHard[ij] = new TH3F(Form("fh3PtRecGenHard_j%d",ij), "Pt hard vs. pt gen vs. pt rec;p_{T,rec};p_{T,gen} (GeV/c);p_{T,hard} (GeV/c)",nBinPt,binLimitsPt,nBinPt,binLimitsPt,nBinPt,binLimitsPt);



    fh3PtRecGenHard_NoW[ij] = new TH3F(Form("fh3PtRecGenHard_NoW_j%d",ij), "Pt hard vs. pt gen vs. pt rec no weight;p_{T,rec};p_{T,gen} (GeV/c);p_{T,hard} (GeV/c)",nBinPt,binLimitsPt,nBinPt,binLimitsPt,nBinPt,binLimitsPt);

	
    fh2Frag[ij] = new TH2F(Form("fh2Frag_j%d",ij),"Jet Fragmentation;x=E_{i}/E_{jet};E_{jet};1/N_{jet}dN_{ch}/dx",
			   nBinFrag,0.,1.,nBinPt,binLimitsPt);

    fh2FragLn[ij] = new TH2F(Form("fh2FragLn_j%d",ij),"Jet Fragmentation Ln;#xi=ln(E_{jet}/E_{i});E_{jet}(GeV);1/N_{jet}dN_{ch}/d#xi",
			     nBinFrag,0.,10.,nBinPt,binLimitsPt);

    fh3RecEtaPhiPt[ij] = new TH3F(Form("fh3RecEtaPhiPt_j%d",ij),"Rec eta, phi, pt; #eta; #phi; p_{T,rec} (GeV/c)",
				  nBinEta,binLimitsEta,nBinPhi,binLimitsPhi,nBinPt,binLimitsPt);



    fh3RecEtaPhiPt_NoGen[ij] = new TH3F(Form("fh3RecEtaPhiPt_NoGen_j%d",ij),"No generated for found jet Rec eta, phi, pt; #eta; #phi; p_{T,rec} (GeV/c)",
					nBinEta,binLimitsEta,nBinPhi,binLimitsPhi,nBinPt,binLimitsPt);


    fh3RecEtaPhiPt_NoFound[ij] = new TH3F(Form("fh3RecEtaPhiPt_NoFound_g%d",ij),"No found for generated jet Rec eta, phi, pt; #eta; #phi; p_{T,rec} (GeV/c)",
					nBinEta,binLimitsEta,nBinPhi,binLimitsPhi,nBinPt,binLimitsPt);



    fh3MCEtaPhiPt[ij] = new TH3F(Form("fh3MCEtaPhiPt_j%d",ij),"MC eta, phi, pt; #eta; #phi; p_{T,rec} (GeV/c)",
				 nBinEta,binLimitsEta,nBinPhi,binLimitsPhi,nBinPt,binLimitsPt);

  }

  const Int_t saveLevel = 1; // large save level more histos

  if(saveLevel>0){
    fHistList->Add(fh1PtHard);
    fHistList->Add(fh1PtHard_NoW);
    fHistList->Add(fh1PtHard_Trials);
    fHistList->Add(fh1PtHard_Trials_NoW);
    fHistList->Add(fh1NGenJets);
    fHistList->Add(fh1NRecJets);
    for(int ij  = 0;ij<kMaxJets;++ij){
      fHistList->Add(fh1E[ij]);
      fHistList->Add(fh1PtRecIn[ij]);
      fHistList->Add(fh1PtRecOut[ij]);
      fHistList->Add(fh1PtGenIn[ij]);
      fHistList->Add(fh1PtGenOut[ij]);
      fHistList->Add(fh2PtFGen[ij]);
      if(saveLevel>2){
	fHistList->Add(fh3RecEtaPhiPt[ij]);
	fHistList->Add(fh3RecEtaPhiPt_NoGen[ij]);
	fHistList->Add(fh3RecEtaPhiPt_NoFound[ij]);
	fHistList->Add(fh3MCEtaPhiPt[ij]);      
      }
    }
  }

  // =========== Switch on Sumw2 for all histos ===========
  for (Int_t i=0; i<fHistList->GetEntries(); ++i) {
    TH1 *h1 = dynamic_cast<TH1*>(fHistList->At(i));
    if (h1){
      // Printf("%s ",h1->GetName());
      h1->Sumw2();
    }
  }

  TH1::AddDirectory(oldStatus);

}

void AliAnalysisTaskJetSpectrum::Init()
{
  //
  // Initialization
  //

  Printf(">>> AnalysisTaskJetSpectrum::Init() debug level %d\n",fDebug);
  if (fDebug > 1) printf("AnalysisTaskJetSpectrum::Init() \n");

}

void AliAnalysisTaskJetSpectrum::UserExec(Option_t */*option*/)
{
  //
  // Execute analysis for current event
  //

  if (fDebug > 1)printf("Analysing event # %5d\n", (Int_t) fEntry);

  
  AliAODHandler *aodH = dynamic_cast<AliAODHandler*>(AliAnalysisManager::GetAnalysisManager()->GetOutputEventHandler());

  if(!aodH){
    Printf("%s:%d no output aodHandler found Jet",(char*)__FILE__,__LINE__);
    return;
  }


  //  aodH->SelectEvent(kTRUE);

  // ========= These pointers need to be valid in any case ======= 


  /*
  AliUA1JetHeaderV1 *jhRec = dynamic_cast<AliUA1JetHeaderV1*>(fJetFinderRec->GetHeader());
  if(!jhRec){
    Printf("%s:%d No Jet Header found",(char*)__FILE__,__LINE__);
    return;
  }
  */
  if (fDebug > 10)Printf("%s:%d",(char*)__FILE__,__LINE__);
  TClonesArray *aodRecJets = dynamic_cast<TClonesArray*>(fAOD->FindListObject(fBranchRec.Data()));
  if(!aodRecJets){
    Printf("%s:%d no reconstructed Jet array with name %s in AOD",(char*)__FILE__,__LINE__,fBranchRec.Data());
    return;
  }

  // ==== General variables needed


  // We use statice array, not to fragment the memory
  AliAODJet genJets[kMaxJets];
  Int_t nGenJets = 0;
  AliAODJet recJets[kMaxJets];
  Int_t nRecJets = 0;

  Double_t eventW = 1;
  Double_t ptHard = 0; 
  Double_t nTrials = 1; // Trials for MC trigger weigth for real data

  if(fUseExternalWeightOnly){
    eventW = fExternalWeight;
  }


  if (fDebug > 10)Printf("%s:%d",(char*)__FILE__,__LINE__);
  if((fAnalysisType&kAnaMC)==kAnaMC){
    // this is the part we only use when we have MC information
    AliMCEvent* mcEvent = MCEvent();
    //    AliStack *pStack = 0; 
    if(!mcEvent){
      Printf("%s:%d no mcEvent",(char*)__FILE__,__LINE__);
      return;
    }
    AliGenPythiaEventHeader*  pythiaGenHeader = AliAnalysisHelperJetTasks::GetPythiaEventHeader(mcEvent);
    if(!pythiaGenHeader){
      return;
    }

    nTrials = pythiaGenHeader->Trials();
    ptHard  = pythiaGenHeader->GetPtHard();

    if(!fUseExternalWeightOnly){
	// case were we combine more than one p_T hard bin...
	// eventW = AnalysisHelperCKB::GetXSectionWeight(pythiaGenHeader->GetPtHard(),fEnergy)*fExternalWeight;      
    }

    // fetch the pythia generated jets only to be used here
    Int_t nPythiaGenJets = pythiaGenHeader->NTriggerJets();
    AliAODJet pythiaGenJets[kMaxJets];

    if(fBranchGen.Length()==0)nGenJets = nPythiaGenJets;
    for(int ip = 0;ip < nPythiaGenJets;++ip){
      if(ip>=kMaxJets)continue;
      Float_t p[4];
      pythiaGenHeader->TriggerJet(ip,p);
      pythiaGenJets[ip].SetPxPyPzE(p[0],p[1],p[2],p[3]);
      if(fBranchGen.Length()==0){
	// if we have MC particles and we do not read from the aod branch
	// use the pythia jets
	genJets[ip].SetPxPyPzE(p[0],p[1],p[2],p[3]);
      }
    }


  }// (fAnalysisType&kMC)==kMC)

  if (fDebug > 10)Printf("%s:%d",(char*)__FILE__,__LINE__);
  fh1PtHard->Fill(ptHard,eventW);
  fh1PtHard_NoW->Fill(ptHard,1);
  fh1PtHard_Trials->Fill(ptHard,nTrials);


  // If we set a second branch for the input jets fetch this 
  if(fBranchGen.Length()>0){
    TClonesArray *aodGenJets = dynamic_cast<TClonesArray*>(fAOD->FindListObject(fBranchGen.Data()));
    if(aodGenJets){
      nGenJets = aodGenJets->GetEntries();
      for(int ig = 0;ig < nGenJets;++ig){
	AliAODJet *tmp = dynamic_cast<AliAODJet*>(aodGenJets->At(ig));
	if(!tmp)continue;
	genJets[ig] = *tmp;
      }
    }
    else{
      Printf("%s:%d Generated jet branch %s not found",fBranchGen.Data());
    }
  }

  fh1NGenJets->Fill(nGenJets);
  // We do not want to exceed the maximum jet number
  nGenJets = TMath::Min(nGenJets,kMaxJets);

  // Fetch the reconstructed jets...


  nRecJets = aodRecJets->GetEntries();
  for(int ir = 0;ir < nRecJets;++ir){
    AliAODJet *tmp = dynamic_cast<AliAODJet*>(aodRecJets->At(ir));
    if(!tmp)continue;
    recJets[ir] = *tmp;
  }

  fh1NRecJets->Fill(nRecJets);
  nRecJets = TMath::Min(nRecJets,kMaxJets);
  if (fDebug > 10)Printf("%s:%d",(char*)__FILE__,__LINE__);
  // Relate the jets
  Int_t iGenIndex[kMaxJets];    // Index of the generated jet for i-th rec -1 if none
  Int_t iRecIndex[kMaxJets];    // Index of the rec jet for i-th gen -1 if none
  
  for(int i = 0;i<kMaxJets;++i){
    iGenIndex[i] = iRecIndex[i] = -1;
  }


  GetClosestJets(genJets,nGenJets,recJets,nRecJets,
		 iGenIndex,iRecIndex,fDebug);
  if (fDebug > 10)Printf("%s:%d",(char*)__FILE__,__LINE__);

  // loop over reconstructed jets
  for(int ir = 0;ir < nRecJets;++ir){
    Double_t ptRec = recJets[ir].Pt();
    Double_t phiRec = recJets[ir].Phi();
    if(phiRec<0)phiRec+=TMath::Pi()*2.;    
    Double_t etaRec = recJets[ir].Eta();

    fh1E[ir]->Fill(recJets[ir].E(),eventW);
    fh1PtRecIn[ir]->Fill(ptRec,eventW);
    fh3RecEtaPhiPt[ir]->Fill(etaRec,phiRec,ptRec,eventW);
    // Fill Correlation
    Int_t ig = iGenIndex[ir];
    if(ig>=0&&ig<nGenJets){
      fh1PtRecOut[ir]->Fill(ptRec,eventW);
      Double_t ptGen = genJets[ig].Pt();
      fh2PtFGen[ir]->Fill(ptRec,ptGen,eventW);
      fh3PtRecGenHard[ir]->Fill(ptRec,ptGen,ptHard,eventW);
      fh3PtRecGenHard_NoW[ir]->Fill(ptRec,ptGen,ptHard,1);
    }
  }// loop over reconstructed jets


  if (fDebug > 10)Printf("%s:%d",(char*)__FILE__,__LINE__);
  for(int ig = 0;ig < nGenJets;++ig){
    Double_t ptGen = genJets[ig].Pt();
    // Fill Correlation
    fh1PtGenIn[ig]->Fill(ptGen,eventW);
    Int_t ir = iRecIndex[ig];
    if(ir>=0){
      fh1PtGenOut[ig]->Fill(ptGen,eventW);
    }
  }// loop over reconstructed jets

  if (fDebug > 10)Printf("%s:%d",(char*)__FILE__,__LINE__);
  PostData(1, fHistList);
}

void AliAnalysisTaskJetSpectrum::Terminate(Option_t */*option*/)
{
// Terminate analysis
//
    if (fDebug > 1) printf("AnalysisJetSpectrum: Terminate() \n");
}


void AliAnalysisTaskJetSpectrum::GetClosestJets(AliAODJet *genJets,Int_t &nGenJets,
						AliAODJet *recJets,Int_t &nRecJets,
						Int_t *iGenIndex,Int_t *iRecIndex,Int_t iDebug){

  //
  // Relate the two input jet Arrays
  //

  //
  // The association has to be unique
  // So check in two directions
  // find the closest rec to a gen
  // and check if there is no other rec which is closer
  // Caveat: Close low energy/split jets may disturb this correlation

  // Idea: search in two directions generated e.g (a--e) and rec (1--3)
  // Fill a matrix with Flags (1 for closest rec jet, 2 for closest rec jet
  // in the end we have something like this
  //    1   2   3
  // ------------
  // a| 3   2   0
  // b| 0   1   0
  // c| 0   0   3
  // d| 0   0   1
  // e| 0   0   1
  // Topology
  //   1     2
  //     a         b        
  //
  //  d      c
  //        3     e
  // Only entries with "3" match from both sides

  Int_t iFlag[kMaxJets][kMaxJets];

  for(int i = 0;i < kMaxJets;++i){
    iRecIndex[i] = -1;
    iGenIndex[i] = -1;
    for(int j = 0;j < kMaxJets;++j)iFlag[i][j] = 0;
  }

  if(nRecJets==0)return;
  if(nGenJets==0)return;

  const Float_t maxDist = 1.4;
  // find the closest distance
  for(int ig = 0;ig<nGenJets;++ig){
    Float_t dist = maxDist;
    if(iDebug>1)Printf("Gen (%d) p_T %3.3f eta %3.3f ph %3.3f ",ig,genJets[ig].Pt(),genJets[ig].Eta(),genJets[ig].Phi());
    for(int ir = 0;ir<nRecJets;++ir){
      Double_t dR = genJets[ig].DeltaR(&recJets[ir]);
      if(iDebug>1)Printf("Rec (%d) p_T %3.3f eta %3.3f ph %3.3f ",ir,recJets[ir].Pt(),recJets[ir].Eta(),recJets[ir].Phi());
      if(iDebug>1)Printf("Distance (%d)--(%d) %3.3f ",ig,ir,dR);
      if(dR<dist){
	iRecIndex[ig] = ir;
	dist = dR;
      }	
    }
    if(iRecIndex[ig]>=0)iFlag[ig][iRecIndex[ig]]+=1;
  }
  // other way around
  for(int ir = 0;ir<nRecJets;++ir){
    Float_t dist = maxDist;
    for(int ig = 0;ig<nGenJets;++ig){
      Double_t dR = genJets[ig].DeltaR(&recJets[ir]);
      if(dR<dist){
	iGenIndex[ir] = ig;
	dist = dR;
      }	
    }
    if(iGenIndex[ir]>=0)iFlag[iGenIndex[ir]][ir]+=2;
  }

  // check for "true" correlations

  if(iDebug>1)Printf(">>>>>> Matrix");

  for(int ig = 0;ig<nGenJets;++ig){
    for(int ir = 0;ir<nRecJets;++ir){
      // Print
      if(iDebug>1)printf("%d ",iFlag[ig][ir]);
      if(iFlag[ig][ir]==3){
	iGenIndex[ir] = ig;
	iRecIndex[ig] = ir;
      }
      else{
	iGenIndex[ir] = iRecIndex[ig] = -1;
      }
    }
    if(iDebug>1)printf("\n");
  }
}


