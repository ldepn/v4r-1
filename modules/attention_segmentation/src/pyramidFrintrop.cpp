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

#include "v4r/attention_segmentation/pyramidFrintrop.h"

namespace v4r {

FrintropPyramid::FrintropPyramid() : BasePyramid() {
  reset();
}

FrintropPyramid::~FrintropPyramid() {}

void FrintropPyramid::reset() {
  BasePyramid::reset();

  R.resize(2);
  R.at(0) = 3;
  R.at(1) = 7;
  onSwitch = true;

  pyramidName = "FrintropPyramid";
}

void FrintropPyramid::setR(std::vector<int> &R_) {
  R = R_;
  calculated = false;
  haveImagePyramid = false;
  printf("[INFO]: %s: R is set to: [ ", pyramidName.c_str());
  for (size_t i = 0; i < R.size(); ++i) {
    printf("%d ", R.at(i));
  }
  printf("]\n");
}

std::vector<int> FrintropPyramid::getR() {
  return (R);
}

void FrintropPyramid::setOnSwitch(bool onSwitch_) {
  onSwitch = onSwitch_;
  calculated = false;
  // haveImagePyramid = false;
  printf("[INFO]: %s: onSwitch is set to: %s\n", pyramidName.c_str(), onSwitch ? "yes" : "no");
}

bool FrintropPyramid::getOnSwitch() {
  return (onSwitch);
}

void FrintropPyramid::print() {
  BasePyramid::print();

  printf("[PyramidParameters]: onSwitch             = %s\n", onSwitch ? "yes" : "no");
  printf("[PyramidParameters]: R                    = [ ");
  for (unsigned int i = 0; i < R.size(); ++i) {
    printf("%d ", R.at(i));
  }
  printf("]\n");
}

void FrintropPyramid::combinePyramid(bool standard) {
  calculated = false;

  int number_of_features = (max_level - start_level + 1) * R.size();

  pyramidConspicuities.resize(number_of_features);

  std::vector<int> trueLevel;

  for (int s = start_level; s <= max_level; ++s) {
    for (unsigned int r = 0; r < R.size(); ++r) {
      cv::Mat kernel = cv::Mat_<float>::ones(R.at(r), R.at(r));
      kernel = kernel / (R.at(r) * R.at(r));

      int current = (R.size() - 0) * (s - start_level) + (r - 0);
      if (current < number_of_features) {
        cv::Mat temp;
        filter2D(pyramidFeatures.at(s), temp, pyramidFeatures.at(s).depth(), kernel);

        if (onSwitch) {
          temp = pyramidFeatures.at(s) - temp;
        } else {
          temp = temp - pyramidFeatures.at(s);
        }

        cv::max(temp, 0.0, pyramidConspicuities.at(current));
        v4r::normalize(pyramidConspicuities.at(current), normalization_type);
        trueLevel.push_back(s);
      }
    }
  }

  if (combination_type == AM_COMB_MUL) {
    map = cv::Mat_<float>::ones(pyramidImages.at(sm_level).rows, pyramidImages.at(sm_level).cols);
  } else {
    map = cv::Mat_<float>::zeros(pyramidImages.at(sm_level).rows, pyramidImages.at(sm_level).cols);
  }

  for (int i = 0; i < number_of_features; ++i) {
    cv::Mat temp;
    if (standard) {
      cv::resize(pyramidConspicuities.at(i), temp,
                 cv::Size(pyramidImages.at(sm_level).cols, pyramidImages.at(sm_level).rows));
    } else {
      v4r::scaleImage(pyramidImages, pyramidConspicuities.at(i), temp, trueLevel.at(i), sm_level);
    }
    //     cv::imshow("temp",temp);
    //     cv::waitKey(-1);
    combineConspicuityMaps(map, temp);
  }

  double maxValue, minValue;
  cv::minMaxLoc(map, &minValue, &maxValue);
  max_map_value = maxValue;

  v4r::normalize(map, normalization_type);

  //   cv::minMaxLoc(map,&minValue,&maxValue);
  //   std::cerr << maxValue << std::endl;
  //   cv::imshow("map5",map);
  //   cv::waitKey(-1);

  calculated = true;
}
}  // namespace v4r
