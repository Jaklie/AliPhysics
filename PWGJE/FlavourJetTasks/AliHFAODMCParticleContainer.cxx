/*************************************************************************
* Copyright(c) 1998-2016, ALICE Experiment at CERN, All rights reserved. *
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

#include <TClonesArray.h>
#include <AliAODMCParticle.h>
#include <AliLog.h>

#include "AliHFAODMCParticleContainer.h"

/// \cond CLASSIMP
ClassImp(AliHFAODMCParticleContainer)
/// \endcond

/// This is the default constructor, used for ROOT I/O purposes.
AliHFAODMCParticleContainer::AliHFAODMCParticleContainer() :
  AliMCParticleContainer(),
  fSpecialPDG(0),
  fRejectedOrigin(0),
  fAcceptedDecay(0)
{
  // Constructor.

  fClassName = "AliAODMCParticle";
}

/// This is the standard named constructor.
///
/// \param name Name of the particle collection
AliHFAODMCParticleContainer::AliHFAODMCParticleContainer(const char *name) :
  AliMCParticleContainer(name),
  fSpecialPDG(0),
  fRejectedOrigin(AliAnalysisTaskDmesonJets::kUnknownQuark | AliAnalysisTaskDmesonJets::kFromBottom),
  fAcceptedDecay(AliAnalysisTaskDmesonJets::kAnyDecay)
{
  // Constructor.

  fClassName = "AliAODMCParticle";
}


/// Automatically sets parameters to select only the decay chain c->D0->Kpi
void AliHFAODMCParticleContainer::SelectCharmtoD0toKpi()
{
  SetSpecialPDG(421);
  SetKeepOnlyD0toKpi();
  SetRejectDfromB(kTRUE);
  SetRejectQuarkNotFound(kTRUE);
  SetKeepOnlyDfromB(kFALSE);
}

/// Automatically sets parameters to select only the decay chain c->D*->Kpipi
void AliHFAODMCParticleContainer::SelectCharmtoDStartoKpipi()
{
  SetSpecialPDG(413);
  SetKeepOnlyDStartoKpipi();
  SetRejectDfromB(kTRUE);
  SetRejectQuarkNotFound(kTRUE);
  SetKeepOnlyDfromB(kFALSE);
}

/// Calls the base class method (needed to avoid shadowing).
///
/// \param Pointer to an AliVParticle object.
Bool_t AliHFAODMCParticleContainer::AcceptMCParticle(AliAODMCParticle* vp)
{
  UInt_t rejectionReason = 0;
  return AliMCParticleContainer::AcceptMCParticle(vp, rejectionReason);
}

/// First check whether the particle is a "special" PDG particle (in which case the particle is accepted)
/// or a daughter of a "special" PDG particle (in which case the particle is rejected);
/// then calls the base class AcceptParticle(Int_t i) method.
///
/// \param i Index of the particle to be checked.
///
/// \return kTRUE if the particle is accepted, kFALSE otherwise.
Bool_t AliHFAODMCParticleContainer::AcceptMCParticle(Int_t i)
{
  // Determine whether the MC particle is accepted.
  UInt_t rejectionReason = 0;

  AliAODMCParticle* part = static_cast<AliAODMCParticle*>(fClArray->At(i));

  Int_t partPdgCode = TMath::Abs(part->PdgCode());

  Bool_t isSpecialPdg = (fSpecialPDG != 0 && partPdgCode == fSpecialPDG && part->IsPrimary());

  if (isSpecialPdg) {
    AliAnalysisTaskDmesonJets::EMesonOrigin_t origin = AliAnalysisTaskDmesonJets::AnalysisEngine::CheckOrigin(part, fClArray);

    if ((origin & fRejectedOrigin) != 0) isSpecialPdg = kFALSE;

    AliAnalysisTaskDmesonJets::EMesonDecayChannel_t decayChannel = AliAnalysisTaskDmesonJets::AnalysisEngine::CheckDecayChannel(part, fClArray);

    if ((decayChannel & fAcceptedDecay) == 0) isSpecialPdg = kFALSE;

    AliDebug(2, Form("Including particle %d (PDG = %d, pT = %.3f, eta = %.3f, phi = %.3f)",
                     part->Label(), partPdgCode, part->Pt(), part->Eta(), part->Phi()));

    // Special PDG particle. Apply generator cut and kinematic cuts.

    if (fGeneratorIndex >= 0 && fGeneratorIndex != part->GetGeneratorIndex()) {
      rejectionReason |= kMCGeneratorCut;
      return kFALSE;
    }

    AliTLorentzVector mom;
    GetMomentum(mom, i);
    return ApplyKinematicCuts(mom, rejectionReason);
  }

  if (IsSpecialPDGDaughter(part)) {
    rejectionReason = kHFCut;
    return kFALSE;  // daughter of a special PDG particle, reject it.
  }

  // Not a special PDG particle, and not a daughter of a special PDG particle. Apply regular cuts.
  return AliMCParticleContainer::AcceptMCParticle(i, rejectionReason);
}

/// Check if particle it's a daughter of a "special" PDG particle: AOD mode
/// \param part Pointer to a valid AliAODMCParticle object
///
/// \result kTRUE if it is a daughter of the "special" PDG particle, kFALSE otherwise
Bool_t AliHFAODMCParticleContainer::IsSpecialPDGDaughter(AliAODMCParticle* part) const
{
  if (fSpecialPDG == 0) return kTRUE;
  
  AliAODMCParticle* pm = part;
  Int_t imo = -1;
  while (pm != 0) {
    imo = pm->GetMother();
    if (imo < 0) break;
    pm = static_cast<AliAODMCParticle*>(fClArray->At(imo));
    if (TMath::Abs(pm->GetPdgCode()) == fSpecialPDG && pm->IsPrimary()) {
      AliDebug(2, Form("Rejecting particle (PDG = %d, pT = %.3f, eta = %.3f, phi = %.3f) daughter of %d (PDG = %d, pT = %.3f, eta = %.3f, phi = %.3f)",
                       part->PdgCode(), part->Pt(), part->Eta(), part->Phi(), imo, pm->PdgCode(), pm->Pt(), pm->Eta(), pm->Phi()));
      return kTRUE;
    }
  }
  return kFALSE;
}

