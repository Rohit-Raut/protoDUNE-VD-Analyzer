////////////////////////////////////////////////////////////////////////
/// @file    RawDigitOverlay_module.cc
/// @brief   Overlays RawDigits from a secondary file (cosmic+nu)
///          with primary input (HNL) by adding ADC waveforms
///          channel by channel, tick by tick.
/// @author  Rohit Raut
/// @date    2026
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art_root_io/TFileService.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "lardataobj/RawData/RawDigit.h"
#include "lardataobj/RawData/raw.h"

#include "TFile.h"
#include "TTree.h"
#include "TRandom3.h"

#include <vector>
#include <string>
#include <map>
#include <memory>


namespace pdvd{
    class RawDigitOverlay : public art::EDProducer{
	public:
	    explicit RawDigitOverlay(fhicl::ParameterSet const& p);
	    void produce(art::Event& e) override;
	    void beginJob() override;
	private:
	    art::InputTag fPrimaryLabel; // HNL Raw didigt label
	    std::string fSecondaryFile; // path to cosmic + nu file
	    std::string fSecondaryLabel;  // branhc name for secondary file
	    int fSeed;

	    TFile* fBGFile = nullptr;
	    TFile* FBGTree = nullptr;
	    Long64_t fNBGEvents = 0;

	    std::vector<raw::RawDigit>* fBGDigit = nullptr;

	    TRandom3 fRNG;
	    raw::RawDigit mergeDigit(const raw::RawDigit& primary, const raw::RawDigit& secondary) const;


    };
    RawDigitOverlay::RawDigitOverlay(fhich::ParameterSet const& p)
	:EDProducer{p}, 
	fPrimaryLabel{p.get<art::InputTag>("PrimaryLabel", "tpcrawdecorder:daq")}, 
	fSecondaryFile{p.get<art::string> ("SecondaryFile")},
	fSecondaryLabel{p.get<art::string> ("SecondaryLabel", "raw::RawDigit_tpcrawdecorder_daq_DetsimCosmic")},
	fSeed {p.get<int> ("seed", 42)},
	fRNG {(UInt_t) fSeed}
    {
	produces<std::vector<raw::RawDigit>>("overlay");
    }

    void RawDigitOverlay::beginJob()
    {
	mf::LogInf("RawDigitOverlay")<<"Opening secondary file: "<<fSecondaryFile;
	fBGFile = TFile::Open(fSecondaryFile.c_str(), "READ");
	if(!fBGFile || fBGFile->IsZombie()){
	    throw cet::exception("RawDigitOverlay")<<"Cannot access secondary file"<<fSecondaryFile;
	}

	fBGTree = (TTree*) fBGTree->Get("Events");
	if(!fBGTree){
	    throw cet::exception("RawDigitOverlay")<<"Cannot find the event TTree";
	}
	fBGTree->SetBranchAddress(fSecondaryLabel.c_str(), &fBGDigits);
	fNBGEvents = fBGTree->GetEntries();
	mf::LogInfo("RawDigitOverlay")<<"Secondary File has:"<<fNBGEvents<<" events.";
    }
    raw::RawDigit RawDigitOverlay::MergeDigit(const raw::RawDigit& primary, const raw::RawDigit& secondary) cosnt
    {
	const size_t nSamples = primary.Samples();
	raw::Digit::ADCvector_t adcPrimary(nSamples);
	raw::Uncompress(priamry.ADCs(), adcPriamry, primary.Compression());

	//uncompress secondary
	
    }

}
