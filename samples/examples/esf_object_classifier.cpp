/*
 * Author: Thomas Faeulhammer
 * Date: Sept., 2015
 *
 * Segments PCD files and classifies them using the ESF descriptor
 *
 */

#include <v4r/features/esf_estimator.h>
#include <v4r/io/filesystem.h>
#include <v4r/ml/nearestNeighbor.h>
#include <v4r/ml/svmWrapper.h>
#include <v4r/recognition/global_recognizer.h>
#include <v4r/recognition/source.h>
#include <v4r/segmentation/all_headers.h>

#include <pcl/io/pcd_io.h>
#include <opencv2/opencv.hpp>

#include <glog/logging.h>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>

#include <fstream>
#include <sstream>

namespace po = boost::program_options;

using namespace v4r;

int main(int argc, char** argv) {
  typedef pcl::PointXYZ PointT;
  bool eval_only_closest_cluster = true;
  bf::path models_dir;
  bf::path test_dir;
  bf::path out_dir = "class_results";
  int knn;
  bool retrain = false;

  knn = 5;
  SegmentationType segmentation_method = SegmentationType::ORGANIZED_CONNECTED_COMPONENTS;

  google::InitGoogleLogging(argv[0]);

  po::options_description desc(
      "Depth-map and point cloud Rendering from mesh file\n======================================\n**Allowed options");
  desc.add_options()("help,h", "produce help message");
  desc.add_options()("models_dir,m", po::value<bf::path>(&models_dir)->required(), "model directory ");
  desc.add_options()("test_dir,t", po::value<bf::path>(&test_dir)->required(),
                     "directory containing *.pcd files for testing");
  desc.add_options()("out_dir,o", po::value<bf::path>(&out_dir)->default_value(out_dir), "output directory");
  desc.add_options()("eval_only_closest_cluster",
                     po::value<bool>(&eval_only_closest_cluster)->default_value(eval_only_closest_cluster),
                     "if true, evaluates only the closest segmented cluster with respect to the camera.");
  desc.add_options()("kNN,k", po::value<int>(&knn)->default_value(knn),
                     "defines the number k of nearest neighbor for classification");
  desc.add_options()("seg_method",
                     po::value<SegmentationType>(&segmentation_method)->default_value(segmentation_method),
                     "segmentation method used");
  desc.add_options()("retrain", po::bool_switch(&retrain),
                     "if true, retrains the model database no matter if they already exist");
  po::variables_map vm;
  po::parsed_options parsed = po::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
  std::vector<std::string> to_pass_further = po::collect_unrecognized(parsed.options, po::include_positional);
  po::store(parsed, vm);
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

  // ====== SETUP SEGMENTER ===============
  Segmenter<PointT>::Ptr segmenter = v4r::initSegmenter<PointT>(segmentation_method, to_pass_further);

  // ==== SETUP RECOGNIZER ======
  SourceParameter source_param;
  source_param.has_categories_ = true;
  Source<PointT>::Ptr model_database(new Source<PointT>(source_param));
  model_database->init(models_dir);
  ESFEstimation<PointT>::Ptr estimator(new ESFEstimation<PointT>);
  GlobalEstimator<PointT>::Ptr cast_estimator = std::dynamic_pointer_cast<ESFEstimation<PointT>>(estimator);

  GlobalRecognizer<PointT> rec;
  rec.setModelDatabase(model_database);
  rec.setFeatureEstimator(cast_estimator);
  //    NearestNeighborClassifier::Ptr classifier (new NearestNeighborClassifier);
  SVMParameter svmParam;
  svmParam.svm_.kernel_type = ::RBF;
  svmParam.svm_.gamma = 1. / 640.;
  svmParam.svm_.probability = 1;
  svmParam.knn_ = 3;

  svmClassifier::Ptr classifier(new svmClassifier(svmParam));
  rec.setClassifier(classifier);
  rec.initialize(models_dir, retrain);

  std::vector<std::string> sub_folders = io::getFoldersInDirectory(test_dir);
  if (sub_folders.empty())
    sub_folders.push_back("");

  for (const std::string& sub_folder : sub_folders) {
    const bf::path sequence_path = test_dir / sub_folder;
    const bf::path out_dir_full = out_dir / sub_folder;
    io::createDirIfNotExist(out_dir_full);

    std::vector<std::string> views = io::getFilesInDirectory(sequence_path, ".*.pcd", false);
    for (const std::string& view : views) {
      const bf::path fn = sequence_path / view;

      std::cout << "Segmenting file " << fn << std::endl;
      pcl::PointCloud<PointT>::Ptr cloud(new pcl::PointCloud<PointT>());
      pcl::io::loadPCDFile(fn.string(), *cloud);
      segmenter->setInputCloud(cloud);
      segmenter->segment();
      std::vector<std::vector<int>> found_clusters;
      segmenter->getSegmentIndices(found_clusters);

      if (found_clusters.empty()) {
        std::cerr << "Segmentation failed! Classifying whole image..." << std::endl;
        pcl::PointIndices all_idx;
        all_idx.indices.resize(cloud->points.size());
        for (size_t i = 0; i < cloud->points.size(); i++)
          all_idx.indices[i] = i;
      }

      std::vector<std::vector<std::string>> categories(found_clusters.size());
      std::vector<std::vector<float>> confidences(found_clusters.size());

      if (eval_only_closest_cluster) {  // only save classification result for cluster which is closest to the camera
                                        // (w.r.t. to centroid)
        int min_id = -1;
        double min_centroid = std::numeric_limits<double>::max();

        for (size_t i = 0; i < found_clusters.size(); i++) {
          typename pcl::PointCloud<PointT>::Ptr clusterXYZ(new pcl::PointCloud<PointT>());
          pcl::copyPointCloud(*cloud, found_clusters[i], *clusterXYZ);
          Eigen::Vector4f centroid;
          pcl::compute3DCentroid(*clusterXYZ, centroid);

          if (centroid[2] < min_centroid) {
            min_centroid = centroid[2];
            min_id = i;
          }
        }

        if (min_id >= 0) {
          std::vector<std::vector<int>> closest_cluster;
          closest_cluster.push_back(found_clusters[min_id]);
          found_clusters = closest_cluster;
        }
      }

      std::string out_fn = (out_dir_full / view).string();
      boost::replace_all(out_fn, ".pcd", ".anno_test");
      std::ofstream of(out_fn.c_str());

      typename pcl::PointCloud<PointT>::Ptr cloudXYZ(new pcl::PointCloud<PointT>());
      pcl::copyPointCloud(*cloud, *cloudXYZ);

      for (size_t i = 0; i < found_clusters.size(); i++) {
        GlobalRecognizer<PointT>::Cluster::Ptr cluster(
            new GlobalRecognizer<PointT>::Cluster(*cloudXYZ, found_clusters[i], false));
        rec.setInputCloud(cloudXYZ);
        rec.setCluster(cluster);
        rec.recognize();
        std::vector<ObjectHypothesis::Ptr> ohs = rec.getHypotheses();

        for (ObjectHypothesis::Ptr oh : ohs)
          std::cout << oh->model_id_ << " " << oh->class_id_ << "(" << oh->confidence_ << ")" << std::endl;

        //                of << categories[i][0] << " " << confidences[i][0] << " ";
        of << std::endl;
      }
      of.close();
    }
  }
  return 0;
}
