#include "SimG4CMS/HGCalTestBeam/interface/AHCalSD.h"
#include "SimG4Core/Notification/interface/TrackInformation.h"
#include "DataFormats/HcalDetId/interface/HcalDetId.h"

#include "G4LogicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4ParticleTable.hh"
#include "G4VProcess.hh"

#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"

#include <iomanip>
//#define EDM_ML_DEBUG

AHCalSD::AHCalSD(G4String name, const DDCompactView & cpv,
		 const SensitiveDetectorCatalog & clg,
		 edm::ParameterSet const & p, const SimTrackManager* manager) : 
  CaloSD(name, cpv, clg, p, manager,
         (float)(p.getParameter<edm::ParameterSet>("AHCalSD").getParameter<double>("TimeSliceUnit")),
         p.getParameter<edm::ParameterSet>("AHCalSD").getParameter<bool>("IgnoreTrackID")) {

  edm::ParameterSet m_HC = p.getParameter<edm::ParameterSet>("AHCalSD");
  useBirk          = m_HC.getParameter<bool>("UseBirkLaw");
  birk1            = m_HC.getParameter<double>("BirkC1")*(CLHEP::g/(CLHEP::MeV*CLHEP::cm2));
  birk2            = m_HC.getParameter<double>("BirkC2");
  birk3            = m_HC.getParameter<double>("BirkC3");
  eminHit          = m_HC.getParameter<double>("EminHit")*CLHEP::MeV;

  edm::LogInfo("HcalSim") << "AHCalSD::  Use of Birks law is set to      " 
                          << useBirk << "  with three constants kB = "
                          << birk1 << ", C1 = " << birk2 << ", C2 = " << birk3
			  << "\nAHCalSD:: Threshold for storing hits: "
                          << eminHit << std::endl;
}

AHCalSD::~AHCalSD() { }

double AHCalSD::getEnergyDeposit(G4Step* aStep) {

  double destep = aStep->GetTotalEnergyDeposit();
  double weight(1.0);
#ifdef EDM_ML_DEBUG
  double weight0 = weight;
#endif
  if (useBirk) weight *= getAttenuation(aStep, birk1, birk2, birk3);
#ifdef EDM_ML_DEBUG
  edm::LogInfo("HcalSim") << "AHCalSD: weight " << weight0 << " " << weight 
			  << std::endl;
#endif
  double edep = weight*destep;
  return edep;
}

uint32_t AHCalSD::setDetUnitId(G4Step * aStep) { 

  G4StepPoint* preStepPoint = aStep->GetPreStepPoint(); 
  const G4VTouchable* touch = preStepPoint->GetTouchable();
  G4ThreeVector hitPoint    = preStepPoint->GetPosition();

  int depth = (touch->GetReplicaNumber(1));
  int incol = ((touch->GetReplicaNumber(0))%10);
  int inrow = ((touch->GetReplicaNumber(0))/10)%10;
  int jncol = ((touch->GetReplicaNumber(0))/100)%10;
  int jnrow = ((touch->GetReplicaNumber(0))/1000)%10;
  int col   = (jncol == 0) ? incol : incol+10;
  int row   = (jnrow == 0) ? inrow : inrow+10;
  uint32_t index = HcalDetId(HcalOther,row,col,depth).rawId();
#ifdef EDM_ML_DEBUG
  edm::LogInfo("HcalSim") << "AHCalSD: det = " << HcalOther 
                          << " depth = " << depth << " row = " << row 
                          << " column = " << col << " packed index = 0x" 
			  << std::hex << index << std::dec << std::endl;
  bool flag = unpackIndex(index, row, col, depth);
  edm::LogInfo("HcalSim") << "Results from unpacker for 0x" << std::hex 
			  << index << std::dec << " Flag " << flag << " Row " 
			  << row << " Col " << col << " Depth " << depth 
			  << std::endl;
#endif
  return index;
}

bool AHCalSD::unpackIndex(const uint32_t& idx, int& row, int& col, int& depth) {

  row = col = depth = 0;
  bool rcode(false);
  HcalSubdetector subdet=(HcalSubdetector((idx>>DetId::kSubdetOffset)&0x7));
  if (subdet == HcalOther) {
    if ((idx&HcalDetId::kHcalIdFormat2) != 0) {
      int ieta = (idx>>HcalDetId::kHcalEtaOffset2)&HcalDetId::kHcalEtaMask2;
      row      = (ieta%10);
      if ((ieta/10) != 0) row = -row;
      int iphi = idx&HcalDetId::kHcalPhiMask2;
      col      = (iphi%10);
      if ((iphi/10) != 0) col = -col;
      depth    = (idx>>HcalDetId::kHcalDepthOffset2)&HcalDetId::kHcalDepthMask2;
      rcode    = true;
    }
  }
#ifdef EDM_ML_DEBUG
  edm::LogInfo("HcalSim") << "AHCalSD: packed index = 0x" << std::hex << idx 
			  << std::dec << " Row " << row << " Column " << col
			  << " Depth " << depth << std::endl;
#endif
  return rcode;
}
    
bool AHCalSD::filterHit(CaloG4Hit* aHit, double time) {
  return ((time <= tmaxHit) && (aHit->getEnergyDeposit() > eminHit));
}
