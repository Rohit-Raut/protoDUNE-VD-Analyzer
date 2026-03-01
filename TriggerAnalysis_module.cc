#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art_root_io/TFileService.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "lardataobj/RawData/RawDigit.h"
#include "lardata/DetectorInfoServices/DetectorClocksService.h"
#include "detdataformats/trigger/TriggerPrimitive.hpp"
#include "detdataformats/trigger/TriggerActivityData.hpp"
#include "larcore/Geometry/WireReadout.h"

#include "TTree.h"
#include <vector>
#include <cmath>
namespace duneana {
    class TriggerAnalysis : public art::EDAnalyzer {
        public:
            explicit TriggerAnalysis(fhicl::ParameterSet const& p);
            void beginJob() override;
            void analyze(art::Event const& e) override;
            void reset();

        private:
            art::InputTag fTPLabel;
            art::InputTag fTALabel;
            geo::WireReadoutGeom const& fWireReadoutGeom = art::ServiceHandle<geo::WireReadout>()->Get();

            TTree* fEventTree;
            TTree* fTPTree;
            TTree* fTATree;
	    TTree* fAna;

            // Event-level variables
            int fRun, fSubRun, fEvent;
            int fNTPs_total;
            int fNTAs_total;
            double fTotalEventTPADC;
            double fTotalEventTAADC;
            
            // Per-plane counts
            std::vector<int> fNTPs_perPlane;
            std::vector<int> fNTAs_perPlane;
            std::vector<double> fTPADC_perPlane;
            std::vector<double> fTAADC_perPlane;

            // TP-level variables
            uint64_t fTP_TimeStart;
            uint64_t fTP_TimePeak;
            uint64_t fTP_TimeOverThreshold;
            uint32_t fTP_Channel;
            uint64_t fTP_ADCPeak;
            uint64_t fTP_ADCIntegral;
            int fTP_Plane;

            // TA-level variables
            double fTA_TimeStart;
            double fTA_TimeEnd;
            double fTA_TimePeak;
            double fTA_ADCPeak;
            double fTA_ADCIntegral;
            int fTA_ChannelStart;
            int fTA_ChannelEnd;
            int fTA_ChannelPeak;
            int fTA_Plane;

	    double fEventTACharge;
	    double fMaxTAADC;
	    bool fTrigFired;
	    double fTrigADCThreshold;
    };
}

duneana::TriggerAnalysis::TriggerAnalysis(fhicl::ParameterSet const& p)
    : EDAnalyzer(p),
      fTPLabel(p.get<art::InputTag>("TPLabel")),
      fTALabel(p.get<art::InputTag>("TALabel")),
      fNTPs_perPlane(3, 0),
      fNTAs_perPlane(3, 0),
      fTPADC_perPlane(3, 0.0),
      fTAADC_perPlane(3, 0.0),
      fTrigADCThreshold(p.get<double>("TrigADCThreshold", 8.0e6))
{}

void duneana::TriggerAnalysis::beginJob() {
    art::ServiceHandle<art::TFileService> tfs;
    
    // Event-level tree
    fEventTree = tfs->make<TTree>("EventTree", "Event-level TP/TA Summary");
    fEventTree->Branch("run", &fRun, "run/I");
    fEventTree->Branch("subrun", &fSubRun, "subrun/I");
    fEventTree->Branch("event", &fEvent, "event/I");
    fEventTree->Branch("nTPs_total", &fNTPs_total, "nTPs_total/I");
    fEventTree->Branch("nTAs_total", &fNTAs_total, "nTAs_total/I");
    fEventTree->Branch("totalTPADC", &fTotalEventTPADC, "totalTPADC/D");
    fEventTree->Branch("totalTAADC", &fTotalEventTAADC, "totalTAADC/D");
    fEventTree->Branch("nTPs_perPlane", &fNTPs_perPlane);
    fEventTree->Branch("nTAs_perPlane", &fNTAs_perPlane);
    fEventTree->Branch("TPADC_perPlane", &fTPADC_perPlane);
    fEventTree->Branch("TAADC_perPlane", &fTAADC_perPlane);

    // TP-level tree
    fTPTree = tfs->make<TTree>("TPTree", "Trigger Primitive Data");
    fTPTree->Branch("run", &fRun, "run/I");
    fTPTree->Branch("subrun", &fSubRun, "subrun/I");
    fTPTree->Branch("event", &fEvent, "event/I");
    fTPTree->Branch("timeStart", &fTP_TimeStart, "timeStart/l");
    fTPTree->Branch("timePeak", &fTP_TimePeak, "timePeak/l");
    fTPTree->Branch("timeOverThreshold", &fTP_TimeOverThreshold, "timeOverThreshold/l");
    fTPTree->Branch("channel", &fTP_Channel, "channel/i");
    fTPTree->Branch("adcPeak", &fTP_ADCPeak, "adcPeak/l");
    fTPTree->Branch("adcIntegral", &fTP_ADCIntegral, "adcIntegral/l");
    fTPTree->Branch("plane", &fTP_Plane, "plane/I");

    // TA-level tree
    fTATree = tfs->make<TTree>("TATree", "Trigger Activity Data");
    fTATree->Branch("run", &fRun, "run/I");
    fTATree->Branch("subrun", &fSubRun, "subrun/I");
    fTATree->Branch("event", &fEvent, "event/I");
    fTATree->Branch("timeStart", &fTA_TimeStart, "timeStart/D");
    fTATree->Branch("timeEnd", &fTA_TimeEnd, "timeEnd/D");
    fTATree->Branch("timePeak", &fTA_TimePeak, "timePeak/D");
    fTATree->Branch("adcPeak", &fTA_ADCPeak, "adcPeak/D");
    fTATree->Branch("adcIntegral", &fTA_ADCIntegral, "adcIntegral/D");
    fTATree->Branch("channelStart", &fTA_ChannelStart, "channelStart/I");
    fTATree->Branch("channelEnd", &fTA_ChannelEnd, "channelEnd/I");
    fTATree->Branch("channelPeak", &fTA_ChannelPeak, "channelPeak/I");
    fTATree->Branch("plane", &fTA_Plane, "plane/I");

    fAna = tfs->make<TTree>("Ana", "TA-Based Trigger Efficiency");
    fAna->Branch("run", 	&fRun, "run/I");
    fAna->Branch("subrun", 	&fSubRun, "subrun/I");
    fAna->Branch("event", 	&fEvent, "event/I");
    
    fAna->Branch("totalTAADC", 	&fMaxTAADC, "totalTAADC/D");
    fAna->Branch("maxTAADC", 	&fMaxTAADC, "maxTAADX/D");
    fAna->Branch("trigFired", 	&fTrigFired, "trigFired/O");

}

void duneana::TriggerAnalysis::analyze(art::Event const& e) {
    fRun = e.run();
    fSubRun = e.subRun();
    fEvent = e.id().event();
    
    std::cout << "\n=== Processing Event " << fEvent << " ===" << std::endl;
    
    reset();

    // Process Trigger Primitives
    auto tpHandle = e.getHandle<std::vector<dunedaq::trgdataformats::TriggerPrimitive>>(fTPLabel);
    if (tpHandle.isValid()) {
        std::cout << "Found " << tpHandle->size() << " Trigger Primitives" << std::endl;
        
        for (const auto& tp : *tpHandle) {
            fTP_Channel = tp.channel;
            int plane = fWireReadoutGeom.View(tp.channel);
            if (plane < 0 || plane > 2) continue;
            
            fTP_Plane = plane;
            fTP_TimeStart = tp.time_start;
            fTP_TimePeak = tp.time_peak;
            fTP_TimeOverThreshold = tp.time_over_threshold;
            fTP_ADCPeak = tp.adc_peak;
            fTP_ADCIntegral = tp.adc_integral;
            
            fNTPs_total++;
            fNTPs_perPlane[plane]++;
            fTotalEventTPADC += tp.adc_integral;
            fTPADC_perPlane[plane] += tp.adc_integral;
            
            fTPTree->Fill();
        }
        
        std::cout << "TP counts per plane - U: " << fNTPs_perPlane[0] 
                  << ", V: " << fNTPs_perPlane[1] 
                  << ", Z: " << fNTPs_perPlane[2] << std::endl;
        std::cout << "Total TP ADC: " << fTotalEventTPADC << std::endl;
    } else {
        std::cout << "WARNING: No Trigger Primitives found with label: " << fTPLabel << std::endl;
    }

    // Process Trigger Activities
    auto const& clocks = art::ServiceHandle<detinfo::DetectorClocksService const>()->DataFor(e);
    double tick_us = clocks.TPCClock().TickPeriod();
    int in5ms = std::lround(2500.0/tick_us);
    auto taHandle = e.getHandle<std::vector<dunedaq::trgdataformats::TriggerActivityData>>(fTALabel);
    if (taHandle.isValid()) {
        std::cout << "Found " << taHandle->size() << " Trigger Activities" << std::endl;
        
        for (const auto& ta : *taHandle) {
            int plane = fWireReadoutGeom.View(ta.channel_peak);
            if (plane < 0 || plane > 2) continue;
	    static int debug_count=0;
	    
	    std::cout << "TA[" << debug_count << "] time_peak=" << ta.time_peak 
                          << " abs=" << std::abs(static_cast<int>(ta.time_peak)) 
                          << " window=" << in5ms << std::endl;
	    


	    ++fNTAs_total;
	    fTotalEventTAADC += ta.adc_integral;
	    fTAADC_perPlane[plane] += ta.adc_integral;
	    fNTAs_perPlane[plane]++;
            fTA_Plane = plane;
            fTA_TimeStart = ta.time_start;
            fTA_TimeEnd = ta.time_end;
            fTA_TimePeak = ta.time_peak;
            fTA_ADCPeak = ta.adc_peak;
            fTA_ADCIntegral = ta.adc_integral;
            fTA_ChannelStart = ta.channel_start;
            fTA_ChannelEnd = ta.channel_end;
            fTA_ChannelPeak = ta.channel_peak;
            
            
            fTATree->Fill();
        }
        
        std::cout << "TA counts per plane - U: " << fNTAs_perPlane[0] 
                  << ", V: " << fNTAs_perPlane[1] 
                  << ", Z: " << fNTAs_perPlane[2] << std::endl;
        std::cout << "Total TA ADC: " << fTotalEventTAADC << std::endl;
    } else {
        std::cout << "WARNING: No Trigger Activities found with label: " << fTALabel << std::endl;
    }
    
    std::cout << "Event Summary - TPs: " << fNTPs_total << ", TAs: " << fNTAs_total << std::endl;
    
    fEventTree->Fill();
}

void duneana::TriggerAnalysis::reset() {
    fNTPs_total = 0;
    fNTAs_total = 0;
    fTotalEventTPADC = 0.0;
    fTotalEventTAADC = 0.0;
    
    std::fill(fNTPs_perPlane.begin(), fNTPs_perPlane.end(), 0);
    std::fill(fNTAs_perPlane.begin(), fNTAs_perPlane.end(), 0);
    std::fill(fTPADC_perPlane.begin(), fTPADC_perPlane.end(), 0.0);
    std::fill(fTAADC_perPlane.begin(), fTAADC_perPlane.end(), 0.0);
}

DEFINE_ART_MODULE(duneana::TriggerAnalysis)
