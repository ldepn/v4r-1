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
 * @file viewport_checker.h
 * @author Martin Velas , Thomas Faeulhammer (faeulhammer@acin.tuwien.ac.at)
 * @date 2016
 * @brief
 *
 */

/*
 * viewport_checker.hpp
 *
 *  Created on: 10 Aug 2015
 *      Author: martin
 */

#ifndef VIEWPORT_CHECKER_HPP_
#define VIEWPORT_CHECKER_HPP_

#include <pcl/point_cloud.h>
#include <vector>

#include <v4r/core/macros.h>

namespace v4r {

template <class PointType>
class V4R_EXPORTS ViewVolume {
 public:
  /**
   * angles in rad
   */
  ViewVolume(double min_dist_, double max_dist_, double h_angle_, double v_angle_, const Eigen::Affine3f &sensor_pose_,
             double tolerance_)
  : min_dist(min_dist_), max_dist(max_dist_), max_sin_h_angle(sin(h_angle_ / 2 - tolerance_)),
    max_sin_v_angle(sin(v_angle_ / 2 - tolerance_)), sensor_pose(sensor_pose_) {}

  int computeVisible(const typename pcl::PointCloud<PointType>::Ptr input, std::vector<bool> &mask) const;

  static ViewVolume<PointType> ofXtion(const Eigen::Affine3f &sensor_pose, double tolerance = 5.0 /*deg*/) {
    static double degToRad = M_PI / 180.0;
    return ViewVolume<PointType>(0.5, 3.5, 58 * degToRad, 45 * degToRad, sensor_pose, tolerance * degToRad);
  }

  pcl::PointCloud<pcl::PointXYZ> getBorders() const;

 protected:
  bool in(const PointType &pt) const {
    double sin_h_angle = fabs(pt.x) / sqrt(pt.x * pt.x + pt.z * pt.z);
    double sin_v_angle = fabs(pt.y) / sqrt(pt.y * pt.y + pt.z * pt.z);

    return (pt.z > min_dist) && (pt.z < max_dist) && (sin_h_angle < max_sin_h_angle) && (sin_v_angle < max_sin_v_angle);
  }

 private:
  double min_dist;
  double max_dist;
  double max_sin_h_angle;
  double max_sin_v_angle;
  Eigen::Affine3f sensor_pose;
};

template <class PointType>
class ViewportChecker {
 public:
  void add(const ViewVolume<PointType> volume) {
    volumes.push_back(volume);
  }

  void getVisibles(const typename pcl::PointCloud<PointType>::Ptr input,
                   typename pcl::PointCloud<PointType>::Ptr visible,
                   typename pcl::PointCloud<PointType>::Ptr nonVisible) const;

 private:
  std::vector<ViewVolume<PointType>> volumes;
};
}  // namespace v4r

#endif /* VIEWPORT_CHECKER_HPP_ */
