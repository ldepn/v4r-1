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

#ifndef TJ_HPP
#define TJ_HPP

#include <v4r/core/macros.h>
#include "v4r/attention_segmentation/headers.h"

namespace v4r {

/*extern int dy8[8];
extern int dx8[8];

extern int dx4[4];
extern int dy4[4];
*/

enum PointJunctionType {
  UNKNOWN = 0,
  T_JUNCTION = 3,
  END_POINT = 1,
  REGULAR_POINT = 2,
};

struct JunctionNode {
  int num;
  int x, y;
  std::vector<int> edges;
  int edges_num;
  int type;
  float saliency;
  JunctionNode() {
    edges_num = 0;
    type = UNKNOWN;
  }
};

class V4R_EXPORTS SaliencyLine {
 private:
  std::vector<JunctionNode> points;
  int points_num;
  float saliency;

 public:
  SaliencyLine();
  void clear();
  float getSaliency();
  std::vector<JunctionNode> getPoints();
  void addPoint(JunctionNode node);
  int getPointsNumber();
};

struct PointSaliency {
  cv::Point point;
  float saliency;
};

bool calculateSaliencyLine(cv::Mat mask, const cv::Mat symmetry, SaliencyLine &saliencyLine, unsigned int th = 10);
bool findTJunctions(SaliencyLine saliencyLine, std::vector<int> &tjunctionPoints);
std::vector<int> findEndPoints(SaliencyLine saliencyLine, std::vector<int> segment);
std::vector<int> findEndPoints(SaliencyLine saliencyLine);
std::vector<int> getEdges(std::vector<JunctionNode> nodes, int nodeIdx);
void breakIntoSegments(SaliencyLine saliencyLine, std::vector<std::vector<int>> &segments);
void modifySymmetryLine(SaliencyLine saliencyLine, std::vector<bool> &usedPoints, float th = 0.5);
V4R_EXPORTS void selectSaliencyCenterPoint(SaliencyLine saliencyLine, PointSaliency &center);
void createSimpleLine(SaliencyLine saliencyLine, std::vector<cv::Point> &points);
V4R_EXPORTS bool extractSaliencyLine(cv::Mat mask, cv::Mat map, SaliencyLine &saliencyLine, unsigned int th = 10);
V4R_EXPORTS void createAttentionPoints(std::vector<PointSaliency> saliencyPoints,
                                       std::vector<cv::Point> &attentionPoints);

inline bool saliencyPointsSort(PointSaliency p1, PointSaliency p2) {
  return (p1.saliency > p2.saliency);
}

}  // namespace v4r

#endif  // TJ_HPP
