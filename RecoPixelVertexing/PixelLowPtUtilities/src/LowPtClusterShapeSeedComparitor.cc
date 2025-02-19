#include <cmath>

#include "DataFormats/GeometryVector/interface/Basic2DVector.h"
#include "DataFormats/GeometryVector/interface/GlobalPoint.h"
#include "DataFormats/GeometryVector/interface/GlobalVector.h"
#include "DataFormats/TrackerRecHit2D/interface/SiPixelRecHit.h"
#include "DataFormats/TrackingRecHit/interface/TrackingRecHit.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/ESInputTag.h"
#include "RecoPixelVertexing/PixelLowPtUtilities/interface/HitInfo.h"
#include "RecoPixelVertexing/PixelLowPtUtilities/interface/LowPtClusterShapeSeedComparitor.h"
#include "RecoPixelVertexing/PixelTrackFitting/interface/CircleFromThreePoints.h"
#include "RecoTracker/TkSeedingLayers/interface/SeedingHitSet.h"

namespace {
  typedef Basic2DVector<float> Vector2D;

  inline float sqr(float x) { return x * x; }

  /*****************************************************************************/
  inline float areaParallelogram(const Vector2D& a, const Vector2D& b) { return a.x() * b.y() - a.y() * b.x(); }

  /*****************************************************************************/

  inline bool getGlobalDirs(GlobalPoint const* g, GlobalVector* globalDirs) {
    // Determine circle
    CircleFromThreePoints circle(g[0], g[1], g[2]);

    float curvature = circle.curvature();
    if (0.f == curvature) {
      LogDebug("LowPtClusterShapeSeedComparitor")
          << "the curvature is null:"
          << "\n point1: " << g[0] << "\n point2: " << g[1] << "\n point3: " << g[2];
      return false;
    }

    // Get 2d points
    Vector2D p[3];
    Vector2D c = circle.center();
    for (int i = 0; i != 3; i++)
      p[i] = g[i].basicVector().xy() - c;

    float area = std::abs(areaParallelogram(p[1] - p[0], p[1]));

    float a12 = std::asin(std::min(area * curvature * curvature, 1.f));

    float slope = (g[1].z() - g[0].z()) / a12;

    // Calculate globalDirs

    float cotTheta = slope * curvature;
    float sinTheta = 1.f / std::sqrt(1.f + sqr(cotTheta));
    float cosTheta = cotTheta * sinTheta;

    if (areaParallelogram(p[0], p[1]) < 0)
      sinTheta = -sinTheta;

    for (int i = 0; i != 3; i++) {
      Vector2D vl = p[i] * (curvature * sinTheta);
      globalDirs[i] = GlobalVector(-vl.y(), vl.x(), cosTheta);
    }
    return true;
  }

  /*****************************************************************************/

  inline void getGlobalPos(const SeedingHitSet& hits, GlobalPoint* globalPoss) {
    for (unsigned int i = 0; i != hits.size(); ++i)
      globalPoss[i] = hits[i]->globalPosition();
  }

}  // namespace

/*****************************************************************************/
LowPtClusterShapeSeedComparitor::LowPtClusterShapeSeedComparitor(const edm::ParameterSet& ps,
                                                                 edm::ConsumesCollector& iC)
    : thePixelClusterShapeCacheToken(
          iC.consumes<SiPixelClusterShapeCache>(ps.getParameter<edm::InputTag>("clusterShapeCacheSrc"))),
      theShapeFilterLabel_(ps.getParameter<std::string>("clusterShapeHitFilter")),
      clusterShapeHitFilterESToken_(iC.esConsumes(edm::ESInputTag("", theShapeFilterLabel_))),
      trackerTopologyESToken_(iC.esConsumes()) {}

/*****************************************************************************/
void LowPtClusterShapeSeedComparitor::init(const edm::Event& e, const edm::EventSetup& es) {
  clusterShapeHitFilter_ = &es.getData(clusterShapeHitFilterESToken_);
  trackerTopology_ = &es.getData(trackerTopologyESToken_);
  e.getByToken(thePixelClusterShapeCacheToken, thePixelClusterShapeCache);
}

bool LowPtClusterShapeSeedComparitor::compatible(const SeedingHitSet& hits) const
//(const reco::Track* track, const vector<const TrackingRecHit *> & recHits) const
{
  assert(hits.size() == 3);

  if (clusterShapeHitFilter_ == nullptr)
    throw cms::Exception("LogicError") << "LowPtClusterShapeSeedComparitor: init(EventSetup) method was not called";

  // Get global positions
  GlobalPoint globalPoss[3];
  getGlobalPos(hits, globalPoss);

  // Get global directions
  GlobalVector globalDirs[3];

  bool ok = getGlobalDirs(globalPoss, globalDirs);

  // Check whether shape of pixel cluster is compatible
  // with local track direction

  if (!ok) {
    LogDebug("LowPtClusterShapeSeedComparitor") << "curvarture 0:"
                                                << "\nnHits: " << hits.size() << " will say the seed is good anyway.";
    return true;
  }

  for (int i = 0; i < 3; i++) {
    const SiPixelRecHit* pixelRecHit = dynamic_cast<const SiPixelRecHit*>(hits[i]->hit());

    if (!pixelRecHit) {
      edm::LogError("LowPtClusterShapeSeedComparitor") << "this is not a pixel cluster";
      ok = false;
      break;
    }

    if (!pixelRecHit->isValid()) {
      ok = false;
      break;
    }

    LogDebug("LowPtClusterShapeSeedComparitor") << "about to compute compatibility."
                                                << "hit ptr: " << pixelRecHit << "global direction:" << globalDirs[i];

    if (!clusterShapeHitFilter_->isCompatible(*pixelRecHit, globalDirs[i], *thePixelClusterShapeCache)) {
      LogTrace("LowPtClusterShapeSeedComparitor")
          << " clusShape is not compatible" << HitInfo::getInfo(*hits[i]->hit(), trackerTopology_);

      ok = false;
      break;
    }
  }

  return ok;
}
