// -*- C++ -*-
//
// Package:    CalibPPS/TimingCalibration/PPSDiamondSampicTimingCalibrationPCLWorker
// Class:      PPSDiamondSampicTimingCalibrationPCLWorker
//
/**\class PPSDiamondSampicTimingCalibrationPCLWorker PPSDiamondSampicTimingCalibrationPCLWorker.cc CalibPPS/TimingCalibration/PPSDiamondSampicTimingCalibrationPCLWorker/plugins/PPSDiamondSampicTimingCalibrationPCLWorker.cc

 Description: Worker of DiamondSampic calibration which produces RecHitsTime histograms and id mapping for the Harvester

 Implementation:
     [Notes on implementation]
*/
//
// Original Author:  Christopher Misan
//         Created:  Mon, 26 Jul 2021 07:37:13 GMT
//
//

#include <string>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "DQMServices/Core/interface/DQMGlobalEDAnalyzer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "Geometry/VeryForwardGeometryBuilder/interface/CTPPSGeometry.h"
#include "Geometry/Records/interface/VeryForwardRealGeometryRecord.h"

#include "DataFormats/CTPPSDetId/interface/TotemTimingDetId.h"
#include "DataFormats/CTPPSDetId/interface/CTPPSDiamondDetId.h"
#include "DataFormats/CTPPSDigi/interface/TotemTimingDigi.h"
#include "DataFormats/CTPPSReco/interface/TotemTimingRecHit.h"
#include "DataFormats/Common/interface/DetSetVector.h"

//------------------------------------------------------------------------------

struct Histograms_PPSDiamondSampicTimingCalibrationPCLWorker {
  std::unordered_map<uint32_t, dqm::reco::MonitorElement*> timeHisto;
  std::unordered_map<uint32_t, dqm::reco::MonitorElement*> db;
  std::unordered_map<uint32_t, dqm::reco::MonitorElement*> sampic;
  std::unordered_map<uint32_t, dqm::reco::MonitorElement*> channel;
};

class PPSDiamondSampicTimingCalibrationPCLWorker
    : public DQMGlobalEDAnalyzer<Histograms_PPSDiamondSampicTimingCalibrationPCLWorker> {
public:
  explicit PPSDiamondSampicTimingCalibrationPCLWorker(const edm::ParameterSet&);
  ~PPSDiamondSampicTimingCalibrationPCLWorker() override;

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  void bookHistograms(DQMStore::IBooker&,
                      edm::Run const&,
                      edm::EventSetup const&,
                      Histograms_PPSDiamondSampicTimingCalibrationPCLWorker&) const override;

  void dqmAnalyze(edm::Event const&,
                  edm::EventSetup const&,
                  Histograms_PPSDiamondSampicTimingCalibrationPCLWorker const&) const override;

  // ------------ member data ------------
  edm::EDGetTokenT<edm::DetSetVector<TotemTimingDigi>> totemTimingDigiToken_;
  edm::EDGetTokenT<edm::DetSetVector<TotemTimingRecHit>> totemTimingRecHitToken_;
  edm::ESGetToken<CTPPSGeometry, VeryForwardRealGeometryRecord> geomEsToken_;
  std::string folder_;
};

//------------------------------------------------------------------------------

PPSDiamondSampicTimingCalibrationPCLWorker::PPSDiamondSampicTimingCalibrationPCLWorker(const edm::ParameterSet& iConfig)
    : totemTimingDigiToken_(
          consumes<edm::DetSetVector<TotemTimingDigi>>(iConfig.getParameter<edm::InputTag>("totemTimingDigiTag"))),
      totemTimingRecHitToken_(
          consumes<edm::DetSetVector<TotemTimingRecHit>>(iConfig.getParameter<edm::InputTag>("totemTimingRecHitTag"))),
      geomEsToken_(esConsumes<edm::Transition::BeginRun>()),
      folder_(iConfig.getParameter<std::string>("folder")) {}

PPSDiamondSampicTimingCalibrationPCLWorker::~PPSDiamondSampicTimingCalibrationPCLWorker() {}

//------------------------------------------------------------------------------

void PPSDiamondSampicTimingCalibrationPCLWorker::dqmAnalyze(
    edm::Event const& iEvent,
    edm::EventSetup const& iSetup,
    Histograms_PPSDiamondSampicTimingCalibrationPCLWorker const& histos) const {
  edm::Handle<edm::DetSetVector<TotemTimingDigi>> timingDigi;
  edm::Handle<edm::DetSetVector<TotemTimingRecHit>> timingRecHit;
  iEvent.getByToken(totemTimingRecHitToken_, timingRecHit);
  iEvent.getByToken(totemTimingDigiToken_, timingDigi);

  if (timingRecHit->empty()) {
    edm::LogWarning("PPSDiamondSampicTimingCalibrationPCLWorker:dqmAnalyze")
        << "No rechits retrieved from the event content.";
    return;
  }

  for (const auto& digis : *timingDigi) {
    const CTPPSDiamondDetId detId(digis.detId());
    for (const auto& digi : digis) {
      histos.db.at(detId.rawId())->Fill(digi.hardwareBoardId());
      histos.sampic.at(detId.rawId())->Fill(digi.hardwareSampicId());
      histos.channel.at(detId.rawId())->Fill(digi.hardwareChannelId());
    }
  }

  for (const auto& recHits : *timingRecHit) {
    const CTPPSDiamondDetId detId(recHits.detId());
    for (const auto& recHit : recHits)
      histos.timeHisto.at(detId.rawId())->Fill(recHit.time());
  }
}

//------------------------------------------------------------------------------

void PPSDiamondSampicTimingCalibrationPCLWorker::bookHistograms(
    DQMStore::IBooker& ibook,
    edm::Run const& run,
    edm::EventSetup const& iSetup,
    Histograms_PPSDiamondSampicTimingCalibrationPCLWorker& histos) const {
  ibook.setCurrentFolder(folder_);
  std::string ch_name;
  const auto& geom = iSetup.getData(geomEsToken_);
  for (auto it = geom.beginSensor(); it != geom.endSensor(); ++it) {
    if (!CTPPSDiamondDetId::check(it->first))
      continue;
    const CTPPSDiamondDetId detid(it->first);

    std::string path;
    detid.channelName(path, CTPPSDiamondDetId::nPath);
    detid.channelName(ch_name);
    histos.timeHisto[detid.rawId()] = ibook.book1D(path + "/" + ch_name, ch_name, 500, -25, 25);
    histos.db[detid.rawId()] = ibook.bookInt(path + "/" + ch_name + "db");
    histos.sampic[detid.rawId()] = ibook.bookInt(path + "/" + ch_name + "sampic");
    histos.channel[detid.rawId()] = ibook.bookInt(path + "/" + ch_name + "channel");
  }
}

//------------------------------------------------------------------------------

void PPSDiamondSampicTimingCalibrationPCLWorker::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>("totemTimingDigiTag", edm::InputTag("totemTimingRawToDigi", "TotemTiming"))
      ->setComment("input tag for the PPS diamond detectors digi");
  desc.add<edm::InputTag>("totemTimingRecHitTag", edm::InputTag("totemTimingRecHits"))
      ->setComment("input tag for the PPS diamond detectors rechits");
  desc.add<std::string>("folder", "AlCaReco/PPSDiamondSampicTimingCalibrationPCL")
      ->setComment("output path for the various DQM plots");
  descriptions.add("PPSDiamondSampicTimingCalibrationPCLWorker", desc);
}

DEFINE_FWK_MODULE(PPSDiamondSampicTimingCalibrationPCLWorker);
