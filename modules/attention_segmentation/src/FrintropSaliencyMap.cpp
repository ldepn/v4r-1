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

#include "v4r/attention_segmentation/FrintropSaliencyMap.h"

namespace v4r {

FrintropSaliencyMap::FrintropSaliencyMap() : BaseMap() {
  reset();
}

FrintropSaliencyMap::~FrintropSaliencyMap() {}

void FrintropSaliencyMap::reset() {
  BaseMap::reset();
  numberOfOrientations = 4;

  mapName = "FrintropSaliencyMap";
}

void FrintropSaliencyMap::print() {
  BaseMap::print();
  printf("[%s]: numberOfOrientations = %d\n", mapName.c_str(), numberOfOrientations);
}

void FrintropSaliencyMap::setNumberOfOrientations(int numberOfOrientations_) {
  numberOfOrientations = numberOfOrientations_;
  calculated = false;
  printf("[INFO]: %s: numberOfOrientations: %d.\n", mapName.c_str(), numberOfOrientations);
}

int FrintropSaliencyMap::getNumberOfOrientations() {
  return (numberOfOrientations);
}

int FrintropSaliencyMap::checkParameters() {
  if (!haveImage) {
    printf("[ERROR]: %s: No image set.\n", mapName.c_str());
    return (AM_IMAGE);
  }

  if ((width == 0) || (height == 0) || (image.rows == 0) || (image.cols == 0)) {
    printf("[ERROR]: %s: Seems like image is empty.\n", mapName.c_str());
    return (AM_IMAGE);
  }

  if ((image.cols != width) || (image.rows != height)) {
    printf("[ERROR]: %s: Problem with image sizes.\n", mapName.c_str());
    return (AM_IMAGE);
  }

  if (image.channels() != 3) {
    printf("[ERROR]: %s: Image should have 3 channels.\n", mapName.c_str());
    return (AM_IMAGE);
  }

  if (!haveMask)
    mask = cv::Mat_<uchar>::ones(height, width);

  if ((mask.cols != width) || (mask.rows != height)) {
    mask = cv::Mat_<uchar>::ones(height, width);
  }

  return (AM_OK);
}

void FrintropSaliencyMap::createColorChannels() {
  if (image.channels() > 1)
    cv::cvtColor(image, I, cv::COLOR_RGB2GRAY);
  else
    image.copyTo(I);

  I.convertTo(I, CV_32F, 1.0f / 255);

  if (image.channels() > 1) {
    ColorSaliencyMap colorSaliencyMap;
    colorSaliencyMap.setImage(image);
    colorSaliencyMap.setUseLAB(true);
    // red
    colorSaliencyMap.setColor(cv::Scalar(255, 127));
    colorSaliencyMap.calculate();
    if (!colorSaliencyMap.getMap(R)) {
      throw std::runtime_error("FrintropSaliencyMap:createColorChannels:R: computation failed.");
    }
    // green
    colorSaliencyMap.setColor(cv::Scalar(0, 127));
    colorSaliencyMap.calculate();
    if (!colorSaliencyMap.getMap(G)) {
      throw std::runtime_error("FrintropSaliencyMap:createColorChannels:G: computation failed.");
    }
    // blue
    colorSaliencyMap.setColor(cv::Scalar(127, 0));
    colorSaliencyMap.calculate();
    if (!colorSaliencyMap.getMap(B)) {
      throw std::runtime_error("FrintropSaliencyMap:createColorChannels:B: computation failed.");
    }
    // yellow
    colorSaliencyMap.setColor(cv::Scalar(127, 255));
    colorSaliencyMap.calculate();
    if (!colorSaliencyMap.getMap(Y)) {
      throw std::runtime_error("FrintropSaliencyMap:createColorChannels:Y: computation failed.");
    }
  }
}

void FrintropSaliencyMap::initializePyramid(FrintropPyramid::Ptr pyramid, cv::Mat &IM, bool onSwitch_) {
  pyramid->setStartLevel(2);                          //
  pyramid->setMaxLevel(4);                            //
  pyramid->setSMLevel(0);                             //
  pyramid->setWidth(width);                           //
  pyramid->setHeight(height);                         //
  pyramid->setNormalizationType(normalization_type);  // v4r::NT_FRINTROP_NORM

  std::vector<int> R;  //
  R.resize(2);
  R.at(0) = 3;
  R.at(1) = 7;                      //
  pyramid->setR(R);                 //
  pyramid->setOnSwitch(onSwitch_);  //

  pyramid->setImage(IM);
  pyramid->buildPyramid();
  pyramid->print();
}

void FrintropSaliencyMap::initializePyramid(SimplePyramid::Ptr pyramid, cv::Mat &IM) {
  pyramid->setStartLevel(2);                          //
  pyramid->setMaxLevel(4);                            //
  pyramid->setSMLevel(0);                             //
  pyramid->setWidth(width);                           //
  pyramid->setHeight(height);                         //
  pyramid->setNormalizationType(normalization_type);  // v4r::NT_FRINTROP_NORM;

  pyramid->setImage(IM);
  pyramid->buildPyramid();
  pyramid->print();
}

int FrintropSaliencyMap::calculate() {
  calculated = false;

  int rt_code = checkParameters();
  if (rt_code != AM_OK)
    return (rt_code);

  printf("[INFO]: %s: Computation started.\n", mapName.c_str());

  createColorChannels();

  FrintropPyramid::Ptr pyramidIOn(new FrintropPyramid());
  initializePyramid(pyramidIOn, I, true);

  FrintropPyramid::Ptr pyramidIOff(new FrintropPyramid());
  initializePyramid(pyramidIOff, I, false);

  std::vector<SimplePyramid::Ptr> pyramidO;
  pyramidO.resize(numberOfOrientations);

  for (int i = 0; i < numberOfOrientations; ++i) {
    pyramidO.at(i) = SimplePyramid::Ptr(new SimplePyramid());
    initializePyramid(pyramidO.at(i), I);
  }

  FrintropPyramid::Ptr pyramidR(new FrintropPyramid());
  FrintropPyramid::Ptr pyramidG(new FrintropPyramid());
  FrintropPyramid::Ptr pyramidB(new FrintropPyramid());
  FrintropPyramid::Ptr pyramidY(new FrintropPyramid());

  if (image.channels() > 1) {
    initializePyramid(pyramidR, R, true);
    initializePyramid(pyramidG, G, true);
    initializePyramid(pyramidB, B, true);
    initializePyramid(pyramidY, Y, true);
  }

  rt_code = createFeatureMapsI(pyramidIOn);
  if (rt_code != AM_OK)
    return (rt_code);
  rt_code = createFeatureMapsI(pyramidIOff);
  if (rt_code != AM_OK)
    return (rt_code);

  for (int i = 0; i < numberOfOrientations; ++i) {
    float angle = i * 180.0 / numberOfOrientations;
    rt_code = createFeatureMapsO(pyramidO.at(i), angle);
    if (rt_code != AM_OK)
      return (rt_code);
  }

  if (image.channels() > 1) {
    rt_code = createFeatureMapsI(pyramidR);
    if (rt_code != AM_OK)
      return (rt_code);
    rt_code = createFeatureMapsI(pyramidG);
    if (rt_code != AM_OK)
      return (rt_code);
    rt_code = createFeatureMapsI(pyramidB);
    if (rt_code != AM_OK)
      return (rt_code);
    rt_code = createFeatureMapsI(pyramidY);
    if (rt_code != AM_OK)
      return (rt_code);
  }

  cv::Mat pyramidIOn_map;
  if (!pyramidIOn->getMap(pyramidIOn_map)) {
    printf("[ERROR]: Something went wrong! Can't get saliency map from the pyramid!\n");
    return (AM_CUSTOM);
  }

  //   cv::imshow("pyramidIOn_map",pyramidIOn_map);
  //   cv::waitKey(-1);

  cv::Mat pyramidIOff_map;
  if (!pyramidIOff->getMap(pyramidIOff_map)) {
    printf("[ERROR]: Something went wrong! Can't get saliency map from the pyramid!\n");
    return (AM_CUSTOM);
  }

  //   cv::imshow("pyramidIOff_map",pyramidIOff_map);
  //   cv::waitKey(-1);

  float maxIntensityValue = std::max(pyramidIOn->getMaxMapValue(), pyramidIOff->getMaxMapValue());
  cv::Mat intensity = pyramidIOn_map + pyramidIOff_map;
  v4r::normalize(intensity, v4r::NT_NONE, maxIntensityValue);
  v4r::normalize(intensity, normalization_type);

  cv::Mat orientation;
  float maxOrientationValue = 0;
  for (int i = 0; i < numberOfOrientations; ++i) {
    maxOrientationValue = std::max(maxOrientationValue, pyramidO.at(i)->getMaxMapValue());

    cv::Mat orientation_temp;

    if (!pyramidO.at(i)->getMap(orientation_temp)) {
      printf("[ERROR]: Something went wrong! Can't get saliency map from the pyramid!\n");
      return (AM_CUSTOM);
    }

    if (i == 0)
      orientation_temp.copyTo(orientation);
    else
      orientation = orientation + orientation_temp;
  }
  v4r::normalize(orientation, v4r::NT_NONE, maxOrientationValue);
  v4r::normalize(orientation, normalization_type);

  //   cv::imshow("orientation",orientation);
  //   cv::waitKey(-1);

  map = intensity + orientation;

  if (image.channels() > 1) {
    float maxColorValue = std::max(pyramidR->getMaxMapValue(), pyramidG->getMaxMapValue());
    maxColorValue = std::max(maxColorValue, pyramidB->getMaxMapValue());
    maxColorValue = std::max(maxColorValue, pyramidY->getMaxMapValue());

    cv::Mat pyramidR_map;
    if (!pyramidR->getMap(pyramidR_map)) {
      printf("[ERROR]: Something went wrong! Can't get saliency map from the pyramid!\n");
      return (AM_CUSTOM);
    }

    //     cv::imshow("pyramidR_map",pyramidR_map);
    //     cv::waitKey(-1);

    cv::Mat pyramidG_map;
    if (!pyramidG->getMap(pyramidG_map)) {
      printf("[ERROR]: Something went wrong! Can't get saliency map from the pyramid!\n");
      return (AM_CUSTOM);
    }

    //     cv::imshow("pyramidG_map",pyramidG_map);
    //     cv::waitKey(-1);

    cv::Mat pyramidB_map;
    if (!pyramidB->getMap(pyramidB_map)) {
      printf("[ERROR]: Something went wrong! Can't get saliency map from the pyramid!\n");
      return (AM_CUSTOM);
    }

    //     cv::imshow("pyramidB_map",pyramidB_map);
    //     cv::waitKey(-1);

    cv::Mat pyramidY_map;
    if (!pyramidY->getMap(pyramidY_map)) {
      printf("[ERROR]: Something went wrong! Can't get saliency map from the pyramid!\n");
      return (AM_CUSTOM);
    }

    //     cv::imshow("pyramidY_map",pyramidY_map);
    //     cv::waitKey(-1);

    cv::Mat color = pyramidR_map + pyramidG_map + pyramidB_map + pyramidY_map;
    v4r::normalize(color, v4r::NT_NONE, maxColorValue);
    v4r::normalize(color, normalization_type);
    map = map + color;
  }

  //   cv::imshow("map",map);
  //   cv::waitKey(-1);

  v4r::normalize(map);

  calculated = true;

  printf("[INFO]: %s: Computation finished.\n", mapName.c_str());

  return (AM_OK);
}

int FrintropSaliencyMap::createFeatureMapsI(FrintropPyramid::Ptr pyramid) {
  for (unsigned int i = pyramid->getStartLevel(); i <= (unsigned int)pyramid->getMaxLevel(); ++i) {
    printf("[INFO]: %s: Computating feature map for level %d.\n", mapName.c_str(), i);

    cv::Mat current_image;
    if (!pyramid->getImage(i, current_image)) {
      printf("[ERROR]: Something went wrong! Can't get image for level %d!\n", i);
      return (AM_CUSTOM);
    }

    if (!pyramid->setFeatureMap(i, current_image)) {
      printf("[ERROR]: Something went wrong! Can't set feature map for level %d!\n", i);
      return (AM_CUSTOM);
    }

    printf("[INFO]: %s: Feature map at level %d is set.\n", mapName.c_str(), i);
  }
  // combine saliency maps
  pyramid->combinePyramid(true);

  return (AM_OK);
}

int FrintropSaliencyMap::createFeatureMapsO(SimplePyramid::Ptr pyramid, float angle) {
  for (unsigned int i = pyramid->getStartLevel(); i <= (unsigned int)pyramid->getMaxLevel(); ++i) {
    printf("[INFO]: %s: Computating feature map for level %d.\n", mapName.c_str(), i);

    cv::Mat current_image;
    if (!pyramid->getImage(i, current_image)) {
      printf("[ERROR]: Something went wrong! Can't get image for level %d!\n", i);
      return (AM_CUSTOM);
    }

    cv::Mat gaborKernel0, gaborKernel90;
    v4r::makeGaborFilter(gaborKernel0, gaborKernel90, angle);
    cv::Mat temp0, temp90;
    cv::filter2D(current_image, temp0, -1, gaborKernel0);
    temp0 = cv::abs(temp0);
    cv::filter2D(current_image, temp90, -1, gaborKernel90);
    temp90 = cv::abs(temp90);
    cv::Mat current_map;
    cv::add(temp0, temp90, current_map);

    if (!pyramid->setFeatureMap(i, current_map)) {
      printf("[ERROR]: Something went wrong! Can't set feature map for level %d!\n", i);
      return (AM_CUSTOM);
    }

    printf("[INFO]: %s: Feature map at level %d is set.\n", mapName.c_str(), i);
  }
  // combine saliency maps
  pyramid->combinePyramid(true);

  return (AM_OK);
}

}  // namespace v4r
