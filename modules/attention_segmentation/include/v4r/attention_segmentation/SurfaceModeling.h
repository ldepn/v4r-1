/****************************************************************************
**
** Copyright (C) 2017 TU Wien, ACIN, Vision 4 Robotics (V4R) group
** Contact: v4r.acin.tuwien.ac.at
**
** This file is part of V4R
**
** V4R is distributed under dual licenses - GPLv3 or closed source.
**
** GNU General Public License Usage
** V4R is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published
** by the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** V4R is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** Please review the following information to ensure the GNU General Public
** License requirements will be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
**
** Commercial License Usage
** If GPL is not suitable for your project, you must purchase a commercial
** license to use V4R. Licensees holding valid commercial V4R licenses may
** use this file in accordance with the commercial license agreement
** provided with the Software or, alternatively, in accordance with the
** terms contained in a written agreement between you and TU Wien, ACIN, V4R.
** For licensing terms and conditions please contact office<at>acin.tuwien.ac.at.
**
**
** The copyright holder additionally grants the author(s) of the file the right
** to use, copy, modify, merge, publish, distribute, sublicense, and/or
** sell copies of their contributions without any restrictions.
**
****************************************************************************/

/**
 * @file SurfaceModeling.h
 * @author Richtsfeld
 * @date October 2012
 * @version 0.1
 * @brief Models patches using NURBS.
 */

#ifndef SURFACE_SURFACEMODELING_HH
#define SURFACE_SURFACEMODELING_HH

#include <iostream>
#include <opencv2/opencv.hpp>

#include <pcl/ModelCoefficients.h>
#include <pcl/common/time.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/filters/project_inliers.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

#include <v4r/attention_segmentation/sequential_fitter.h>
#include "v4r/attention_segmentation/EPBase.h"
#include "v4r/attention_segmentation/EPUtils.h"
#include "v4r/attention_segmentation/SurfaceModel.h"

/*
 *@ep: things that are not clear:
 1) savings
 2) some merging procedures for planes
 * */

namespace v4r {

double computeSavings(int numParams, std::vector<double> &probs, double kappa1, double kappa2);
double computePlaneSavingsNormalized(int numParams, std::vector<double> &probs, double norm, double kappa1,
                                     double kappa2);
double computeSavingsNormalized(int numParams, std::vector<double> &probs, double norm, double kappa1, double kappa2);
// computes probabilities, that points belong to the selected model ()
void computePointProbs(std::vector<double> &errs, std::vector<double> &probs, double sigmaError);

struct MergedPair {
  int id1, id2;
  double savings;
};

inline bool cmpSavings(const MergedPair &a, const MergedPair &b) {
  return (a.savings > b.savings);
}

class SurfaceModeling : public EPBase {
 public:
  typedef std::shared_ptr<SurfaceModeling> Ptr;

  class Parameter {
   public:
    pcl::on_nurbs::SequentialFitter::Parameter nurbsParams;

    double sigmaError;  // error for gaussian error model
    double kappa1;      // base cost (0.002, 0.005 1 cyl nurb)
    double kappa2;      // weights the error (0.8)
    double plane_savings;
    double bspline_savings;
    int planePointsFixation;  // classified planes will not be merged with b-splines anymore (5000 for 640x480)
    double z_max;             // Maximum z-value for 3D neighborhood

    Parameter(pcl::on_nurbs::SequentialFitter::Parameter nurbs = pcl::on_nurbs::SequentialFitter::Parameter(),
              double _sigmaError = 0.003, double _kappa1 = 0.003, double _kappa2 = 0.9, int _pPF = 5000,
              double _z_max = 0.01)
    : nurbsParams(nurbs), sigmaError(_sigmaError), kappa1(_kappa1), kappa2(_kappa2), planePointsFixation(_pPF),
      z_max(_z_max) {}
  };

 private:
  bool tryMergePlanes, tryMergeNurbs, msCheck;

  bool haveIntr, haveExtr;
  Eigen::Matrix4d camIntr;
  Eigen::Matrix4d camExtr;

  bool have_surfaces;

  std::vector<SurfaceModel::Ptr> surfaces;  //< Created surfaces

  bool have_boundary2D, have_boundary3D;
  std::map<v4r::borderIdentification, std::vector<v4r::neighboringPair>> ngbr2D_map;
  std::map<v4r::borderIdentification, std::vector<v4r::neighboringPair>> ngbr3D_map;

  bool filter_by_size;
  int minSurfaceSize;

  std::vector<int> addedTo;

  // inits all necessary data structures
  void init();
  // compute a least square plane from a set of points
  void computeLeastSquarePlane(SurfaceModel::Ptr plane);
  // fits plane to the patch
  void fitPlane(SurfaceModel::Ptr plane);
  // fits NURBS to the plane
  void fitNurbs(SurfaceModel::Ptr model);
  // replace a model with a NURBS if savings are better
  bool replacePlaneWithBetterNurbs(SurfaceModel::Ptr &surf);
  // merges two models if savings are better
  bool tryMergeSurfaces(SurfaceModel::Ptr surf1, SurfaceModel::Ptr surf2, SurfaceModel::Ptr &mergedSurf);
  bool tryMergeSurfacesWithPlanes(SurfaceModel::Ptr surf1, SurfaceModel::Ptr surf2, SurfaceModel::Ptr &mergedSurf);
  // selects the best model for the patch (plane or NURBS) and merges neigboring pathces if there is a benefit
  void modelSelection();
  // computes the error from the point to the model
  void computePointError(SurfaceModel::Ptr surf);
  // computes errors for each point in the patch and probabilities to belong to the plane
  void initSurface(SurfaceModel::Ptr surface);
  // merge planes using nurbs
  void mergeWithNurbs(std::vector<MergedPair> &mergePairs);
  void mergeWithPlanes(std::vector<MergedPair> &mergePairs);
  // modify neighbors
  void modifyNeighbours(int oldIdx, int newIdx);
  // remove neighbors
  void modifyBoundary(unsigned int oldIdx, unsigned int newIdx);

  // create neighbors
  cv::Mat neigbouring_matrix2D, neigbouring_matrix3D;
  void createNeighbours();
  void copySurfaces();

 public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  Parameter param;

  SurfaceModeling(Parameter p = Parameter());
  ~SurfaceModeling();

  /** Set surfaces **/
  void setSurfaces(const std::vector<SurfaceModel::Ptr> _surfaces);
  /** Set boundary 2D **/
  void setBoundary2D(const std::map<borderIdentification, std::vector<neighboringPair>> _ngbr2D_map);
  /** Set boundary 3D **/
  void setBoundary3D(const std::map<borderIdentification, std::vector<neighboringPair>> _ngbr3D_map);

  /** set intrinsic camera parameter **/
  void setIntrinsic(double fx, double fy, double cx, double cy);
  /** set extrinsic camera parameter **/
  void setExtrinsic(Eigen::Matrix4d &pose);

  /** set minimum number of points in the surface for it to be valid **/
  void setMinSurfaceSize(int _minSurfaceSize);

  /** compute surface modeling **/
  virtual void compute();
  void pruneSurfaces();

  /** Return computed surfaces **/
  inline std::vector<SurfaceModel::Ptr> getSurfaces();
  /** Return modified boundaries **/
  inline std::map<borderIdentification, std::vector<neighboringPair>> getBoundary2D();
  inline std::map<borderIdentification, std::vector<neighboringPair>> getBoundary3D();

  /** Return computed surfaces **/
  inline std::vector<int> getAddedTo();

  void setTryMergePlanes(bool _tryMergePlanes) {
    tryMergePlanes = _tryMergePlanes;
  };
  void setTryMergeNurbs(bool _tryMergeNurbs) {
    tryMergeNurbs = _tryMergeNurbs;
  };
  void setMsCheck(bool _msCheck) {
    msCheck = _msCheck;
  };

  bool getTryMergePlanes() {
    return tryMergePlanes;
  };
  bool getTryMergeNurbs() {
    return tryMergeNurbs;
  };
  bool getMsCheck() {
    return msCheck;
  };

  void printErrorsAndProbs(std::string file_name);
  void printNeigbours(std::string file_name);
  void printSurfaces(std::string file_name);
};

inline std::vector<SurfaceModel::Ptr> SurfaceModeling::getSurfaces() {
  return surfaces;
}

inline std::map<borderIdentification, std::vector<neighboringPair>> SurfaceModeling::getBoundary2D() {
  return ngbr2D_map;
}

inline std::map<borderIdentification, std::vector<neighboringPair>> SurfaceModeling::getBoundary3D() {
  return ngbr3D_map;
}

inline std::vector<int> SurfaceModeling::getAddedTo() {
  return addedTo;
}
}  // namespace v4r

#endif
