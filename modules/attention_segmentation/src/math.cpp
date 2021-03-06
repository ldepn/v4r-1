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

#include <pcl/filters/project_inliers.h>
#include <v4r/core/macros.h>
#include "v4r/attention_segmentation/conversions.h"

#include "v4r/attention_segmentation/math.h"

namespace v4r {
float dotProduct(const Eigen::Vector3f &v1, const Eigen::Vector3f &v2) {
  return (v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2]);
}

float dotProduct(const cv::Point3f &v1, const cv::Point3f &v2) {
  Eigen::Vector3f v1_, v2_;
  v1_[0] = v1.x;
  v1_[1] = v1.y;
  v1_[2] = v1.z;
  v2_[0] = v2.x;
  v2_[1] = v2.y;
  v2_[2] = v2.z;
  return (dotProduct(v1_, v2_));
}

float vectorLength(const Eigen::Vector3f &v) {
  return (sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]));
}

float vectorLength(const cv::Point3d &v) {
  Eigen::Vector3f v_;
  v_[0] = v.x;
  v_[1] = v.y;
  v_[2] = v.z;
  return (vectorLength(v_));
}

float calculateCosine(const Eigen::Vector3f &v1, const Eigen::Vector3f &v2) {
  float v1len = vectorLength(v1);
  float v2len = vectorLength(v2);

  float cosine = 0;
  if ((v1len > 0) && (v2len > 0)) {
    cosine = dotProduct(v1, v2) / (v1len * v2len);
  }

  return (cosine);
}

float calculateCosine(const cv::Point3d &v1, const cv::Point3d &v2) {
  Eigen::Vector3f v1_, v2_;
  v1_[0] = v1.x;
  v1_[1] = v1.y;
  v1_[2] = v1.z;
  v2_[0] = v2.x;
  v2_[1] = v2.y;
  v2_[2] = v2.z;
  return (calculateCosine(v1_, v2_));
}

Eigen::Vector3f normalize(Eigen::Vector3f v) {
  float vlen = vectorLength(v);
  if (vlen > 0) {
    v[0] /= vlen;
    v[1] /= vlen;
    v[2] /= vlen;
  } else {
    v[0] = 0;
    v[1] = 0;
    v[2] = 0;
  }
  return (v);
}

cv::Point3d normalize(cv::Point3d v) {
  Eigen::Vector3f v_;
  v_[0] = v.x;
  v_[1] = v.y;
  v_[2] = v.z;
  v_ = normalize(v_);
  v.x = v_[0];
  v.y = v_[1];
  v.z = v_[2];
  return (v);
}

Eigen::Vector3f crossProduct(const Eigen::Vector3f &v1, const Eigen::Vector3f &v2) {
  Eigen::Vector3f v;
  v[0] = v1[1] * v2[2] - v1[2] * v2[1];
  v[1] = v1[2] * v2[0] - v1[0] * v2[2];
  v[2] = v1[0] * v2[1] - v1[1] * v2[0];

  return (v);
}

cv::Point3d crossProduct(const cv::Point3d &v1, const cv::Point3d &v2) {
  Eigen::Vector3f v1_, v2_;
  v1_[0] = v1.x;
  v1_[1] = v1.y;
  v1_[2] = v1.z;
  v2_[0] = v2.x;
  v2_[1] = v2.y;
  v2_[2] = v2.z;
  Eigen::Vector3f v_ = crossProduct(v1_, v2_);
  cv::Point3d v;
  v.x = v_[0];
  v.y = v_[1];
  v.z = v_[2];
  return (v);
}

Eigen::Vector3f crossProduct(const Eigen::Vector3f &p1, const Eigen::Vector3f &p2, const Eigen::Vector3f &p3) {
  Eigen::Vector3f v1, v2;
  v1[0] = p1[0] - p2[0];
  v1[1] = p1[1] - p2[1];
  v1[2] = p1[2] - p2[2];
  v2[0] = p1[0] - p3[0];
  v2[1] = p1[1] - p3[1];
  v2[2] = p1[2] - p3[2];
  return (crossProduct(v1, v2));
}

cv::Point3d crossProduct(const cv::Point3d &p1, const cv::Point3d &p2, const cv::Point3d &p3) {
  Eigen::Vector3f v1, v2;
  v1[0] = p1.x - p2.x;
  v1[1] = p1.y - p2.y;
  v1[2] = p1.z - p2.z;
  v2[0] = p1.x - p3.x;
  v2[1] = p1.y - p3.y;
  v2[2] = p1.z - p3.z;
  Eigen::Vector3f v_ = crossProduct(v1, v2);
  cv::Point3d v;
  v.x = v_[0];
  v.y = v_[1];
  v.z = v_[2];
  return (v);
}

Eigen::Vector3f calculatePlaneNormal(const Eigen::Vector3f &v1, const Eigen::Vector3f &v2) {
  Eigen::Vector3f v = crossProduct(v1, v2);
  v = normalize(v);
  return (v);
}

Eigen::Vector3f calculatePlaneNormal(const Eigen::Vector3f &p1, const Eigen::Vector3f &p2, const Eigen::Vector3f &p3) {
  Eigen::Vector3f v = crossProduct(p1, p2, p3);
  v = normalize(v);
  return (v);
}

cv::Point3d calculatePlaneNormal(const cv::Point3d &v1, const cv::Point3d &v2) {
  cv::Point3d v = crossProduct(v1, v2);
  v = normalize(v);
  return (v);
}

cv::Point3d calculatePlaneNormal(const cv::Point3d &p1, const cv::Point3d &p2, const cv::Point3d &p3) {
  cv::Point3d v = crossProduct(p1, p2, p3);
  v = normalize(v);
  return (v);
}

#ifndef NOT_USE_PCL

float dotProduct(const pcl::Normal &v1, const pcl::Normal &v2) {
  Eigen::Vector3f v1_, v2_;
  v1_[0] = v1.normal[0];
  v1_[1] = v1.normal[1];
  v1_[2] = v1.normal[2];
  v2_[0] = v2.normal[0];
  v2_[1] = v2.normal[1];
  v2_[2] = v2.normal[2];
  return (dotProduct(v1_, v2_));
}

float dotProduct(const pcl::PointXYZ &v1, const pcl::PointXYZ &v2) {
  Eigen::Vector3f v1_, v2_;
  v1_[0] = v1.x;
  v1_[1] = v1.y;
  v1_[2] = v1.z;
  v2_[0] = v2.x;
  v2_[1] = v2.y;
  v2_[2] = v2.z;
  return (dotProduct(v1_, v2_));
}

float vectorLength(const pcl::Normal &v) {
  Eigen::Vector3f v_;
  v_[0] = v.normal[0];
  v_[1] = v.normal[1];
  v_[2] = v.normal[2];
  return (vectorLength(v_));
}

float vectorLength(const pcl::PointXYZ &v) {
  Eigen::Vector3f v_;
  v_[0] = v.x;
  v_[1] = v.y;
  v_[2] = v.z;
  return (vectorLength(v_));
}

V4R_EXPORTS float calculateCosine(const pcl::Normal &v1, const pcl::Normal &v2) {
  Eigen::Vector3f v1_, v2_;
  v1_[0] = v1.normal[0];
  v1_[1] = v1.normal[1];
  v1_[2] = v1.normal[2];
  v2_[0] = v2.normal[0];
  v2_[1] = v2.normal[1];
  v2_[2] = v2.normal[2];
  return (calculateCosine(v1_, v2_));
}

float calculateCosine(const pcl::PointXYZ &v1, const pcl::PointXYZ &v2) {
  Eigen::Vector3f v1_, v2_;
  v1_[0] = v1.x;
  v1_[1] = v1.y;
  v1_[2] = v1.z;
  v2_[0] = v2.x;
  v2_[1] = v2.y;
  v2_[2] = v2.z;
  return (calculateCosine(v1_, v2_));
}

V4R_EXPORTS pcl::Normal normalize(pcl::Normal v) {
  Eigen::Vector3f v_;
  v_[0] = v.normal[0];
  v_[1] = v.normal[1];
  v_[2] = v.normal[2];
  v_ = normalize(v_);
  v.normal[0] = v_[0];
  v.normal[1] = v_[1];
  v.normal[2] = v_[2];
  return (v);
}

pcl::PointXYZ normalize(pcl::PointXYZ v) {
  Eigen::Vector3f v_;
  v_[0] = v.x;
  v_[1] = v.y;
  v_[2] = v.z;
  v_ = normalize(v_);
  v.x = v_[0];
  v.y = v_[1];
  v.z = v_[2];
  return (v);
}

pcl::Normal crossProduct(const pcl::Normal &v1, const pcl::Normal &v2) {
  Eigen::Vector3f v1_, v2_;
  v1_[0] = v1.normal[0];
  v1_[1] = v1.normal[1];
  v1_[2] = v1.normal[2];
  v2_[0] = v2.normal[0];
  v2_[1] = v2.normal[1];
  v2_[2] = v2.normal[2];
  Eigen::Vector3f v_ = crossProduct(v1_, v2_);
  pcl::Normal v;
  v.normal[0] = v_[0];
  v.normal[1] = v_[1];
  v.normal[2] = v_[2];
  return (v);
}

pcl::PointXYZ crossProduct(const pcl::PointXYZ &v1, const pcl::PointXYZ &v2) {
  Eigen::Vector3f v1_, v2_;
  v1_[0] = v1.x;
  v1_[1] = v1.y;
  v1_[2] = v1.z;
  v2_[0] = v2.x;
  v2_[1] = v2.y;
  v2_[2] = v2.z;
  Eigen::Vector3f v_ = crossProduct(v1_, v2_);
  pcl::PointXYZ v;
  v.x = v_[0];
  v.y = v_[1];
  v.z = v_[2];
  return (v);
}

pcl::Normal crossProduct(const pcl::Normal &p1, const pcl::Normal &p2, const pcl::Normal &p3) {
  Eigen::Vector3f v1, v2;
  v1[0] = p1.normal[0] - p2.normal[0];
  v1[1] = p1.normal[1] - p2.normal[1];
  v1[2] = p1.normal[2] - p2.normal[2];
  v2[0] = p1.normal[0] - p3.normal[0];
  v2[1] = p1.normal[1] - p3.normal[1];
  v2[2] = p1.normal[2] - p3.normal[2];
  Eigen::Vector3f v_ = crossProduct(v1, v2);
  pcl::Normal v;
  v.normal[0] = v_[0];
  v.normal[1] = v_[1];
  v.normal[2] = v_[2];
  return (v);
}

pcl::PointXYZ crossProduct(const pcl::PointXYZ &p1, const pcl::PointXYZ &p2, const pcl::PointXYZ &p3) {
  Eigen::Vector3f v1, v2;
  v1[0] = p1.x - p2.x;
  v1[1] = p1.y - p2.y;
  v1[2] = p1.z - p2.z;
  v2[0] = p1.x - p3.x;
  v2[1] = p1.y - p3.y;
  v2[2] = p1.z - p3.z;
  Eigen::Vector3f v_ = crossProduct(v1, v2);
  pcl::PointXYZ v;
  v.x = v_[0];
  v.y = v_[1];
  v.z = v_[2];
  return (v);
}

V4R_EXPORTS pcl::Normal calculatePlaneNormal(const pcl::Normal &v1, const pcl::Normal &v2) {
  pcl::Normal v = crossProduct(v1, v2);
  v = normalize(v);
  return (v);
}

pcl::Normal calculatePlaneNormal(const pcl::Normal &p1, const pcl::Normal &p2, const pcl::Normal &p3) {
  pcl::Normal v = crossProduct(p1, p2, p3);
  v = normalize(v);
  return (v);
}

pcl::PointXYZ calculatePlaneNormal(const pcl::PointXYZ &v1, const pcl::PointXYZ &v2) {
  pcl::PointXYZ v = crossProduct(v1, v2);
  v = normalize(v);
  return (v);
}

pcl::PointXYZ calculatePlaneNormal(const pcl::PointXYZ &p1, const pcl::PointXYZ &p2, const pcl::PointXYZ &p3) {
  pcl::PointXYZ v = crossProduct(p1, p2, p3);
  v = normalize(v);
  return (v);
}

#endif

V4R_EXPORTS void ProjectPointsOnThePlane(pcl::ModelCoefficients::ConstPtr coefficients,
                                         pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr cloud,
                                         pcl::PointCloud<pcl::PointXYZRGB>::Ptr points_projected,
                                         std::vector<float> &distances, pcl::PointIndices::Ptr indices,
                                         bool normalize) {
  if ((indices->indices.size()) == 0) {
    indices->indices.resize(cloud->size());
    for (unsigned int i = 0; i < cloud->size(); ++i) {
      indices->indices.at(i) = i;
    }
  }

  // check types
  assert(indices->indices.size() <= cloud->size());

  // Retrieve Ground Plane Coefficients
  float a = coefficients->values.at(0);
  float b = coefficients->values.at(1);
  float c = coefficients->values.at(2);
  float d = coefficients->values.at(3);

  pcl::ProjectInliers<pcl::PointXYZRGB> proj;
  proj.setModelType(pcl::SACMODEL_PLANE);
  proj.setInputCloud(cloud);
  proj.setIndices(indices);
  proj.setModelCoefficients(coefficients);
  proj.filter(*points_projected);

  distances.resize(indices->indices.size());

  float max_distance = 0;

  for (unsigned int pi = 0; pi < indices->indices.size(); ++pi) {
    int idx = indices->indices.at(pi);
    distances.at(pi) = pcl::pointToPlaneDistance(cloud->points.at(idx), a, b, c, d);
    if (distances.at(pi) > max_distance) {
      max_distance = distances.at(pi);
    }
  }

  if ((normalize) && (max_distance > 0)) {
    for (unsigned int pi = 0; pi < distances.size(); ++pi) {
      distances.at(pi) /= max_distance;
    }
  }
}

}  // namespace v4r
