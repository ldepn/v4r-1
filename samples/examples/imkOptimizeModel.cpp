/******************************************************************************
 * Copyright (c) 2017 Johann Prankl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 ******************************************************************************/

#include <float.h>
#include <pcl/common/centroid.h>
#include <pcl/common/time.h>
#include <pcl/io/pcd_io.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <v4r/io/filesystem.h>
#include <v4r/recognition/IMKOptimizeModel.h>
#include <v4r_config.h>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdexcept>
#include <v4r/keypoints/impl/PoseIO.hpp>
#include <v4r/keypoints/impl/invPose.hpp>
#include <v4r/keypoints/impl/toString.hpp>
#include <v4r/reconstruction/impl/projectPointToImage.hpp>
#include "pcl/common/transforms.h"
//#include <v4r/features/FeatureDetector_KD_FAST_IMGD.h>
#ifdef HAVE_SIFTGPU
#define USE_SIFT_GPU
#include <v4r/features/FeatureDetector_KD_SIFTGPU.h>
#else
#include <v4r/features/FeatureDetector_KD_CVSIFT.h>
#endif

#include <pcl/common/time.h>

using namespace std;
namespace po = boost::program_options;

void drawConfidenceBar(cv::Mat &im, const double &conf);
cv::Point2f drawCoordinateSystem(cv::Mat &im, const Eigen::Matrix4f &pose, const cv::Mat_<double> &intrinsic,
                                 const cv::Mat_<double> &dist_coeffs, double size, int thickness);
void convertImage(const pcl::PointCloud<pcl::PointXYZRGB> &cloud, cv::Mat &image);

//--------------------------- default configuration -------------------------------

void InitParameter();

void InitParameter() {}

//------------------------------ helper methods -----------------------------------
void setup(int argc, char **argv);

//----------------------------- data containers -----------------------------------
cv::Mat_<cv::Vec3b> image;
cv::Mat_<cv::Vec3b> im_draw;
pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZRGB>);

cv::Mat_<double> dist_coeffs;  // = cv::Mat::zeros(4, 1, CV_64F);
cv::Mat_<double> intrinsic = cv::Mat_<double>::eye(3, 3);
Eigen::Matrix4f pose = Eigen::Matrix4f::Identity();

int ul_lr = 0;
int start = 0, end_idx = 10;
cv::Point track_win[2];
std::string cam_file;
string base_dir;
std::vector<std::string> object_names;
double thr_conf = 0;

int live = -1;
bool loop = false;

/******************************************************************
 * MAIN
 */
int main(int argc, char *argv[]) {
  track_win[0] = cv::Point(0, 0);
  track_win[1] = cv::Point(640, 480);

  // config
  InitParameter();

  setup(argc, argv);

  intrinsic(0, 0) = intrinsic(1, 1) = 525;
  intrinsic(0, 2) = 320, intrinsic(1, 2) = 240;
  if (cam_file.size() > 0) {
    cv::FileStorage fs(cam_file, cv::FileStorage::READ);
    fs["camera_matrix"] >> intrinsic;
    fs["distortion_coefficients"] >> dist_coeffs;
  }

  cv::namedWindow("image", CV_WINDOW_AUTOSIZE);

  // init recognizer
  v4r::IMKOptimizeModel::Parameter param;
  param.pnp_param.eta_ransac = 0.01;
  param.pnp_param.max_rand_trials = 5000;
  param.pnp_param.inl_dist_px = 1.5;
  param.pnp_param.inl_dist_z = 0.015;

#ifdef USE_SIFT_GPU
  v4r::FeatureDetector::Ptr detector(new v4r::FeatureDetector_KD_SIFTGPU());
#else
  v4r::FeatureDetector::Ptr detector(new v4r::FeatureDetector_KD_CVSIFT());
#endif

  v4r::IMKOptimizeModel opti(param, detector, detector);

  opti.setCameraParameter(intrinsic, dist_coeffs);
  opti.setDataDirectory(base_dir);

  if (object_names.size() == 0) {  // take all direcotry names from the base_dir
    object_names = v4r::io::getFoldersInDirectory(base_dir);
  }

  for (unsigned i = 0; i < object_names.size(); i++)
    opti.addObject(object_names[i]);

  opti.loadAllObjectViews();
  opti.optimize();

  cv::VideoCapture cap;
  if (live != -1) {
    cap.open(live);
    cap.set(CV_CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
    loop = true;
    if (!cap.isOpened()) {
      cout << "Could not initialize capturing...\n";
      return 0;
    }
  }

  cv::waitKey(0);

  return 0;
}

/******************************** SOME HELPER METHODS **********************************/

/**
 * setup
 */
void setup(int argc, char **argv) {
  po::options_description general("General options");
  general.add_options()("help,h", "show help message")(
      "base_dir,d", po::value<std::string>(&base_dir)->default_value(base_dir), "Object model directory")(
      "object_names,n", po::value<std::vector<std::string>>(&object_names)->multitoken(), "Object names")(
      "cam_file,a", po::value<std::string>(&cam_file)->default_value(cam_file),
      "camera calibration files (opencv format)");

  po::options_description all("");
  all.add(general);

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).options(all).run(), vm);
  po::notify(vm);
  std::string usage = "";

  if (vm.count("help")) {
    std::cout << usage << std::endl;
    std::cout << all;
    exit(0);
  }
}

/**
 * drawConfidenceBar
 */
void drawConfidenceBar(cv::Mat &im, const double &conf) {
  int bar_start = 50, bar_end = 200;
  int diff = bar_end - bar_start;
  int draw_end = diff * conf;
  double col_scale = 255. / (double)diff;
  cv::Point2f pt1(0, 30);
  cv::Point2f pt2(0, 30);
  cv::Vec3b col(0, 0, 0);

  if (draw_end <= 0)
    draw_end = 1;

  for (int i = 0; i < draw_end; i++) {
    col = cv::Vec3b(255 - (i * col_scale), i * col_scale, 0);
    pt1.x = bar_start + i;
    pt2.x = bar_start + i + 1;
    cv::line(im, pt1, pt2, CV_RGB(col[0], col[1], col[2]), 8);
  }
}

cv::Point2f drawCoordinateSystem(cv::Mat &im, const Eigen::Matrix4f &_pose, const cv::Mat_<double> &_intrinsic,
                                 const cv::Mat_<double> &_dist_coeffs, double size, int thickness) {
  Eigen::Matrix3f R = _pose.topLeftCorner<3, 3>();
  Eigen::Vector3f t = _pose.block<3, 1>(0, 3);

  Eigen::Vector3f pt0 = R * Eigen::Vector3f(0, 0, 0) + t;
  Eigen::Vector3f pt_x = R * Eigen::Vector3f(size, 0, 0) + t;
  Eigen::Vector3f pt_y = R * Eigen::Vector3f(0, size, 0) + t;
  Eigen::Vector3f pt_z = R * Eigen::Vector3f(0, 0, size) + t;

  cv::Point2f im_pt0, im_pt_x, im_pt_y, im_pt_z;

  if (!_dist_coeffs.empty()) {
    v4r::projectPointToImage(&pt0[0], &_intrinsic(0), &_dist_coeffs(0), &im_pt0.x);
    v4r::projectPointToImage(&pt_x[0], &_intrinsic(0), &_dist_coeffs(0), &im_pt_x.x);
    v4r::projectPointToImage(&pt_y[0], &_intrinsic(0), &_dist_coeffs(0), &im_pt_y.x);
    v4r::projectPointToImage(&pt_z[0], &_intrinsic(0), &_dist_coeffs(0), &im_pt_z.x);
  } else {
    v4r::projectPointToImage(&pt0[0], &_intrinsic(0), &im_pt0.x);
    v4r::projectPointToImage(&pt_x[0], &_intrinsic(0), &im_pt_x.x);
    v4r::projectPointToImage(&pt_y[0], &_intrinsic(0), &im_pt_y.x);
    v4r::projectPointToImage(&pt_z[0], &_intrinsic(0), &im_pt_z.x);
  }

  cv::line(im, im_pt0, im_pt_x, CV_RGB(255, 0, 0), thickness);
  cv::line(im, im_pt0, im_pt_y, CV_RGB(0, 255, 0), thickness);
  cv::line(im, im_pt0, im_pt_z, CV_RGB(0, 0, 255), thickness);

  return im_pt0;
}

void convertImage(const pcl::PointCloud<pcl::PointXYZRGB> &_cloud, cv::Mat &_image) {
  _image = cv::Mat_<cv::Vec3b>(_cloud.height, _cloud.width);

  for (unsigned v = 0; v < _cloud.height; v++) {
    for (unsigned u = 0; u < _cloud.width; u++) {
      cv::Vec3b &cv_pt = _image.at<cv::Vec3b>(v, u);
      const pcl::PointXYZRGB &pt = _cloud(u, v);

      cv_pt[2] = pt.r;
      cv_pt[1] = pt.g;
      cv_pt[0] = pt.b;
    }
  }
}
