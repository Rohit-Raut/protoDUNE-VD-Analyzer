#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art_root_io/TFileService.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "detdataformats/trigger/TriggerPrimitive.hpp"
#include "detdataformats/trigger/TriggerActivityData.hpp"
#include "lardataobj/RawData/RawDigit.h"
#include "lardataobj/Simulation/SimEnergyDeposit.h"
// LArSoft data products
#include "nusimdata/SimulationBase/MCParticle.h"
#include "larcore/Geometry/Geometry.h"
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h"
#include "larcore/Geometry/WireReadout.h"
#include "lardata/Utilities/LArFFT.h"
#include "lardataobj/RawData/raw.h"
// ROOT includes
#include "TTree.h"
#include "TLorentzVector.h"
#include "TVector3.h"
#include <array>
#include <vector>
#include <string>
#include <cmath>
#include <limits>
#include <algorithm>
constexpr int kMaxRawChannels   = 13000;
constexpr int kMaxRawTicks      = 9600;

namespace duneana{
    class cosmicAnalysis: public art::EDAnalyzer{
        public:
            explicit cosmicAnalysis(fhicl::ParameterSet const& p);
            cosmicAnalysis(cosmicAnalysis const&)                   = delete;
            cosmicAnalysis(cosmicAnalysis&&)                        = delete;
            cosmicAnalysis& operator = (cosmicAnalysis const&)	    = delete;
            cosmicAnalysis& operator = (cosmicAnalysis&&)           = delete;
            void beginJob() override;
            void analyze(art::Event const& e) override;
            void reset();
        private:
            art::InputTag fMCParticleTag;
	        art::InputTag fTPLabel;
	        art::InputTag fTALabel;
            //art::InputTag fRawDigitLabel;
		art::InputTag fSedLabel;
	    TTree* fTree;
	    TTree* fTreeTP;
	    TTree* fTreeTA;
	    TTree* fAna;
            //TTree* fRawDigitTree;
            geo::WireReadoutGeom const& fWireReadoutGeom=art::ServiceHandle<geo::WireReadout>()->Get();
	    
	    //Optimizing the code to check for TPC boundary condition
	    struct TPCBounds{
		double minX, maxX, minY, maxY, minZ, maxZ;
	    };
	    std::vector<TPCBounds> fCachedTPCBounds;
	    std::unordered_map<int, int> fChannelToPlane;
	    std::map<int, std::vector<const simb::MCParticle*>> fTrackIdToSecondaries;



            //recording data
            int fRun;
            int fSubRun;
            int fEvent;
            
            //double rawMax = std::numeric_limits<double>::lowest();
            //just to make sure the detector geometry is fine
            std::string fDetectorName;
            double fDetHalfWidth;
            double fDetHalfHeight;
            double fDetLength;

            int fNPrimaries;
            std::vector<int> fPrimPdg;
            std::vector<double> fPrimE;
            std::vector<double> fPrimVx, fPrimVy, fPrimVz;
            std::vector<double> fPrimPx, fPrimPy, fPrimPz;
            std::vector<double> fTrackLengthInTPC;
            std::vector<double> fTPCEntryX, fTPCEntryY, fTPCEntryZ;
            std::vector<double> fTPCExitX, fTPCExitY, fTPCExitZ;
            int fNSecondaries;
            std::vector<int>    fSecondaryPdg;
            std::vector<double> fSecondaryE;
            std::vector<double> fSecondaryVx, fSecondaryVy, fSecondaryVz;
		    std::vector<int>    fTPCParticlePdg;
            std::vector<double> fTPCParticleE;
            //TP data storage
	    std::vector<uint64_t> fTPTimeStart, fTPTimePeak, fTPTimeOverThreshold, fTPChannel;
	    std::vector<Double_t> fTPADCPeak, fTPADCSum, fTPDetId;
    	    //uint64_t fTPTimeStart;
    	    //uint64_t fTPTimePeak;
    	    //uint64_t fTPTimeOverThreshold;
    	    //uint32_t    fTPChannel;
    	    //uint64_t fTPADCPeak;
    	    //uint64_t fTPADCSum;
            //uint64_t fTPDetId;
            
	    std::vector<double>fADCIntegralDAQ; 
	    
	    std::vector<uint64_t> fTANTPs, fTAChannelStart, fTAChannelEnd, fTAChannelPeak;
	    std::vector<double> fTATimeStart, fTATimePeak, fTAADCPeak, fTAADCSum, fTATimeEnd;
	    //TA information
    	    //int fTANTPs;
    	    //double fTATimeStart;
    	    //double fTATimeEnd;
    	    //double fTATimePeak;
    	    //double fTAADCPeak;

	    double fTotalEventADC;
	    double fTotalEventTA;

	    double fEventTACharge;
            bool fTrigFired;
            double fTrigADCThreshold;
            double EDep;
	    //Storing the raw didigt data
           // int fRaw_nChan;
           // int fRaw_channel[kMaxRawChannels];
           // int fRaw_plane[kMaxRawChannels];
           // int fRaw_ADCs[kMaxRawChannels][kMaxRawTicks];
    };
}
duneana::cosmicAnalysis::cosmicAnalysis(fhicl::ParameterSet const& p)
    :EDAnalyzer(p),
    fMCParticleTag(p.get<art::InputTag>("MCParticleTag")),
    fTPLabel(p.get<art::InputTag>("TPLabel")),
    fTALabel(p.get<art::InputTag>("TALabel")), 
    fSedLabel(p.get<art::InputTag>("SEDLabel", art::InputTag("largeant", "LArG4DetectorServicevolTPCActive"))),
    fTrigADCThreshold(p.get<double>("TrigADCThreshold", 8.0e6))
     //fRawDigitLabel(p.get<art::InputTag>("RawDigitLabel"))
{}



void duneana::cosmicAnalysis::beginJob(){
    art::ServiceHandle<art::TFileService> tfs;
    fTree = tfs->make<TTree>("cosmicTree", "Analysis");
    fTree->Branch("run",        &fRun,      "run/I");
    fTree->Branch("subrun",     &fSubRun,   "subrun/I");
    fTree->Branch("event",      &fEvent,     "event/I");

    fTree->Branch("detectorName",   &fDetectorName);
    fTree->Branch("detHalfWidth",   &fDetHalfWidth,     "detHalfWidth/D");
    fTree->Branch("detHalfHeight",  &fDetHalfHeight,    "detHalfHeight/D");
    fTree->Branch("detLength",      &fDetLength,        "detLength/D");

    
    // Store ALL primaries as vectors
    fTree->Branch("nPrimaries", &fNPrimaries, "nPrimaries/I");
    fTree->Branch("primPdg",    &fPrimPdg);
    fTree->Branch("primE",      &fPrimE);
    fTree->Branch("primVx",     &fPrimVx);
    fTree->Branch("primVy",     &fPrimVy);
    fTree->Branch("primVz",     &fPrimVz);
    fTree->Branch("primPx",     &fPrimPx);
    fTree->Branch("primPy",     &fPrimPy);
    fTree->Branch("primPz",     &fPrimPz);
    fTree->Branch("tracklength",&fTrackLengthInTPC);
    fTree->Branch("tpcEntryX",  &fTPCEntryX);
    fTree->Branch("tpcEntryY",  &fTPCEntryY);
    fTree->Branch("tpcEntryZ",  &fTPCEntryZ);
    fTree->Branch("tpcExitX",   &fTPCExitX);
    fTree->Branch("tpcExitY",   &fTPCExitY);
    fTree->Branch("tpcExitZ",   &fTPCExitZ);
    fTree->Branch("tpcE",       &fTPCParticleE);


    fTree->Branch("nSecondaries",   &fNSecondaries, "nSecondaries/I");
    fTree->Branch("secondaryPdg",   &fSecondaryPdg);
    fTree->Branch("secondaryE",     &fSecondaryE);
    fTree->Branch("secondaryVx",    &fSecondaryVx);
    fTree->Branch("secondaryVy",    &fSecondaryVy);
    fTree->Branch("secondaryVz",    &fSecondaryVz);
    fTree->Branch("TPCPdg",         &fTPCParticlePdg);
    fTree->Branch("totalEventADC",  &fTotalEventADC);
    fTree->Branch("totalEventTA", &fTotalEventTA);
    fTreeTP = tfs->make<TTree>("TP", "analysis");

    fTreeTP->Branch("run", 		&fRun, 		"run/I");
    fTreeTP->Branch("subrun", 		&fSubRun, 	"subrun/I");
    fTreeTP->Branch("event", 		&fEvent, 	"event/I");
    fTreeTP->Branch("TPStart", 		&fTPTimeStart);
    fTreeTP->Branch("TPPeak", 		&fTPTimePeak);
    fTreeTP->Branch("TPSum", 		&fTPADCSum);
    fTreeTP->Branch("TPTimeOverThreshold", &fTPTimeOverThreshold);
    fTreeTP->Branch("TPChannel", 	&fTPChannel);
    fTreeTP->Branch("TPADCPeak", 	&fTPADCPeak);
    fTreeTP->Branch("TPDetId", 		&fTPDetId);
    fTreeTP->Branch("tp_adcIntegral", 	&fADCIntegralDAQ);
    //fTreeTP->Branch("totalEventADC", 	&fTotalEventADC);


    //TA information is also recorded in TPTree
    fTreeTA = tfs->make<TTree>("TA", "analysis");
    fTreeTA->Branch("event", 		&fEvent); 	
    fTreeTA->Branch("TAnum", 		&fTANTPs); 	
    fTreeTA->Branch("TAStart", 		&fTATimeStart); 	
    fTreeTA->Branch("TAEnd", 		&fTATimeEnd); 	
    fTreeTA->Branch("TAPeak", 		&fTATimePeak); 	
    fTreeTA->Branch("TAADCPeak", 	&fTAADCPeak);	
    fTreeTA->Branch("TASum", 		&fTAADCSum);	
    fTreeTA->Branch("TAChannelStart", 	&fTAChannelStart);
    fTreeTA->Branch("TAChannelEnd",	&fTAChannelEnd);
    fTreeTA->Branch("TAChannelPeak", 	&fTAChannelPeak);

    art::ServiceHandle<geo::Geometry> geom;
    mf::LogInfo("CosmicAnalysis")<<"Caching TPC Geometry Bounds";
    for (geo::TPCGeo const& tpc: geom->Iterate<geo::TPCGeo>()){
	auto const center = tpc.GetCenter();
	double halfHeight = tpc.HalfHeight();
	double halfLenght = tpc.HalfLength();
	double driftDist = tpc.DriftDistance();

	TPCBounds bounds;
	bounds.minX = center.X() - driftDist;
	bounds.maxX = center.X() + driftDist;
	bounds.minY = center.Y() - halfHeight;
	bounds.maxY = center.Y() + halfHeight;
	bounds.minZ = center.Z() - halfLenght;
	bounds.maxZ = center.Z() + halfLenght;

	fCachedTPCBounds.push_back(bounds);
    }
    mf::LogInfo("CosmicAnalysis")<<"Cashed "<< fCachedTPCBounds.size()<< " TPC Bounds ";
    mf::LogInfo("CosmicAnalysis")<<"Cashing Channel To Plane Mapping";
    size_t nChannels = fWireReadoutGeom.Nchannels();
    for (raw::ChannelID_t channel= 0; channel<nChannels; ++channel){
	int plane = fWireReadoutGeom.View(channel);
	if(plane>=0 && plane<3){
	    fChannelToPlane[channel] = plane;
	}
    }
    mf::LogInfo("CosmicAnalysis")<<"Cashed"<<fChannelToPlane.size()<<"channel->plane Mapping";
    std::cout<<"\n============Detector TPC Geometry=============="<<std::endl;
    for(size_t i=0; i<fCachedTPCBounds.size();++i){
	const auto& bounds = fCachedTPCBounds[i];
	std::cout<<"TPC"<<i <<":"<<std::endl;
	std::cout << "  X range: [" << bounds.minX << ", " << bounds.maxX << "]" << std::endl;
        std::cout << "  Y range: [" << bounds.minY << ", " << bounds.maxY << "]" << std::endl;
        std::cout << "  Z range: [" << bounds.minZ << ", " << bounds.maxZ << "]" << std::endl;
    }
    std::cout << "====================================================\n" << std::endl;
    
    fAna = tfs->make<TTree>("Ana", "TA-Based Trigger Efficiency");
    fAna->Branch("run",         &fRun, "run/I");
    fAna->Branch("subrun",      &fSubRun, "subrun/I");
    fAna->Branch("event",       &fEvent, "event/I");

    fAna->Branch("eventTAADC",  &fEventTACharge, "eventTAADC/D");
    fAna->Branch("trigFired",   &fTrigFired, "trigFired/O");
    fAna->Branch("trigADCThreshold", &fTrigADCThreshold, "trigADCThreshold/D");
    fAna->Branch("EDep", &EDep, "EDep/D");

}


void duneana::cosmicAnalysis::analyze(art::Event const& e){
    fRun    = e.run();
    fSubRun = e.subRun();
    fEvent  = e.id().event();
    reset(); 
    art::ServiceHandle<geo::Geometry> geom;
    // geo::BoxBoundedGeo active_volume = geom->ActiveBoundedBox();
    fDetectorName = geom->DetectorName();
    
    //auto mcParticleHandle = e.getValidHandle<std::vector<simb::MCParticle>>(fMCParticleTag);
    //auto const& mcParticleList = *mcParticleHandle;

    std::cout<<"\n--------------------------------------Cosmic Track Analysis--------------------------------------\n"<<std::endl;
    std::cout<<"Detector Name: "<<fDetectorName<<" .\n"<<std::endl;
    std::cout<<"Using cached geometry: "<<fCachedTPCBounds.size()<<" TPCs, Half Height: "<<fDetHalfHeight<<", Half Width: "<<fDetHalfWidth<<", Length: "<<fDetLength<<"\n"<<std::endl;

    //int primaryCount = 0;
    
    //Debugginf some of the geometry information
    std::array<size_t, 3> tpCountPerPlane = {0, 0, 0};
    //TP information extraction
    art::Handle<std::vector<dunedaq::trgdataformats::TriggerPrimitive>> tpHandle = e.getHandle<std::vector<dunedaq::trgdataformats::TriggerPrimitive>>(fTPLabel);
    art::Handle<std::vector<sim::SimEnergyDeposit>> sedHandle;
    fTPTimeStart.clear(); fTPTimePeak.clear(); fTPTimeOverThreshold.clear(); fTPChannel.clear();
    fTPADCPeak.clear(); fTPADCSum.clear(); fTPDetId.clear();
    fADCIntegralDAQ.clear();
    if (tpHandle.isValid()){
    	fTotalEventADC=0.0;
	fEventTACharge=0.0;
	for (const dunedaq::trgdataformats::TriggerPrimitive &tp: *tpHandle){
            fTPChannel		.push_back(tp.channel);
            //int plane = fWireReadoutGeom.View(tp.channel);            
	    auto it = fChannelToPlane.find(tp.channel);
	    if(it==fChannelToPlane.end() || it->second<0 || it->second>2) continue;
	    int plane = it->second;
            fTPTimeStart 	.push_back(tp.time_start);
            fTPTimePeak  	.push_back(tp.time_peak);
            fTPTimeOverThreshold.push_back(tp.time_over_threshold);
            fTPADCPeak 		.push_back(tp.adc_peak);
            fADCIntegralDAQ 	.push_back(tp.adc_integral);
            fTPDetId		.push_back(tp.detid);
	    fTotalEventADC	+= tp.adc_integral;
	    fEventTACharge 	+= tp.adc_integral;		//this is redundant and ik it it just for convenient for now and will be modified later on
            ++tpCountPerPlane[plane];
            


        }
    }
    else{
      mf::LogWarning("TP Analysis")<< "No Trigger Primitive Found:  "<<fTPLabel<<"..........\n";
    }
    fTreeTP->Fill();
    mf::LogInfo("CosmicAnalysis")<< "TP counts per plane → "<< "U=" << tpCountPerPlane[0] << ", V=" << tpCountPerPlane[1] << ", "<< "Z=" << tpCountPerPlane[2];

    

    fTANTPs.clear(); fTAChannelStart.clear(); fTAChannelEnd.clear(); fTAChannelPeak.clear();
    fTATimeStart.clear(); fTATimePeak.clear(); fTAADCPeak.clear(); fTAADCSum.clear(); fTATimeEnd.clear();
    art::Handle<std::vector<dunedaq::trgdataformats::TriggerActivityData>> taHandle = e.getHandle<std::vector<dunedaq::trgdataformats::TriggerActivityData>>(fTALabel);
    if(!taHandle.isValid()){
        mf::LogWarning("CosmicAnalysis")<<"No TA found recheck again";
    }
    std::array<size_t, 3> taCountPerPlane = {};
    if(taHandle.isValid()){
      fTotalEventTA=0;
      for (const auto& ta: *taHandle){
        //fTANTPs 	= ta.num_tps;
        fTATimeStart.push_back(ta.time_start);
        fTATimeEnd.push_back(ta.time_end);
        fTATimePeak.push_back(ta.time_peak);
        fTAADCPeak.push_back(ta.adc_peak);
        fTAADCSum.push_back(ta.adc_integral);
        fTAChannelStart.push_back(ta.channel_start);
        fTAChannelEnd.push_back(ta.channel_end);
        fTAChannelPeak.push_back(ta.channel_peak);
	fTotalEventTA 	+=ta.adc_integral;
	int planeno = -1;
        auto taIt = fChannelToPlane.find(ta.channel_peak);
	if(taIt !=fChannelToPlane.end()) planeno = taIt->second;
	if(planeno>=0 && planeno<3)++taCountPerPlane[planeno];

      }
      double maxTA = fTAADCSum.empty()?0.0: *std::max_element(fTAADCSum.begin(), fTAADCSum.end());
      fTrigFired = (maxTA>=fTrigADCThreshold);
    }
    else{
      mf::LogWarning("TA analysis")<<"No Trigger Activity Detected: "<<fTALabel<<" .........\n";
    }
    //mf::LogInfo("CosmicAnalysis")<<"TA Count per plane -> U: "<<taCountPerPlane[0]<<", V: "<<taCountPerPlane[1]<<" , Z: "<<taCountPerPlane[2]; 

//    fPrimPdg.clear();
    //fPrimE.clear();
    //fTPCParticlePdg.clear();
    //fPrimVx.clear(); fPrimVy.clear(); fPrimVz.clear();
    //fPrimPx.clear(); fPrimPy.clear(); fPrimPz.clear();
    //fTrackLengthInTPC.clear();
    //fTPCEntryX.clear(); fTPCEntryY.clear(); fTPCEntryZ.clear();
    //fTPCExitX.clear(); fTPCExitY.clear(); fTPCExitZ.clear();
    //fSecondaryPdg.clear();
    //fSecondaryE.clear();
    //fSecondaryVx.clear(); fSecondaryVy.clear(); fSecondaryVz.clear();
    //fTPCParticleE.clear(); 
    //for (auto const& particle: mcParticleList){

    //  if(particle.Mother() ==0)
    //  {		
    //    primaryCount++;
    //    fPrimPdg.push_back(particle.PdgCode());
    //    fPrimE.push_back(particle.E());
    //    fPrimVx.push_back(particle.Vx());
    //    fPrimVy.push_back(particle.Vy());
    //    fPrimVz.push_back(particle.Vz());
    //    fPrimPx.push_back(particle.Px());
    //    fPrimPy.push_back(particle.Py());
    //    fPrimPz.push_back(particle.Pz());



    //    double tracklength       = 0.0;
    //    TVector3 tpcEntryPoint(-999,-999,-999), tpcExitPoint(-999,-999,-999);

    //    bool enterTPC           = false;
    //    for (size_t i=0; i<particle.NumberTrajectoryPoints(); ++i){
    //      TVector3 currentPoint = particle.Position(i).Vect();
    //      bool isInside = false;

    //      //checking if the point are inside any of the TPCs
    //      for(const auto& bounds: fCachedTPCBounds){
    //        if(currentPoint.X()>=bounds.minX && currentPoint.X()<=bounds.maxX &&
    //    	    currentPoint.Y()>=bounds.minY && currentPoint.Y()<=bounds.maxY&&
    //    	    currentPoint.Z()>=bounds.minZ && currentPoint.Z()<=bounds.maxZ){
    //    	isInside = true;
    //    	break;
    //        }
    //      }

    //      if(isInside){
    //        if(!enterTPC){
    //          tpcEntryPoint 	= currentPoint;
    //          enterTPC 	=true;
    //        }
    //        tpcExitPoint = currentPoint;
    //      }
    //    }
    //    if(enterTPC){
    //        fTPCParticlePdg.push_back(particle.PdgCode());
    //        fTPCEntryX.push_back(tpcEntryPoint.X());
    //        fTPCEntryY.push_back(tpcEntryPoint.Y());
    //        fTPCEntryZ.push_back(tpcEntryPoint.Z());
    //        fTPCExitX.push_back(tpcExitPoint.X());
    //        fTPCExitY.push_back(tpcExitPoint.Y());
    //        fTPCExitZ.push_back(tpcExitPoint.Z());
    //        fTPCParticleE.push_back(particle.E());
    //        tracklength     =(tpcExitPoint-tpcEntryPoint).Mag();
    //        
    //        fTrackLengthInTPC.push_back(tracklength);
    //    }
    //    }  
    //}
    //std::cout<<"\n Number of Primary Recorded: "<<primaryCount<<".\n"<<std::endl;
    //fNPrimaries = primaryCount;
    //fNSecondaries = fSecondaryPdg.size();
    EDep = 0.0;
    //art::Handle<std::vector<sim::SimEnergyDeposit>> sedHandle;
    if (e.getByLabel(fSedLabel, sedHandle)) {
        for (auto const& sed : *sedHandle) {
            auto const& pos = sed.MidPoint();
            double x = pos.X();
            double y = pos.Y();
            double z = pos.Z();

            bool inside = false;
            for (auto const& bounds : fCachedTPCBounds) {
                if (x >= bounds.minX && x <= bounds.maxX &&
                    y >= bounds.minY && y <= bounds.maxY &&
                    z >= bounds.minZ && z <= bounds.maxZ) {
                    inside = true;
                    break;
                }
            }
            if (!inside) continue;

            // sed.Energy() is in MeV → convert to GeV
            EDep += sed.Energy() * 1.0e-3;
        }
    }
    else {
        mf::LogWarning("CosmicAnalysis")
          << "No SimEnergyDeposit product found for label " << fSedLabel;
    }
    
    fTree->Fill();
    fAna->Fill();



}






void duneana::cosmicAnalysis::reset(){
    fTotalEventADC = 0.0;
    fTotalEventTA = 0.0;
    fEventTACharge = 0.0;
    fTrigFired = false;
    EDep = 0.0;
}



DEFINE_ART_MODULE(duneana::cosmicAnalysis)
