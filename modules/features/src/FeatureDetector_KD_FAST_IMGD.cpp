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

#include <v4r/features/FeatureDetector_KD_FAST_IMGD.h>

#if CV_MAJOR_VERSION < 3
#define HAVE_OCV_2
#else
#include <opencv2/core/ocl.hpp>
#endif

namespace v4r {

using namespace std;

FeatureDetector_KD_FAST_IMGD::FeatureDetector_KD_FAST_IMGD(const Parameter &_p)
: FeatureDetector(FeatureDetector::Type::KD_FAST_IMGD), param(_p) {
  descr_name_ = "fast_imgd";
// orb = new cv::ORB(10000, 1.2, 6, 13, 0, 2, cv::ORB::HARRIS_SCORE, 13); //31
// orb = new cv::ORB(1000, 1.44, 2, 17, 0, 2, cv::ORB::HARRIS_SCORE, 17);
#ifdef HAVE_OCV_2
  orb = new cv::ORB(param.nfeatures, param.scaleFactor, param.nlevels, param.patchSize, 0, 2, cv::ORB::HARRIS_SCORE,
                    param.patchSize);
#else
  orb = cv::ORB::create(param.nfeatures, param.scaleFactor, param.nlevels, 31, 0, 2, cv::ORB::HARRIS_SCORE,
                        param.patchSize);
#endif

  imGDesc.reset(new ComputeImGradientDescriptors(param.gdParam));

  fs.reset(new FeatureSelection(FeatureSelection::Parameter(2., 0.5)));
}

FeatureDetector_KD_FAST_IMGD::~FeatureDetector_KD_FAST_IMGD() {}

void FeatureDetector_KD_FAST_IMGD::detectAndCompute(const cv::Mat &image, std::vector<cv::KeyPoint> &keys,
                                                    cv::Mat &descriptors, const cv::Mat &object_mask) {
  if (image.type() != CV_8U)
    cv::cvtColor(image, im_gray_, cv::COLOR_RGB2GRAY);
  else
    im_gray_ = image;

  if (!object_mask.empty())
    std::cout << "Object mask for FAST currently not implemented!" << std::endl;

#ifndef HAVE_OCV_2
  cv::ocl::setUseOpenCL(false);
#endif

  orb->detect(im_gray_, keys);
  imGDesc->compute(im_gray_, keys, descriptors);
  computeKeypointIndices(im_gray_, keys);
}

void FeatureDetector_KD_FAST_IMGD::detect(const cv::Mat &image, std::vector<cv::KeyPoint> &keys,
                                          const cv::Mat &object_mask) {
  keys.clear();

  if (image.type() != CV_8U)
    cv::cvtColor(image, im_gray_, cv::COLOR_RGB2GRAY);
  else
    im_gray_ = image;

  if (!object_mask.empty())
    std::cout << "Object mask for Harris Detection currently not implemented!" << std::endl;

#ifndef HAVE_OCV_2
  cv::ocl::setUseOpenCL(false);
#endif

  if (param.tiles > 1) {
    cv::Rect rect;
    cv::Point2f pt_offs;
    std::vector<cv::KeyPoint> tmp_keys;

    tile_size_w = image.cols / param.tiles;
    tile_size_h = image.rows / param.tiles;

    for (int v = 0; v < param.tiles; v++) {
      for (int u = 0; u < param.tiles; u++) {
        getExpandedRect(u, v, image.rows, image.cols, rect);

        orb->detect(im_gray_(rect), tmp_keys);

        pt_offs = cv::Point2f(rect.x, rect.y);

        for (unsigned i = 0; i < tmp_keys.size(); i++)
          tmp_keys[i].pt += pt_offs;

        keys.insert(keys.end(), tmp_keys.begin(), tmp_keys.end());
        // cout<<"tile "<<v*param.tiles+u<<": "<<tmp_keys.size()<<" features"<<endl;  //DEBUG!!!!
      }
    }
  } else
    orb->detect(im_gray_, keys);

  computeKeypointIndices(im_gray_, keys);
}

/**
 * detect
 */
void FeatureDetector_KD_FAST_IMGD::compute(const cv::Mat &image, std::vector<cv::KeyPoint> &keys,
                                           cv::Mat &descriptors) {
  if (image.type() != CV_8U)
    cv::cvtColor(image, im_gray_, cv::COLOR_RGB2GRAY);
  else
    im_gray_ = image;

#ifndef HAVE_OCV_2
  cv::ocl::setUseOpenCL(false);
#endif

  imGDesc->compute(im_gray_, keys, descriptors);

  if (param.do_feature_selection) {
    fs->dbg = image;
    // cout<<"[FeatureDetector_KD_FAST_IMGD::extract] num detected: "<<keys.size()<<", "<<descriptors.rows<<endl;
    fs->compute(keys, descriptors);
    // cout<<"[FeatureDetector_KD_FAST_IMGD::extract] num selected: "<<keys.size()<<", "<<descriptors.rows<<endl;
  }
}
}  // namespace v4r
