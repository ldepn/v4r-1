#include <pcl/io/io.h>
#include <pcl/io/pcd_io.h>
#include <v4r/common/miscellaneous.h>
#include <v4r/io/filesystem.h>
#include <v4r/object_modelling/incremental_object_learning.h>
#include <fstream>
#include <iostream>

#include <time.h>

#include <boost/any.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

//--do_erosion 1 --radius 0.005 --dot_product 0.99 --normal_method 0 -z 2 --transfer_latest_only 0
//--do_sift_based_camera_pose_estimation 0 -s /media/Data/datasets/TUW_new/test_set_icra16 -m
/// media/Data/datasets/TUW/TUW_gt_models_first -o /home/thomas/Desktop/out_TUW/

double getTimeDiff(timeval a, timeval b);

double getTimeDiff(timeval a, timeval b) {
  double first = a.tv_sec + (a.tv_usec / 1000000.0);
  double second = b.tv_sec + (b.tv_usec / 1000000.0);

  return (first - second) * 1000;
}

int main(int argc, char** argv) {
  typedef pcl::PointXYZRGB PointT;

  std::string scene_dir, input_mask_dir, output_dir = "/tmp/dol/";
  bool visualize = false;
  bool save_views = false;
  size_t min_mask_points = 50;
  bool first_frame_only = false;  // used for evaluation when only using the first view

  v4r::object_modelling::IOL m;

  po::options_description desc(
      "Evaluation Dynamic Object Learning with Ground Truth\n======================================\n **Allowed "
      "options");
  desc.add_options()("help,h", "produce help message");
  desc.add_options()("scenes_dir,s", po::value<std::string>(&scene_dir)->required(),
                     "input directory with .pcd files of the scenes. Each folder is "
                     "considered as separate sequence. Views are sorted "
                     "alphabetically and object mask is applied on first view.");
  desc.add_options()("input_mask_dir,m", po::value<std::string>(&input_mask_dir)->required(),
                     "directory containing the object masks used as a seed to learn the object in the first cloud");
  desc.add_options()(
      "output_dir,o", po::value<std::string>(&output_dir)->default_value(output_dir),
      "Output directory where the model, training data, timing information and parameter values will be stored");
  desc.add_options()(
      "save_views", po::bool_switch(&save_views),
      "if true, also saves point clouds, camera pose and object masks for each training views. This is necessary for "
      "recognition.");
  desc.add_options()(
      "radius,r", po::value<double>(&m.param_.radius_)->default_value(m.param_.radius_),
      "Radius used for region growing. Neighboring points within this distance are candidates for clustering it "
      "to the object model.");
  desc.add_options()(
      "dot_product", po::value<double>(&m.param_.eps_angle_)->default_value(m.param_.eps_angle_),
      "Threshold for the normals dot product used for region growing. Neighboring points with a surface normal "
      "within this threshold are candidates for clustering it to the object model.");
  desc.add_options()(
      "dist_threshold_growing",
      po::value<double>(&m.param_.dist_threshold_growing_)->default_value(m.param_.dist_threshold_growing_), "");
  desc.add_options()("seed_res",
                     po::value<double>(&m.param_.seed_resolution_)->default_value(m.param_.seed_resolution_), "");
  desc.add_options("voxel_res",
                   po::value<double>(&m.param_.voxel_resolution_)->default_value(m.param_.voxel_resolution_), "");
  desc.add_options()("ratio", po::value<double>(&m.param_.ratio_supervoxel_)->default_value(m.param_.ratio_supervoxel_),
                     "");
  desc.add_options()("do_erosion", po::value<bool>(&m.param_.do_erosion_)->default_value(m.param_.do_erosion_), "");
  desc.add_options()("do_mst_refinement",
                     po::value<bool>(&m.param_.do_mst_refinement_)->default_value(m.param_.do_mst_refinement_), "");
  desc.add_options()("transfer_latest_only",
                     po::value<bool>(&m.param_.transfer_indices_from_latest_frame_only_)
                         ->default_value(m.param_.transfer_indices_from_latest_frame_only_),
                     "");
  desc.add_options()("chop_z,z", po::value<double>(&m.param_.chop_z_)->default_value(m.param_.chop_z_),
                     "Cut-off distance of the input clouds with respect to the camera. Points further away than this "
                     "distance will be ignored.");
  desc.add_options()("normal_method,n",
                     po::value<int>(&m.param_.normal_method_)->default_value(m.param_.normal_method_), "");
  desc.add_options()(
      "ratio_cluster_obj_supported",
      po::value<double>(&m.param_.ratio_cluster_obj_supported_)->default_value(m.param_.ratio_cluster_obj_supported_),
      "");
  desc.add_options()(
      "ratio_cluster_occluded",
      po::value<double>(&m.param_.ratio_cluster_occluded_)->default_value(m.param_.ratio_cluster_occluded_), "");
  desc.add_options()("stat_outlier_removal_meanK",
                     po::value<int>(&m.sor_params_.meanK_)->default_value(m.sor_params_.meanK_),
                     "MeanK used for statistical outlier removal (see PCL documentation)");
  desc.add_options()("stat_outlier_removal_std_mul",
                     po::value<double>(&m.sor_params_.std_mul_)->default_value(m.sor_params_.std_mul_),
                     "Standard Deviation multiplier used for statistical outlier removal (see PCL documentation)");
  desc.add_options()("inlier_threshold_plane_seg",
                     po::value<double>(&m.p_param_.inlDist)->default_value(m.p_param_.inlDist), "");
  desc.add_options()("min_points_smooth_cluster",
                     po::value<int>(&m.p_param_.minPointsSmooth)->default_value(m.p_param_.minPointsSmooth),
                     "Minimum number of points for a cluster");
  desc.add_options()("min_plane_points", po::value<int>(&m.p_param_.minPoints)->default_value(m.p_param_.minPoints),
                     "Minimum number of points for a cluster to be a candidate for a plane");
  desc.add_options()("smooth_clustering",
                     po::value<bool>(&m.p_param_.smooth_clustering)->default_value(m.p_param_.smooth_clustering),
                     "If true, does smooth clustering. Otherwise only plane clustering.");
  desc.add_options()("visualize,v", po::bool_switch(&visualize), "turn visualization on");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return false;
  }

  try {
    po::notify(vm);
  } catch (std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl << std::endl << desc << std::endl;
    return false;
  }

  v4r::io::createDirIfNotExist(output_dir);

  ofstream param_file;
  param_file.open((output_dir + "/param.nfo").c_str());
  m.printParams(param_file);
  param_file << "stat_outlier_removal_meanK" << m.sor_params_.meanK_ << std::endl
             << "stat_outlier_removal_std_mul" << m.sor_params_.std_mul_ << std::endl
             << "inlier_threshold_plane_seg" << m.p_param_.inlDist << std::endl
             << "min_points_smooth_cluster" << m.p_param_.minPointsSmooth << std::endl
             << "min_plane_points" << m.p_param_.minPoints << std::endl;
  param_file.close();

  std::vector<std::string> sub_folder_names = v4r::io::getFoldersInDirectory(scene_dir);
  if (sub_folder_names.empty())
    sub_folder_names.push_back("");

  v4r::io::createDirIfNotExist(output_dir + "/models");

  for (const std::string& sub_folder_name : sub_folder_names) {
    const std::string output_rec_model = output_dir + "/" + sub_folder_name + "/models";
    v4r::io::createDirIfNotExist(output_rec_model);

    const std::string annotations_dir = input_mask_dir + "/" + sub_folder_name;
    std::vector<std::string> mask_file_v = v4r::io::getFilesInDirectory(annotations_dir, ".*.txt", false);

    for (size_t o_id = 0; o_id < mask_file_v.size(); o_id++) {
      const std::string mask_file = annotations_dir + "/" + mask_file_v[o_id];

      size_t idx_tmp;
      std::vector<size_t> mask;
      std::ifstream initial_mask_file(mask_file.c_str());
      while (initial_mask_file >> idx_tmp)
        mask.push_back(idx_tmp);

      initial_mask_file.close();

      if (mask.size() < min_mask_points)  // not enough points to grow an object
        continue;

      const std::string scene_path = scene_dir + "/" + sub_folder_name;
      std::vector<std::string> views = v4r::io::getFilesInDirectory(scene_path, ".*.pcd", false);

      std::cout << "Learning object from mask " << mask_file << " for scene " << scene_path << std::endl;

      timeval start, stop;

      gettimeofday(&start, NULL);
      for (size_t v_id = 0; v_id < views.size(); v_id++) {
        const std::string view_file = scene_path + "/" + views[v_id];
        pcl::PointCloud<PointT>::Ptr pCloud(new pcl::PointCloud<PointT>());
        pcl::io::loadPCDFile(view_file, *pCloud);
        const Eigen::Matrix4f trans = v4r::RotTrans2Mat4f(pCloud->sensor_orientation_, pCloud->sensor_origin_);

        pCloud->sensor_origin_ = Eigen::Vector4f::Zero();  // for correct visualization
        pCloud->sensor_orientation_ = Eigen::Quaternionf::Identity();

        if (v_id == 0)
          m.learn_object(*pCloud, trans, mask);
        else {
          if (!first_frame_only)
            m.learn_object(*pCloud, trans);
        }
      }
      gettimeofday(&stop, NULL);

      std::string out_fn = mask_file_v[o_id];
      boost::replace_last(out_fn, "mask.txt", "dol");
      m.save_model(output_rec_model, out_fn, save_views);
      if (visualize)
        m.visualize();
      m.clear();

      // write running time to file
      const std::string timing_fn = output_dir + "/" + sub_folder_name + "/timing.nfo";
      double learning_time = getTimeDiff(stop, start);
      ofstream f(timing_fn.c_str());
      f << learning_time;
      f.close();
    }
  }
  return 0;
}
