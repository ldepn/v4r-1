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

#ifndef KP_OBJECT_TRACKER_MONO_HH
#define KP_OBJECT_TRACKER_MONO_HH

#include <float.h>
#include <v4r/core/macros.h>
#include <v4r/features/FeatureDetector_KD_FAST_IMGD.h>
#include <v4r/reconstruction/KeypointPoseDetector.h>
#include <v4r/reconstruction/LKPoseTracker.h>
#include <v4r/reconstruction/ProjLKPoseTrackerR2.h>
#include <v4r/tracking/KeypointObjectRecognizerR2.h>
#include <Eigen/Dense>
#include <fstream>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <v4r/keypoints/impl/Object.hpp>

namespace v4r {

/**
 * ObjectTrackerMono
 */
class V4R_EXPORTS ObjectTrackerMono {
 public:
  /**
   * Parameter
   */
  class Parameter {
   public:
    double conf_reinit;    // 0.05
    bool do_inc_pyr_lk;    // true
    double min_conf;       // 0.3
    int min_conf_cnt;      // 2
    int min_not_conf_cnt;  // 1
    bool use_codebook;
    KeypointPoseDetector::Parameter kd_param;
    LKPoseTracker::Parameter lk_param;
    ProjLKPoseTrackerR2::Parameter kt_param;
    FeatureDetector_KD_FAST_IMGD::Parameter det_param;
    KeypointObjectRecognizerR2::Parameter or_param;
    Parameter(double _conf_reinit = 0.05, bool _do_inc_pyr_lk = true, double _min_conf = 0.3, int _min_conf_cnt = 3,
              int _min_not_conf_cnt = 1, bool _use_codebook = true,
              const KeypointPoseDetector::Parameter &_kd_param = KeypointPoseDetector::Parameter(),
              const LKPoseTracker::Parameter &_lk_param = LKPoseTracker::Parameter(),
              const ProjLKPoseTrackerR2::Parameter &_kt_param = ProjLKPoseTrackerR2::Parameter(),
              const FeatureDetector_KD_FAST_IMGD::Parameter &_det_param =
                  FeatureDetector_KD_FAST_IMGD::Parameter(250, 1.44, 2, 17, 2),
              const KeypointObjectRecognizerR2::Parameter &_or_param = KeypointObjectRecognizerR2::Parameter())
    : conf_reinit(_conf_reinit), do_inc_pyr_lk(_do_inc_pyr_lk), min_conf(_min_conf), min_conf_cnt(_min_conf_cnt),
      min_not_conf_cnt(_min_not_conf_cnt), use_codebook(_use_codebook), kd_param(_kd_param), lk_param(_lk_param),
      kt_param(_kt_param), det_param(_det_param), or_param(_or_param) {}
  };

 private:
  Parameter param;

  cv::Mat_<double> dist_coeffs;
  cv::Mat_<double> intrinsic;

  cv::Mat_<unsigned char> im_gray;

  ObjectView::Ptr view;
  Object::Ptr model;

  double conf;
  int conf_cnt;
  int not_conf_cnt;

  KeypointPoseDetector::Ptr kpDetector;
  ProjLKPoseTrackerR2::Ptr projTracker;
  LKPoseTracker::Ptr lkTracker;
  KeypointObjectRecognizerR2::Ptr kpRecognizer;

  // CodebookMatcher::Ptr cbMatcher;

  double viewPointChange(const Eigen::Vector3f &pt, const Eigen::Matrix4f &inv_pose1, const Eigen::Matrix4f &inv_pose2);
  double reinit(const cv::Mat_<unsigned char> &im, Eigen::Matrix4f &pose, ObjectView::Ptr &view);
  void updateView(const Eigen::Matrix4f &pose, const Object &model, ObjectView::Ptr &view);

 public:
  cv::Mat dbg;
  // CodebookMatcher::Ptr cbMatcher;

  ObjectTrackerMono(const ObjectTrackerMono::Parameter &p = ObjectTrackerMono::Parameter());
  ~ObjectTrackerMono();

  void setCameraParameter(const cv::Mat &_intrinsic, const cv::Mat &_dist_coeffs);
  void setObjectCameraParameter(const cv::Mat &_intrinsic, const cv::Mat &_dist_coeffs);
  void setObjectModel(const Object::Ptr &_model);
  void reset();

  bool track(const cv::Mat &image, Eigen::Matrix4f &pose, double &out_conf);

  const Object::Ptr &getModelPtr() {
    return model;
  }
  inline const Object &getModel() {
    return *model;
  }

  typedef std::shared_ptr<::v4r::ObjectTrackerMono> Ptr;
  typedef std::shared_ptr<::v4r::ObjectTrackerMono const> ConstPtr;
};

/*************************** INLINE METHODES **************************/

}  // namespace v4r

#endif
