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
 * @file Graph.cpp
 * @author Andreas Richtsfeld
 * @date July 2011
 * @version 0.1
 * @brief Create graph from relations.
 */

#include "v4r/attention_segmentation/Graph.h"
#include <cmath>

#ifndef GC_DEBUG
#define GC_DEBUG true
#endif

namespace gc {

template <typename T1, typename T2>
inline T1 Dot3(const T1 v1[3], const T2 v2[3]) {
  return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

template <typename T1, typename T2, typename T3>
inline void Add3(const T1 v1[3], const T2 v2[3], T3 r[3]) {
  r[0] = v1[0] + v2[0];
  r[1] = v1[1] + v2[1];
  r[2] = v1[2] + v2[2];
}

/**
 * @brief Constructor of Graph
 */
Graph::Graph() {
  nodes = 0;
  relations.clear();
}

/**
 * @brief Constructor of Graph
 */
Graph::Graph(unsigned nrNodes, std::vector<v4r::Relation> &rel) {
  nodes = nrNodes;
  relations = rel;
}

/**
 * @brief Constructor of Graph
 */
Graph::Graph(std::vector<v4r::SurfaceModel::Ptr> &surf, std::vector<v4r::Relation> &rel) {
  surfaces = surf;
  relations = rel;
}

/**
 * @brief Destructor of GraphCut
 */
Graph::~Graph() {}

/**
 * @brief Build graph from results of the SVM-Predictor.
 * @param e Vector of edges
 * @param num_edges Number of created edges.
 */
void Graph::BuildFromSVM(std::vector<gc::Edge> &e, unsigned &num_edges, std::vector<int> &surfaces_reindex2) {
  if (GC_DEBUG) {
    for (unsigned i = 0; i < relations.size(); i++)
      printf("[Graph::BuildFromSVM] Relation %u: %u-%u\n", i, relations[i].id_0, relations[i].id_1);
  }

  // Connectivity check for graph
  //@ep: create connected graph?
  bool find_sink = false;
  int sink = 0;
  for (unsigned i = 0; i < surfaces.size(); i++) {
    if (!(surfaces.at(i)->selected))
      continue;

    if (!find_sink) {
      sink = i;
      find_sink = true;
    }

    bool node_found = false;
    for (unsigned j = 0; j < relations.size(); j++) {
      if (relations[j].id_0 == sink && relations[j].id_1 == ((int)i)) {
        node_found = true;
      }
    }

    if (!node_found) {
      if (GC_DEBUG)
        printf("[Graph::BuildFromSVM] Warning: Node without relation: Add relation: %u-%u.\n", 0, i);

      v4r::Relation r;
      r.id_0 = sink;
      r.id_1 = i;
      r.groundTruth = -1;
      r.type = 1;
      r.rel_probability.push_back(1.0);
      r.rel_probability.push_back(0.0);
      relations.push_back(r);
    }
  }

  if (GC_DEBUG) {
    for (unsigned i = 0; i < relations.size(); i++)
      printf("[Graph::BuildFromSVM] Relation %u: %u-%u\n", i, relations[i].id_0, relations[i].id_1);
  }

  // create maping from real surface to artificial surfaces
  surfaces_reindex.clear();
  for (unsigned i = 0; i < surfaces.size(); i++) {
    if (!(surfaces.at(i)->selected))
      continue;

    surfaces_reindex.push_back(i);
  }

  surfaces_reindex2.resize(surfaces.size());
  int num_valid_surfaces = 0;
  for (unsigned i = 0; i < surfaces.size(); i++) {
    surfaces_reindex2.at(i) = -1;

    if (!(surfaces.at(i)->selected))
      continue;

    surfaces_reindex2.at(i) = num_valid_surfaces;
    num_valid_surfaces++;
  }

  for (unsigned i = 0; i < relations.size(); i++) {
    gc::Edge e;
    e.a = surfaces_reindex2.at(relations[i].id_0);
    e.b = surfaces_reindex2.at(relations[i].id_1);
    e.type = 1;
    e.w = relations[i].rel_probability[0];
    if (GC_DEBUG)
      printf("[Graph::BuildFromSVM] New edge (type: %i): %i-%i: %8.8f\n", e.type, e.a, e.b, e.w);
    edges.push_back(e);
  }

  num_edges = edges.size();
  e = edges;

  if (GC_DEBUG)
    printf("[Graph::BuildFromSVM] Created %lu edges from %lu relations\n", edges.size(), relations.size());
}

/**
 * @brief Build graph from point cloud with 4-neighborhood.
 * @param pcl_cloud
 * @param e Vector of edges
 * @param num_edges Number of created edges.
 */
void Graph::BuildFromPointCloud(pcl::PointCloud<pcl::PointXYZRGB>::Ptr &_pcl_cloud,
                                pcl::PointCloud<pcl::Normal>::Ptr &_normals, std::vector<gc::Edge> &e,
                                unsigned &num_edges) {
  float z_adapt = 0.01;
  pcl_cloud = _pcl_cloud;
  normals = _normals;
  edges.clear();

  // normalization of curvature
  float max_curv = 0.0;
  for (unsigned i = 0; i < normals->points.size(); i++)
    if (!std::isnan(pcl_cloud->points[i].z))
      if (normals->points[i].curvature > max_curv)
        max_curv = normals->points[i].curvature;
  VLOG(1) << "Max curvature: << " << max_curv;

  // pre-calculate color distance
  float color_dist[pcl_cloud->points.size()][4];
  float max_color = 0.0f;
  // #pragma omp parallel for
  for (unsigned row = 0; row < pcl_cloud->height - 1; row++) {
    for (unsigned col = 0; col < pcl_cloud->width - 1; col++) {
      int idx = GetIdx(col, row);
      pcl::PointXYZRGB pt_0 = pcl_cloud->points[idx];  // Check also the distance of the points!
      pcl::PointXYZRGB pt_1 = pcl_cloud->points[idx + 1];
      color_dist[idx][0] =
          sqrt(((float)pt_0.r / 255. - (float)pt_1.r / 255.) * ((float)pt_0.r / 255. - (float)pt_1.r / 255.) +
               ((float)pt_0.g / 255. - (float)pt_1.g / 255.) * ((float)pt_0.g / 255. - (float)pt_1.g / 255.) +
               ((float)pt_0.b / 255. - (float)pt_1.b / 255.) * ((float)pt_0.b / 255. - (float)pt_1.b / 255.));
      if (color_dist[idx][0] > max_color)
        max_color = color_dist[idx][0];

      pt_0 = pcl_cloud->points[idx];
      pt_1 = pcl_cloud->points[idx + pcl_cloud->width];
      color_dist[idx][1] =
          sqrt(((float)pt_0.r / 255. - (float)pt_1.r / 255.) * ((float)pt_0.r / 255. - (float)pt_1.r / 255.) +
               ((float)pt_0.g / 255. - (float)pt_1.g / 255.) * ((float)pt_0.g / 255. - (float)pt_1.g / 255.) +
               ((float)pt_0.b / 255. - (float)pt_1.b / 255.) * ((float)pt_0.b / 255. - (float)pt_1.b / 255.));
      if (color_dist[idx][1] > max_color)
        max_color = color_dist[idx][1];

      pt_0 = pcl_cloud->points[idx];
      pt_1 = pcl_cloud->points[idx + pcl_cloud->width + 1];
      color_dist[idx][2] =
          sqrt(((float)pt_0.r / 255. - (float)pt_1.r / 255.) * ((float)pt_0.r / 255. - (float)pt_1.r / 255.) +
               ((float)pt_0.g / 255. - (float)pt_1.g / 255.) * ((float)pt_0.g / 255. - (float)pt_1.g / 255.) +
               ((float)pt_0.b / 255. - (float)pt_1.b / 255.) * ((float)pt_0.b / 255. - (float)pt_1.b / 255.));
      if (color_dist[idx][2] > max_color)
        max_color = color_dist[idx][2];

      if (col != 0) {
        pt_0 = pcl_cloud->points[idx];
        pt_1 = pcl_cloud->points[idx + pcl_cloud->width - 1];
        color_dist[idx][3] =
            sqrt(((float)pt_0.r / 255. - (float)pt_1.r / 255.) * ((float)pt_0.r / 255. - (float)pt_1.r / 255.) +
                 ((float)pt_0.g / 255. - (float)pt_1.g / 255.) * ((float)pt_0.g / 255. - (float)pt_1.g / 255.) +
                 ((float)pt_0.b / 255. - (float)pt_1.b / 255.) * ((float)pt_0.b / 255. - (float)pt_1.b / 255.));
        if (color_dist[idx][3] > max_color)
          max_color = color_dist[idx][3];
      } else
        color_dist[idx][3] = 1.0;
    }
  }

  //   /// YUV conversion
  //   for(unsigned i=0; i < pcl_cloud->points.size(); i++) {
  //     pcl::PointXYZRGB pt_o = pcl_cloud->points[i];
  //     pcl::PointXYZRGB pt_n;
  //     pt_n.r =  (0.257 * pt_o.r) + (0.504 * pt_o.g) + (0.098 * pt_o.b) + 16;     // R = Y
  //     pt_n.g = -(0.148 * pt_o.r) - (0.291 * pt_o.g) + (0.439 * pt_o.b) + 128;    // G = U
  //     pt_n.b =  (0.439 * pt_o.r) - (0.368 * pt_o.g) - (0.071 * pt_o.b) + 128;    // B = V
  //   }
  //
  //     // pre-calculate color distance
  //   float color_dist[pcl_cloud->points.size()][4];
  //   float max_color = 0.0f;
  // // #pragma omp parallel for
  //   for(unsigned row=0; row < pcl_cloud->height-1; row++) {
  //     for(unsigned col=0; col < pcl_cloud->width-1; col++) {
  //       int idx = GetIdx(col, row);
  //       pcl::PointXYZRGB pt_0 = pcl_cloud->points[idx];                                           // Check also the
  //       distance of the points!
  //       pcl::PointXYZRGB pt_1 = pcl_cloud->points[idx+1];
  //       color_dist[idx][0] = sqrt(((float)pt_0.g/255.-(float)pt_1.g/255.)*((float)pt_0.g/255.-(float)pt_1.g/255.) +
  //                                 ((float)pt_0.b/255.-(float)pt_1.b/255.)*((float)pt_0.b/255.-(float)pt_1.b/255.));
  //       if(color_dist[idx][0] > max_color)
  //         max_color = color_dist[idx][0];
  // // printf("color dist[0]: %4.3f\n", color_dist[idx][0]);
  //       pt_0 = pcl_cloud->points[idx];
  //       pt_1 = pcl_cloud->points[idx+pcl_cloud->width];
  //       color_dist[idx][1] = sqrt(((float)pt_0.g/255.-(float)pt_1.g/255.)*((float)pt_0.g/255.-(float)pt_1.g/255.) +
  //                                 ((float)pt_0.b/255.-(float)pt_1.b/255.)*((float)pt_0.b/255.-(float)pt_1.b/255.));
  //       if(color_dist[idx][1] > max_color)
  //         max_color = color_dist[idx][1];
  // // printf("color dist[1]: %4.3f\n", color_dist[idx][1]);
  //       pt_0 = pcl_cloud->points[idx];
  //       pt_1 = pcl_cloud->points[idx+pcl_cloud->width+1];
  //       color_dist[idx][2] = sqrt(((float)pt_0.g/255.-(float)pt_1.g/255.)*((float)pt_0.g/255.-(float)pt_1.g/255.) +
  //                                 ((float)pt_0.b/255.-(float)pt_1.b/255.)*((float)pt_0.b/255.-(float)pt_1.b/255.));
  //       if(color_dist[idx][2] > max_color)
  //         max_color = color_dist[idx][2];
  // // printf("color dist[2]: %4.3f\n", color_dist[idx][2]);
  //       if(col != 0) {
  //         pt_0 = pcl_cloud->points[idx];
  //         pt_1 = pcl_cloud->points[idx+pcl_cloud->width-1];
  //         color_dist[idx][3] = sqrt(((float)pt_0.g/255.-(float)pt_1.g/255.)*((float)pt_0.g/255.-(float)pt_1.g/255.) +
  //                                   ((float)pt_0.b/255.-(float)pt_1.b/255.)*((float)pt_0.b/255.-(float)pt_1.b/255.));
  //       if(color_dist[idx][3] > max_color)
  //         max_color = color_dist[idx][3];
  //       }
  //       else
  //         color_dist[idx][3] = 1.0;
  // // printf("color dist[3]: %4.3f\n", color_dist[idx][3]);
  //     }
  //   }

  // Create Graph
  for (unsigned row = 0; row < pcl_cloud->height - 1; row++) {  /// TODO Könnte man auch parallelisieren
    for (unsigned col = 0; col < pcl_cloud->width - 1; col++) {
      int idx = GetIdx(col, row);

      /// RIGHT
      gc::Edge e;
      e.a = idx;
      e.b = idx + 1;
      e.type = 1;
      e.w = color_dist[idx][0] / max_color;

      // normals angle
      double angle = 0.0;
      if (std::isnan(pcl_cloud->points[idx].z))
        angle = 1.57;
      else
        angle = acos(Dot3(&normals->points[idx].normal[0], &normals->points[idx + 1].normal[0]));
      if (!std::isnan(angle))
        e.w2 = angle;
      else
        e.w2 = 1.57;

      float z_dist = fabs(pcl_cloud->points[idx].z - pcl_cloud->points[idx + 1].z);
      if (!std::isnan(pcl_cloud->points[idx].z) && !std::isnan(pcl_cloud->points[idx + 1].z) &&
          z_dist < (z_adapt * pcl_cloud->points[idx].z))
        edges.push_back(e);

      /// BOTTOM
      e.a = idx;
      e.b = idx + pcl_cloud->width;
      e.type = 1;
      e.w = color_dist[idx][1] / max_color;

      // normals angle
      angle = 0.0;
      if (std::isnan(pcl_cloud->points[idx].z))
        angle = 1.57;
      else
        angle = acos(Dot3(&normals->points[idx].normal[0], &normals->points[idx + pcl_cloud->width].normal[0]));
      if (!std::isnan(angle))
        e.w2 = angle;
      else
        e.w2 = 1.57;

      z_dist = fabs(pcl_cloud->points[idx].z - pcl_cloud->points[idx + pcl_cloud->width].z);
      if (!std::isnan(pcl_cloud->points[idx].z) && !std::isnan(pcl_cloud->points[idx + pcl_cloud->width].z) &&
          z_dist < (z_adapt * pcl_cloud->points[idx].z))
        edges.push_back(e);

      /// BOTTOM-RIGHT
      e.a = idx;
      e.b = idx + pcl_cloud->width + 1;
      e.type = 1;
      e.w = color_dist[idx][2] / max_color;

      // normals angle
      angle = 0.0;
      if (std::isnan(pcl_cloud->points[idx].z))
        angle = 1.57;
      else
        angle = acos(Dot3(&normals->points[idx].normal[0], &normals->points[idx + pcl_cloud->width + 1].normal[0]));
      if (!std::isnan(angle))
        e.w2 = angle;
      else
        e.w2 = 1.57;

      z_dist = fabs(pcl_cloud->points[idx].z - pcl_cloud->points[idx + pcl_cloud->width + 1].z);
      if (!std::isnan(pcl_cloud->points[idx].z) && !std::isnan(pcl_cloud->points[idx + pcl_cloud->width + 1].z) &&
          z_dist < (z_adapt * pcl_cloud->points[idx].z))
        edges.push_back(e);

      /// BOTTOM-LEFT
      if (idx % pcl_cloud->width != 0) {
        e.a = idx;
        e.b = idx + pcl_cloud->width - 1;
        e.type = 1;
        e.w = color_dist[idx][3] / max_color;

        // normals angle
        angle = 0.0;
        if (std::isnan(pcl_cloud->points[idx].z))
          angle = 1.57;
        else
          angle = acos(Dot3(&normals->points[idx].normal[0], &normals->points[idx + pcl_cloud->width - 1].normal[0]));
        if (!std::isnan(angle))
          e.w2 = angle;
        else
          e.w2 = 1.57;

        z_dist = fabs(pcl_cloud->points[idx].z - pcl_cloud->points[idx + pcl_cloud->width - 1].z);
        if (!std::isnan(pcl_cloud->points[idx].z) && !std::isnan(pcl_cloud->points[idx + pcl_cloud->width - 1].z) &&
            z_dist < (z_adapt * pcl_cloud->points[idx].z))
          edges.push_back(e);
      } else {
        e.w = 1.0;
        z_dist = fabs(pcl_cloud->points[idx].z - pcl_cloud->points[idx + pcl_cloud->width - 1].z);
        if (!std::isnan(pcl_cloud->points[idx].z) && !std::isnan(pcl_cloud->points[idx + pcl_cloud->width - 1].z) &&
            z_dist < (z_adapt * pcl_cloud->points[idx].z))
          edges.push_back(e);
      }
    }
  }
  num_edges = edges.size();
  e = edges;
}
}  // namespace gc
