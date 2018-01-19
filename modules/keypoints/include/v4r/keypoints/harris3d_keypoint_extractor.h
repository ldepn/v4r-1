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
 * @file harris_keypoint_extractor.h
 * @author Thomas Faeulhammer (faeulhammer@acin.tuwien.ac.at)
 * @date 2016
 * @brief
 *
 */
#pragma once

#include <pcl/common/io.h>
#include <v4r/keypoints/keypoint_extractor.h>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

namespace v4r {

class V4R_EXPORTS Harris3DKeypointExtractorParameter {
 public:
  float threshold_;
  float search_radius_;  ///< radius the sphere radius used as the maximum distance to consider a point a neighbor
  bool refine_;

  Harris3DKeypointExtractorParameter() : threshold_(1e-4), search_radius_(0.02f), refine_(true) {}

  /**
   * @brief init parameters
   * @param command_line_arguments (according to Boost program options library)
   * @return unused parameters (given parameters that were not used in this initialization call)
   */
  std::vector<std::string> init(int argc, char **argv) {
    std::vector<std::string> arguments(argv + 1, argv + argc);
    return init(arguments);
  }

  /**
   * @brief init parameters
   * @param command_line_arguments (according to Boost program options library)
   * @return unused parameters (given parameters that were not used in this initialization call)
   */
  std::vector<std::string> init(const std::vector<std::string> &command_line_arguments) {
    po::options_description desc("Harris 3D Keypoint Extractor Parameter\n=====================\n");
    desc.add_options()("help,h", "produce help message");
    desc.add_options()("harris3d_kp_threshold", po::value<float>(&threshold_)->default_value(threshold_), "threshold");
    desc.add_options()("harris3d_search_radius", po::value<float>(&search_radius_)->default_value(search_radius_),
                       "radius the sphere radius used as the maximum distance to consider a point a neighbor");
    desc.add_options()("harris3d_refine", po::value<bool>(&refine_)->default_value(refine_), "refine");
    po::variables_map vm;
    po::parsed_options parsed =
        po::command_line_parser(command_line_arguments).options(desc).allow_unregistered().run();
    std::vector<std::string> to_pass_further = po::collect_unrecognized(parsed.options, po::include_positional);
    po::store(parsed, vm);
    if (vm.count("help")) {
      std::cout << desc << std::endl;
      to_pass_further.push_back("-h");
    }
    try {
      po::notify(vm);
    } catch (std::exception &e) {
      std::cerr << "Error: " << e.what() << std::endl << std::endl << desc << std::endl;
    }
    return to_pass_further;
  }
};

template <typename PointT>
class V4R_EXPORTS Harris3DKeypointExtractor : public KeypointExtractor<PointT> {
 private:
  typedef typename pcl::PointCloud<PointT>::Ptr PointInTPtr;
  using KeypointExtractor<PointT>::input_;
  using KeypointExtractor<PointT>::normals_;
  using KeypointExtractor<PointT>::indices_;
  using KeypointExtractor<PointT>::keypoints_;
  using KeypointExtractor<PointT>::keypoint_indices_;

  Harris3DKeypointExtractorParameter param_;

 public:
  Harris3DKeypointExtractor(const Harris3DKeypointExtractorParameter &p = Harris3DKeypointExtractorParameter())
  : param_(p) {}

  void compute();

  bool needNormals() const {
    return true;
  }

  int getKeypointExtractorType() const {
    return KeypointType::HARRIS3D;
  }

  std::string getKeypointExtractorName() const {
    return "harris3d";
  }

  typename pcl::PointCloud<PointT>::Ptr getKeypoints() {
    keypoints_.reset(new pcl::PointCloud<PointT>);
    pcl::copyPointCloud(*input_, keypoint_indices_, *keypoints_);
    return keypoints_;
  }

  typedef boost::shared_ptr<Harris3DKeypointExtractor<PointT>> Ptr;
  typedef boost::shared_ptr<Harris3DKeypointExtractor<PointT> const> ConstPtr;
};
}
