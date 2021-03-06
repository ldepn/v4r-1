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
 * @file main.cpp
 * @author Johann Prankl (prankl@acin.tuwien.ac.at)
 * @date 2017
 * @brief
 *
 */

#ifndef KP_PLANE_ESTIMATION_RANSAC_HH
#define KP_PLANE_ESTIMATION_RANSAC_HH

#include <v4r/core/macros.h>
#include <Eigen/Dense>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace v4r {

/**
 * PlaneEstimationRANSAC
 */
class V4R_EXPORTS PlaneEstimationRANSAC {
 public:
  class V4R_EXPORTS Parameter {
   public:
    double inl_dist;
    double eta_ransac;         // eta for pose ransac
    unsigned max_rand_trials;  // max. number of trials for pose ransac

    Parameter(double _inl_dist = 0.01, double _eta_ransac = 0.01, unsigned _max_rand_trials = 10000)
    : inl_dist(_inl_dist), eta_ransac(_eta_ransac), max_rand_trials(_max_rand_trials) {}
  };

 private:
  void computeCovarianceMatrix(const std::vector<Eigen::Vector3f> &pts, const std::vector<int> &indices,
                               const Eigen::Vector3f &mean, Eigen::Matrix3f &cov);
  void getInliers(std::vector<float> &dists, std::vector<int> &inliers);
  unsigned countInliers(std::vector<float> &dists);
  void getRandIdx(int size, int num, std::vector<int> &idx);
  void ransac(const std::vector<Eigen::Vector3f> &pts, Eigen::Vector3f &pt, Eigen::Vector3f &n,
              std::vector<int> &inliers);

  inline bool contains(const std::vector<int> &idx, int num);
  inline float sqr(const float &d) {
    return d * d;
  }

 public:
  Parameter param;

  PlaneEstimationRANSAC(const Parameter &p = Parameter());
  ~PlaneEstimationRANSAC();

  void estimatePlaneLS(const std::vector<Eigen::Vector3f> &pts, const std::vector<int> &indices, Eigen::Vector3f &pt,
                       Eigen::Vector3f &n);

  void ransacPlane(const std::vector<Eigen::Vector3f> &pts, Eigen::Vector3f &pt, Eigen::Vector3f &n,
                   std::vector<int> &inliers);

  inline void explicitToImplicit(const Eigen::Vector3f &pt1, const Eigen::Vector3f &pt2, const Eigen::Vector3f &pt3,
                                 float &a, float &b, float &c, float &d);
  inline void explicitToNormal(const Eigen::Vector3f &pt1, const Eigen::Vector3f &pt2, const Eigen::Vector3f &pt3,
                               Eigen::Vector3f &n);
  inline float implicitPointDist(const float &a, const float &b, const float &c, const float &d,
                                 const Eigen::Vector3f &pt);
  static inline float normalPointDist(const Eigen::Vector3f &pt, const Eigen::Vector3f &n,
                                      const Eigen::Vector3f &pt_dist);
  static void getDistances(const std::vector<Eigen::Vector3f> &pts, const Eigen::Vector3f &pt, const Eigen::Vector3f &n,
                           std::vector<float> &dists);

  typedef std::shared_ptr<::v4r::PlaneEstimationRANSAC> Ptr;
  typedef std::shared_ptr<::v4r::PlaneEstimationRANSAC const> ConstPtr;
};

/*********************** INLINE METHODES **************************/
inline bool PlaneEstimationRANSAC::contains(const std::vector<int> &idx, int num) {
  for (unsigned i = 0; i < idx.size(); i++)
    if (idx[i] == num)
      return true;
  return false;
}

/**
 * explicitToImplicit
 */
inline void PlaneEstimationRANSAC::explicitToImplicit(const Eigen::Vector3f &pt1, const Eigen::Vector3f &pt2,
                                                      const Eigen::Vector3f &pt3, float &a, float &b, float &c,
                                                      float &d) {
  a = (pt2[1] - pt1[1]) * (pt3[2] - pt1[2]) - (pt2[2] - pt1[2]) * (pt3[1] - pt1[1]);

  b = (pt2[2] - pt1[2]) * (pt3[0] - pt1[0]) - (pt2[0] - pt1[0]) * (pt3[2] - pt1[2]);

  c = (pt2[0] - pt1[0]) * (pt3[1] - pt1[1]) - (pt2[1] - pt1[1]) * (pt3[0] - pt1[0]);

  d = -pt2[0] * a - pt2[1] * b - pt2[2] * c;
}

/**
 * explicitToNormal
 */
inline void PlaneEstimationRANSAC::explicitToNormal(const Eigen::Vector3f &pt1, const Eigen::Vector3f &pt2,
                                                    const Eigen::Vector3f &pt3, Eigen::Vector3f &n) {
  float norm;

  n[0] = (pt2[1] - pt1[1]) * (pt3[2] - pt1[2]) - (pt2[2] - pt1[2]) * (pt3[1] - pt1[1]);

  n[1] = (pt2[2] - pt1[2]) * (pt3[0] - pt1[0]) - (pt2[0] - pt1[0]) * (pt3[2] - pt1[2]);

  n[2] = (pt2[0] - pt1[0]) * (pt3[1] - pt1[1]) - (pt2[1] - pt1[1]) * (pt3[0] - pt1[0]);

  norm = sqrt(sqr(n[0]) + sqr(n[1]) + sqr(n[2]));

  if (fabs(norm) <= std::numeric_limits<float>::epsilon()) {
    n[0] = n[1] = n[2] = std::numeric_limits<float>::quiet_NaN();
    ;
  } else {
    norm = 1. / norm;
    n[0] *= norm;
    n[1] *= norm;
    n[2] *= norm;
  }
}

/**
 * implicitPointDist
 */
inline float PlaneEstimationRANSAC::implicitPointDist(const float &a, const float &b, const float &c, const float &d,
                                                      const Eigen::Vector3f &pt) {
  return fabs(a * pt[0] + b * pt[1] + c * pt[2] + d) / sqrt(a * a + b * b + c * c);
}

/**
 * normalPointDist
 */
inline float PlaneEstimationRANSAC::normalPointDist(const Eigen::Vector3f &pt, const Eigen::Vector3f &n,
                                                    const Eigen::Vector3f &pt_dist) {
  return (pt_dist - pt).dot(n);
}
}  // namespace v4r

#endif
