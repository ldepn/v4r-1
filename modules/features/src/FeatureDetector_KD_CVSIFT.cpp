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

#include <v4r/features/FeatureDetector_KD_CVSIFT.h>

namespace v4r {

using namespace std;

FeatureDetector_KD_CVSIFT::FeatureDetector_KD_CVSIFT(const Parameter &_p)
: FeatureDetector(FeatureDetector::Type::KD_CVSIFT), param(_p) {
  descr_name_ = "sift_opencv";
#if CV_MAJOR_VERSION < 3
  sift = new cv::SIFT(_p.nfeatures, _p.nOctaveLayers, _p.contrastThreshold, _p.edgeThreshold, _p.sigma);
#else
  sift = cv::xfeatures2d::SIFT::create();
#endif
}

void FeatureDetector_KD_CVSIFT::detectAndCompute(const cv::Mat &image, std::vector<cv::KeyPoint> &keys,
                                                 cv::Mat &descriptors, const cv::Mat &object_mask) {
  if (image.type() != CV_8U)
    cv::cvtColor(image, im_gray_, cv::COLOR_RGB2GRAY);
  else
    im_gray_ = image;

  sift->detectAndCompute(im_gray_, object_mask, keys, descriptors);
  computeKeypointIndices(im_gray_, keys);
}

void FeatureDetector_KD_CVSIFT::detect(const cv::Mat &image, std::vector<cv::KeyPoint> &keys,
                                       const cv::Mat &object_mask) {
  if (image.type() != CV_8U)
    cv::cvtColor(image, im_gray_, cv::COLOR_RGB2GRAY);
  else
    im_gray_ = image;

  sift->detect(im_gray_, keys, object_mask);
  computeKeypointIndices(im_gray_, keys);
}

void FeatureDetector_KD_CVSIFT::compute(const cv::Mat &image, std::vector<cv::KeyPoint> &keys, cv::Mat &descriptors) {
  if (image.type() != CV_8U)
    cv::cvtColor(image, im_gray_, cv::COLOR_RGB2GRAY);
  else
    im_gray_ = image;

  sift->compute(im_gray_, keys, descriptors);
}
}  // namespace v4r
