#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art_root_io/TFileService.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
//#include "lardataobj/RawData/RawDigit.h"
//#include "lardata/DetectorInfoServices/DetectorClocksService.h"
//#include "detdataformats/trigger/TriggerPrimitive2.hpp"
//#include "TriggerPrimitive2.hpp"
#include "detdataformats/trigger/TriggerActivityData.hpp"
//#include "larcore/Geometry/WireReadout.h"
#include "lardataobj/RecoBase/Hit.h"

#include "TTree.h"
#include <vector>
#include <cstddef>
#include <Rtypes.h>
namespace duneana{
    class hitdQ: public art::EDAnalyzer{
        public:
                explicit hitdQ(fhicl::ParameterSet const& p);
                void beginJob() override;
                void analyze(art::Event const& e) override;
                void reset();

        private:
                //art::InputTag fTALabel;
                art::InputTag fTALabel;
                //art::InputTag fHitLabel;
                TTree* fRaw;
                TTree* fHit; 
                TTree* fTA;
        int fRun, fEvent, fSubRun;




	std::vector<uint16_t> fTAVersion;
        std::vector<uint64_t> fTA_time_start;
        std::vector<uint64_t> fTA_time_end;
        std::vector<uint64_t> fTA_time_peak;
        //uint64>_t fTA_time_activity;
        std::vector<uint32_t> fTA_channel_start;
        std::vector<uint32_t> fTA_channel_end;
        std::vector<uint32_t> fTA_channel_peak;

        std::vector<uint64_t> fTA_adc_integral;
        std::vector<uint16_t> fTA_adc_peak;

        std::vector<uint32_t> fTA_detid;
        std::vector<uint16_t> fTA_type;

	//UInt_t    fChannel;
	//UShort_t fTPC, fPlane;
	//Int_t    fWire;
	//Int_t fView;
	//Int_t    fStartTick, fEndTick;
	//Float_t  fPeakTime, fIntegral, fPeakAmplitude, fRMS;

    };
}

duneana::hitdQ::hitdQ(fhicl::ParameterSet const& p)
    :EDAnalyzer(p),
    fTALabel(p.get<art::InputTag>("TALabel"))
    //fHitLabel(p.get<art::InputTag>("HitLabel"))
    //fTPLabel(p.get<art::InputTag>("TPLabel"))
{}

void duneana::hitdQ::beginJob(){
    art::ServiceHandle<art::TFileService> tfs;
    fRaw = tfs->make<TTree>("Raw Data", "Raw digit");
    fRaw->Branch("run",         &fRun, "run/I");
    fRaw->Branch("event",       &fEvent, "event/I");
    fRaw->Branch("subrun",      &fSubRun, "subrun/I");


    fHit = tfs->make<TTree>("Hit", "Hit Digit");
    fHit->Branch("run",  &fRun,  "run/I");
    fHit->Branch("subrun",      &fSubRun, "subrun/I");
    fHit->Branch("event",       &fEvent,   "event/I");
    //fHit->Branch("Channel", 	&fChannel);
    //fHit->Branch("fStartTick", 	&fStartTick);
    //fHit->Branch("fEndTick",	&fEndTick);
    //fHit->Branch("fPeakTime", 	&fPeakTime);
    //fHit->Branch("fIntegral", 	&fIntegral);
    //fHit->Branch("fView", 	&fView);
    //fHit->Branch("fPeakAmplitude", 	&fPeakAmplitude);
    //fHit->Branch("fRMS", 	&fRMS);
    ////fHit->Branch("fROISummedADC", 	&fROISummedADC);
    ////fHit->Branch("fHitSummedADC", 	&fHitSummedADC);
    ////fHit->Branch("fGoodnessofFit", 	&fGoodnessofFit);
    ////fHit->Branch("fNDF"		&fNDF);
    //fHit->Branch("fWireId",	&fWire);
    //fHit->Branch("Plane",	&fPlane);
    //fHit->Branch("TPC",		&fTPC);
    //fHit->Branch("isValid"	&isValid);




    fTA = tfs->make<TTree>("TATree", "Trigger Activity Datas");
    fTA->Branch("run",          &fRun, "run/I");
    fTA->Branch("subrun",       &fSubRun, "subrun/I");
    fTA->Branch("event",        &fEvent, "event/I");
    fTA->Branch("version",      &fTAVersion, "version/s");
    fTA->Branch("time_start",     &fTA_time_start);
    fTA->Branch("time_end",       &fTA_time_end);
    fTA->Branch("time_peak",      &fTA_time_peak    );

    fTA->Branch("channel_start",  &fTA_channel_start);
    fTA->Branch("channel_end",    &fTA_channel_end);
    fTA->Branch("channel_peak",   &fTA_channel_peak);

    fTA->Branch("adc_integral",   &fTA_adc_integral);
    fTA->Branch("adc_peak",       &fTA_adc_peak);

    fTA->Branch("detid",          &fTA_detid);
    fTA->Branch("type",           &fTA_type);
}

void duneana::hitdQ::analyze(art::Event const&e ){
    fRun        = e.run();
    fSubRun     = e.subRun();
    fEvent      = e.id().event();
    reset();
    //std::cout<<"[Debug line 59] Above to get TP hadnle for fTPLabel: "<<fTPLabel<<", Event: "<<fEvent<<std::endl;
    auto taHandle = e.getHandle<std::vector<dunedaq::trgdataformats::TriggerActivityData>>(fTALabel);
    //auto tpHandle = e.getHandle<std::vector<recob::Hit>>(fHitLabel);
    //std::cout<<"[Debug TA label] event: "<<e.id()<<" hadle.isValid ="<<(taHandle.isValid()?"true" : "false")<<std::endl;
   //std::cout << "Size of struct: " << sizeof(dunedaq::trgdataformats2::TriggerPrimitive) << " bytes" << std::endl;
    if(taHandle.isValid()){
	std::cout<<"[DEBUG TA] Found: " <<taHandle->size()<<" TA Object "<<std::endl;
	for(auto const& ta: *taHandle){
    	    //if(ta.channel_peak>6144)continue;
	    fTAVersion.push_back(ta.version);
	    fTA_time_start.push_back(ta.time_start);
            fTA_time_end.push_back(ta.time_end);
            fTA_time_peak.push_back(ta.time_peak);
            fTA_channel_start.push_back(ta.channel_start);
            fTA_channel_end.push_back(ta.channel_end);
            fTA_channel_peak.push_back(ta.channel_peak);
            fTA_adc_integral.push_back(ta.adc_integral);
            fTA_adc_peak.push_back(ta.adc_peak);
            fTA_detid.push_back(ta.detid);
            fTA_type.push_back(static_cast<uint16_t>(ta.type));  
	}
	fTA->Fill();
    }

    //Hit Information
   // auto hitHandle = e.getValidHandle<std::vector<recob::Hit>>(fHitLabel);
   // for(auto const& hit: *hitHandle){
   //     if(hit.Channel()>6144) continue;
   //     fChannel = hit.Channel();
   //     fStartTick = hit.StartTick();
   //     fEndTick = hit.EndTick();
   //     fPeakTime = hit.PeakTime();
   //     fIntegral = hit.Integral();
   //     fView = hit.View();
   //     fPeakAmplitude = hit.PeakAmplitude();
   //     fRMS = hit.RMS();
   //     auto const& wid = hit.WireID();
   //     fWire = wid.Wire;
   //     fPlane = wid.Plane;
   //     fTPC = wid.TPC;
   //     fHit->Fill();
   //     //if (i++ == 5) break;
   //     //std::cout << "Channel: " << hit.Channel()
   //     //      << ", StartTick: " << hit.StartTick()
   //     //      << ", EndTick: " << hit.EndTick()
   //     //      << "\n";
   //     
   // }





}

void duneana::hitdQ::reset(){
    fTAVersion.clear();
    fTA_time_start.clear();  fTA_time_end.clear();  fTA_time_peak.clear();
    fTA_channel_start.clear(); fTA_channel_end.clear(); fTA_channel_peak.clear();
    fTA_adc_integral.clear();
    fTA_adc_peak.clear();
    fTA_detid.clear();
    fTA_type.clear();
    //fStartTick = 0;
    //fEndTick = 0;
    //fPeakTime = 0;
    //fIntegral = 0;
    //fView = 0;
    //fRMS = 0;
    //fPeakAmplitude = 0;
    //fWire = 0;
    //fPlane = 0;
    //fTPC = 0;


}

DEFINE_ART_MODULE(duneana::hitdQ)
