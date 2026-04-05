#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Core/FileBlock.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art_root_io/TFileService.h"
#include "detdataformats/trigger/TriggerActivityData.hpp"

#include <fstream>
#include <iostream>
#include <string>

namespace dune {

class TACountDumper : public art::EDAnalyzer {
public:
    explicit TACountDumper(fhicl::ParameterSet const& pset);
    void analyze(art::Event const& evt) override;
    void respondToOpenInputFile(art::FileBlock const& fb) override;
    void beginJob() override;
    void endJob() override;

private:
    art::InputTag fTA10kTag;
    art::InputTag fTA15kTag;
    art::InputTag fTA20kTag;
    std::string   fOutCSV;
    std::ofstream fCSV;
    int           fFileIdx;
    std::string   fCurrentFile;
};

TACountDumper::TACountDumper(fhicl::ParameterSet const& pset)
    : EDAnalyzer(pset)
    , fTA10kTag(pset.get<art::InputTag>("ta_10k_tag", "tamakerTPC10k"))
    , fTA15kTag(pset.get<art::InputTag>("ta_15k_tag", "tamakerTPC15k"))
    , fTA20kTag(pset.get<art::InputTag>("ta_20k_tag", "tamakerTPC20k"))
    , fOutCSV(pset.get<std::string>("output_csv", "ta_counts.csv"))
    , fFileIdx(-1)
{}

void TACountDumper::beginJob() {
    fCSV.open(fOutCSV);
    fCSV << "file_idx,filename,run,subrun,event,nTA_10k,nTA_15k,nTA_20k\n";
}

void TACountDumper::respondToOpenInputFile(art::FileBlock const& fb) {
    fFileIdx++;
    fCurrentFile = fb.fileName();
    std::cout << "[TACountDumper] file_idx=" << fFileIdx
              << " -> " << fCurrentFile << std::endl;
}

void TACountDumper::analyze(art::Event const& evt) {
    int nTA_10k = 0, nTA_15k = 0, nTA_20k = 0;

    auto h10k = evt.getHandle<std::vector<dunedaq::trgdataformats::TriggerActivityData>>(fTA10kTag);
    if (h10k.isValid()) nTA_10k = h10k->size();

    auto h15k = evt.getHandle<std::vector<dunedaq::trgdataformats::TriggerActivityData>>(fTA15kTag);
    if (h15k.isValid()) nTA_15k = h15k->size();

    auto h20k = evt.getHandle<std::vector<dunedaq::trgdataformats::TriggerActivityData>>(fTA20kTag);
    if (h20k.isValid()) nTA_20k = h20k->size();

    fCSV << fFileIdx << "," << fCurrentFile << ","
         << evt.run() << "," << evt.subRun() << "," << evt.event() << ","
         << nTA_10k << "," << nTA_15k << "," << nTA_20k << "\n";
}

void TACountDumper::endJob() {
    fCSV.close();
    std::cout << "TACountDumper: wrote " << fOutCSV << std::endl;
}

} // namespace dune

DEFINE_ART_MODULE(dune::TACountDumper)
