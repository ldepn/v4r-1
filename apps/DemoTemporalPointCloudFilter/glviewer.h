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

#ifndef _GRAB_PCD_GL_VIEWER_H_
#define _GRAB_PCD_GL_VIEWER_H_

#ifndef Q_MOC_RUN
#include <qgraphicswidget.h>
#include <QElapsedTimer>
#include <QGLWidget>
#include <QMouseEvent>
#include <QMutex>
#include <QThread>
#include <QTimer>

#include <boost/smart_ptr.hpp>
#include <string>
#include <vector>

#include <pcl/octree/octree.h>
#include <pcl/octree/octree_pointcloud_voxelcentroid.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <opencv2/opencv.hpp>
#include <v4r/camera_tracking_and_mapping/Surfel.hh>
#include <v4r/common/impl/DataMatrix2D.hpp>

#include "Camera.h"
#include "sensor.h"

#include <QGraphicsView>
#endif

class GLGraphicsView : public QGraphicsView {
  Q_OBJECT

 signals:
  void mouse_moved(QMouseEvent *event);
  void mouse_pressed(QMouseEvent *event);
  void key_pressed(QKeyEvent *event);
  void wheel_event(QWheelEvent *event);

 public:
  GLGraphicsView(QWidget *widget = 0) : QGraphicsView(widget) {}

  void mouseMoveEvent(QMouseEvent *event) {
    emit mouse_moved(event);
  }

  void mousePressEvent(QMouseEvent *event) {
    emit mouse_pressed(event);
  }

  void keyPressEvent(QKeyEvent *event) {
    emit key_pressed(event);
  }

  void wheelEvent(QWheelEvent *_event) {
    emit wheel_event(_event);
  }
};

class GLViewer : public QGLWidget {
  Q_OBJECT

 public:
  //! Default constructor.
  GLViewer(QWidget *_parent = 0);

  //! Destructor.
  virtual ~GLViewer();

  void showImage(bool enable);
  void showCloud(bool enable);
  void showCameras(bool enable);
  void showNormals(bool enable);
  void resetView(float fw = 0.);

 signals:
  // void segment_image(int x, int y);

 public slots:
  void draw();

  void new_image(const pcl::PointCloud<pcl::PointXYZRGB>::Ptr &_cloud, const cv::Mat_<cv::Vec3b> &_image,
                 const double &_ts);
  void new_image(const cv::Mat_<cv::Vec3b> &_image, const double &_ts);
  void new_cloud(const pcl::PointCloud<pcl::PointXYZRGB>::Ptr &_cloud, const double &_ts);
  void new_sf_cloud(const v4r::DataMatrix2D<v4r::Surfel>::Ptr &_sf_cloud, const double &_ts);
  void update_visualization();

  void mouse_moved(QMouseEvent *event);
  void mouse_pressed(QMouseEvent *event);
  void key_pressed(QKeyEvent *event);
  void wheel_event(QWheelEvent *event);

 private:
  //! Initializes OpenGL states (triggered by Qt).
  void initializeGL();

  //! Draws a coordinate frame at the origin (0,0,0).
  void drawCoordinates(float length = 1.0, const Eigen::Matrix4f &pose = Eigen::Matrix4f::Identity());

  //! Grabs an Image and draws it.
  void drawImage();

  //! Draws the scene (triggered by Qt).
  void paintGL();

  //! Handle resize events (triggered by Qt).
  void resizeGL(int w, int h);

  void initCamera();

  tg::Camera m_cam_origin;
  tg::Camera m_cam_perspective;
  tg::Camera m_cam_ortho;

  Eigen::Vector4f pt00, pt0x, pt0y, pt0z;
  Eigen::Vector4f pt10, pt1x, pt1y, pt1z;

  size_t m_width;
  size_t m_height;

  float m_point_size;

  QPoint m_last_point_2d;
  QElapsedTimer m_elapsed;
  QTimer m_timer;

  pcl::PointCloud<pcl::PointXYZRGB>::Ptr m_cloud;
  cv::Mat_<cv::Vec3b> m_image;
  v4r::DataMatrix2D<v4r::Surfel>::Ptr m_sf_cloud;
  double ts_cloud, ts_image, ts_sf_cloud;

  Eigen::Vector3f oc_center;

  QMutex cam_mutex, oc_mutex, bb_mutex;

  bool m_show_image;
  bool m_show_cloud;
  bool m_show_cameras;
  bool m_show_normals;

  glm::vec3 cor;

  GLuint m_texture_id;

 protected:
  // Qt mouse events
  virtual void mousePressEvent(QMouseEvent *event);
  virtual void mouseMoveEvent(QMouseEvent *event);
  virtual void wheelEvent(QWheelEvent *event);
  virtual void keyPressEvent(QKeyEvent *event);
};

#endif  // GLWIDGET_H
