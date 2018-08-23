#include <glog/logging.h>
#include <pcl/common/angles.h>
#include <pcl/recognition/cg/geometric_consistency.h>
#include <pcl/registration/correspondence_rejection_sample_consensus.h>
#include <pcl/registration/transformation_estimation_svd.h>
#include <pcl_1_8/features/organized_edge_detection.h>
#include <pcl_1_8/keypoints/uniform_sampling.h>
#include <v4r/common/graph_geometric_consistency.h>
#include <v4r/common/miscellaneous.h>
#include <v4r/common/visibility_reasoning.h>
#include <v4r/registration/fast_icp_with_gc.h>

namespace v4r {

template <typename PointT>
std::vector<int> computeRGBEdges(const typename pcl::PointCloud<PointT>::ConstPtr& cloud) {
  std::vector<int> rgb_edge_indices;
  pcl_1_8::OrganizedEdgeFromRGB<PointT, pcl::Label> oed;
  oed.setDepthDisconThreshold(0.03f);
  oed.setRGBCannyLowThreshold(150.f);
  oed.setRGBCannyHighThreshold(200.f);
  oed.setEdgeType(pcl_1_8::OrganizedEdgeBase<PointT, pcl::Label>::EDGELABEL_RGB_CANNY);
  oed.setInputCloud(cloud);

  pcl::PointCloud<pcl::Label>::Ptr labels(new pcl::PointCloud<pcl::Label>);
  std::vector<pcl::PointIndices> indices2;
  oed.compute(*labels, indices2);

  for (const pcl::PointIndices& ind : indices2) {
    for (const auto& idx : ind.indices) {
      if (pcl_isfinite(cloud->points[idx].z))
        rgb_edge_indices.push_back(idx);
    }
  }
  return rgb_edge_indices;
}

template <typename PointT>
inline bool FastIterativeClosestPointWithGC<PointT>::filterHypothesesByPose(
    typename std::shared_ptr<ICPNode<PointT>>& current, typename std::vector<std::shared_ptr<ICPNode<PointT>>>& nodes,
    float trans_threshold) {
  bool found = false;
  // Eigen::Vector4f origin = Eigen::Vector4f(0,0,0,1);
  Eigen::Vector4f origin;
  pcl::compute3DCentroid(*input_, origin);
  Eigen::Vector4f origin_trans = current->accum_transform_ * origin;
  Eigen::Vector4f trans = origin_trans.block<4, 1>(0, 0);

  // Eigen::Quaternionf quat(static_cast<Eigen::Matrix4f>(current->accum_transform_).block<3, 3>(0, 0));
  // quat.normalize();
  // Eigen::Quaternionf quat_conj = quat.conjugate();

  for (const auto n : nodes) {
    Eigen::Vector4f origin_node = n->accum_transform_ * origin;
    Eigen::Vector4f trans_found = origin_node.block<4, 1>(0, 0);
    if ((trans - trans_found).norm() < trans_threshold) {
      // Eigen::Quaternionf quat_found(static_cast<Eigen::Matrix4f>(nodes[i]->accum_transform_).block<3, 3>(0, 0));
      // quat_found.normalize();
      // Eigen::Quaternionf quat_prod = quat_found * quat_conj;
      // float angle = static_cast<float>(acos(quat_prod.z()));
      // angle = std::abs(90.f - pcl::rad2deg(angle));
      // std::cout << angle << std::endl;
      // if(angle < 15.f)
      //{
      found = true;
      //}
      break;
    }
  }

  return found;
}

template <typename PointT>
inline void FastIterativeClosestPointWithGC<PointT>::visualizeICPNodes(
    typename std::vector<std::shared_ptr<ICPNode<PointT>>>& nodes, pcl::visualization::PCLVisualizer& icp_vis) {
  int k = 0, l = 0, viewport = 0;
  int y_s = 0, x_s = 0;
  double x_step = 0, y_step = 0;
  y_s = static_cast<int>(floor(sqrt(static_cast<float>(nodes.size()))));
  x_s = y_s + static_cast<int>(ceil(double(nodes.size()) / double(y_s) - y_s));
  x_step = 1.0 / static_cast<double>(x_s);
  y_step = 1.0 / static_cast<double>(y_s);

  for (size_t i = 0; i < nodes.size(); i++) {
    icp_vis.createViewPort(k * x_step, l * y_step, (k + 1) * x_step, (l + 1) * y_step, viewport);
    k++;
    if (k >= x_s) {
      k = 0;
      l++;
    }

    PointTPtr input_transformed(new pcl::PointCloud<PointT>);
    pcl::transformPointCloud(*input_, *input_transformed, nodes[i]->accum_transform_);

    std::stringstream cloud_name;
    cloud_name << "input_" << i;

    // pcl::visualization::PointCloudColorHandlerCustom<PointSource> color_handler (input_transformed, 255, 0, 0);
    // icp_vis.addPointCloud (input_transformed, color_handler, cloud_name.str (), viewport);
    PointTPtr pp = input_transformed;
    float rgb_m;
    bool exists_m;

    typedef pcl::PointCloud<PointT> CloudM;
    typedef typename pcl::traits::fieldList<typename CloudM::PointType>::type FieldListM;

    pcl::for_each_type<FieldListM>(
        pcl::CopyIfFieldExists<typename CloudM::PointType, float>(pp->points[0], "rgb", exists_m, rgb_m));
    if (exists_m) {
      pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_rgb(new pcl::PointCloud<pcl::PointXYZRGB>);
      if (input_indices_.indices.size() > 0) {
        pcl::copyPointCloud(*pp, input_indices_.indices, *cloud_rgb);
      } else {
        pcl::copyPointCloud(*pp, *cloud_rgb);
      }
      pcl::visualization::PointCloudColorHandlerRGBField<pcl::PointXYZRGB> handler_rgb(cloud_rgb);
      icp_vis.addPointCloud<pcl::PointXYZRGB>(cloud_rgb, handler_rgb, cloud_name.str(), viewport);
    } else {
      pcl::visualization::PointCloudColorHandlerCustom<PointT> handler_rgb(pp, 255, 0, 0);
      icp_vis.addPointCloud<PointT>(pp, handler_rgb, cloud_name.str(), viewport);
    }

    cloud_name << "_text";
    char cVal[32];
    sprintf(cVal, "%f", nodes[i]->reg_error_);

    std::cout << "OSV:" << nodes[i]->osv_fraction_ << std::endl;
    std::cout << "FSV:" << nodes[i]->fsv_fraction_ << std::endl;
    icp_vis.addText(cVal, 5, 5, 10, 1.0, 1.0, 1.0, cloud_name.str(), viewport);
  }

  typename pcl::PointCloud<PointT>::Ptr vis_cloud(new pcl::PointCloud<PointT>);

  if (!target_indices_.indices.empty())
    pcl::copyPointCloud(*target_, target_indices_.indices, *vis_cloud);
  else
    pcl::copyPointCloud(*target_, *vis_cloud);

  icp_vis.addPointCloud<PointT>(vis_cloud, "target");
  icp_vis.spin();
}

template <typename PointT>
std::vector<float> FastIterativeClosestPointWithGC<PointT>::evaluateHypotheses(
    typename pcl::PointCloud<PointT>::ConstPtr im1, typename pcl::PointCloud<PointT>::ConstPtr im_2,
    pcl::PointCloud<pcl::Normal>::ConstPtr normals1, pcl::PointCloud<pcl::Normal>::ConstPtr normals_2,
    const Eigen::Matrix4f& pose_2_to_1) {
  std::vector<float> ret_values;

  // transform im2
  PointTPtr im2(new pcl::PointCloud<PointT>);
  pcl::PointCloud<pcl::Normal>::Ptr normals2(new pcl::PointCloud<pcl::Normal>);

  pcl::transformPointCloud(*im_2, *im2, pose_2_to_1);
  v4r::transformNormals(*normals_2, *normals2, pose_2_to_1);

  CHECK(im2->points.size() == normals2->points.size());

  int keep = 0;
  float normal_dots = 0.f;
  int image_width = static_cast<int>(cam_.w);
  int image_height = static_cast<int>(cam_.h);

  for (size_t i = 0; i < im2->points.size(); i++) {
    if (!pcl_isfinite(im2->points[i].z) || !pcl_isfinite(normals2->points[i].normal_z))
      continue;

    float x = im2->points[i].x;
    float y = im2->points[i].y;
    float z = im2->points[i].z;
    int u = static_cast<int>(cam_.fx * x / z + cam_.cx);
    int v = static_cast<int>(cam_.fy * y / z + cam_.cy);

    if (u >= image_width || v >= image_height || u < 0 || v < 0)
      continue;

    // Check if point depth (distance to camera) is greater than the (u,v) meaning that the point is not visible
    if (!pcl_isfinite(im1->at(u, v).z) || !pcl_isfinite(normals1->at(u, v).normal_z))
      continue;

    normal_dots += normals1->at(u, v).getNormalVector4fMap().dot(normals2->points[i].getNormalVector4fMap());
    keep++;
  }

  ret_values.push_back(normal_dots / static_cast<float>(keep));

  return ret_values;
}

template <typename PointT>
void FastIterativeClosestPointWithGC<PointT>::align(const Eigen::Matrix4f initial_guess) {
#ifdef FAAT_PCL_FAST_ICP_VIS_FINAL
  pcl::visualization::PCLVisualizer survived_vis("survived VIS...");
#endif

  // compute keypoints (uniform sampling, color edges)
  std::vector<int> ind_src_cedges = computeRGBEdges<PointT>(input_);
  std::vector<int> ind_tgt_cedges = computeRGBEdges<PointT>(target_);

  if (!input_indices_.indices.empty() && !target_indices_.indices.empty()) {
    LOG(INFO) << "Input and target indices were set, use them to filter keypoints" << input_indices_.indices.size()
              << " " << target_indices_.indices.size() << std::endl;
    std::vector<int> indices_src_roi, indices_tgt_roi;
    getKeypointsWithMask(target_, ind_tgt_cedges, target_indices_.indices, indices_tgt_roi);
    getKeypointsWithMask(input_, ind_src_cedges, input_indices_.indices, indices_src_roi);
    ind_src_cedges = indices_src_roi;
    ind_tgt_cedges = indices_tgt_roi;
  }

  PointTPtr src_keypoints(new pcl::PointCloud<PointT>);
  PointTPtr tgt_keypoints(new pcl::PointCloud<PointT>);

  pcl::PointCloud<pcl::Normal>::Ptr normal_tgt_keypoints(new pcl::PointCloud<pcl::Normal>);
  pcl::PointCloud<pcl::Normal>::Ptr normal_src_keypoints(new pcl::PointCloud<pcl::Normal>);

  pcl::copyPointCloud(*input_, ind_src_cedges, *src_keypoints);
  pcl::copyPointCloud(*target_, ind_tgt_cedges, *tgt_keypoints);

  if (input_normals_) {
    pcl::copyPointCloud(*target_normals_, ind_tgt_cedges, *normal_tgt_keypoints);
    pcl::copyPointCloud(*input_normals_, ind_src_cedges, *normal_src_keypoints);
  }

  UniformSamplingSharedVoxelGrid<PointT> keypoint_extractor;
  keypoint_extractor.setRadiusSearch(uniform_sampling_radius_);

  // pcl::UniformSampling<PointT> keypoint_extractor;
  // keypoint_extractor.setRadiusSearch (radius);
  keypoint_extractor.setInputCloud(tgt_keypoints);
  pcl::PointCloud<int> keypoints;
  keypoint_extractor.compute(keypoints);

  std::vector<int> ind_tgt(keypoints.points.size());
  for (size_t i = 0; i < keypoints.points.size(); i++) {
    ind_tgt[i] = ind_tgt_cedges[keypoints[i]];
  }

  pcl::copyPointCloud(*target_, ind_tgt, *tgt_keypoints);
  if (target_normals_)
    pcl::copyPointCloud(*target_normals_, ind_tgt, *normal_tgt_keypoints);

  Eigen::Vector4f min_b, max_b;
  keypoint_extractor.getVoxelGridValues(min_b, max_b);

  typename std::vector<std::shared_ptr<ICPNode<PointT>>>
      alive_nodes_;  // contains nodes that need to be processed at a certain ICP iteration

  // create root_node
  if (initial_poses_.empty()) {
    typename std::shared_ptr<ICPNode<PointT>> root_node_(new ICPNode<PointT>(0, true));
    root_node_->accum_transform_ = initial_guess;
    alive_nodes_.push_back(root_node_);
  } else {
    for (size_t i = 0; i < initial_poses_.size(); i++) {
      typename std::shared_ptr<ICPNode<PointT>> root_node_(new ICPNode<PointT>(0, true));
      root_node_->accum_transform_ = initial_poses_[i];
      alive_nodes_.push_back(root_node_);
    }
  }

  size_t nr_iterations = 0;
  bool converged = false;

  // pcl::visualization::PCLVisualizer vis_corresp;
  // vis_corresp.addPointCloud(tgt_keypoints, "tgT_keypoints");

  do {
    typename std::vector<std::shared_ptr<ICPNode<PointT>>> next_level_nodes_;
    typename std::vector<std::vector<std::shared_ptr<ICPNode<PointT>>>> next_level_nodes_by_parent_;
    next_level_nodes_by_parent_.resize(alive_nodes_.size());

    // survived_vis.removeAllPointClouds ();
    // survived_vis.removeAllShapes ();
    // visualizeICPNodes (alive_nodes_, survived_vis, "in the loop");

    //#pragma omp parallel for num_threads(8)
    for (size_t an = 0; an < alive_nodes_.size(); an++) {
      // transform input cloud
      typename std::shared_ptr<ICPNode<PointT>> cur_node = alive_nodes_[an];

      PointTPtr src_keypoints_local(new pcl::PointCloud<PointT>);

      UniformSamplingSharedVoxelGrid<PointT> _keypoint_extractor;
      _keypoint_extractor.setRadiusSearch(uniform_sampling_radius_);
      _keypoint_extractor.setVoxelGridValues(min_b, max_b);

      // pcl::UniformSampling<PointT> keypoint_extractor;
      // keypoint_extractor.setRadiusSearch (radius);
      keypoint_extractor.setInputCloud(src_keypoints);
      pcl::PointCloud<int> keypoints;
      keypoint_extractor.compute(keypoints);

      std::vector<int> ind_src(keypoints.points.size());
      for (size_t i = 0; i < keypoints.points.size(); i++) {
        ind_src[i] = ind_src_cedges[keypoints[i]];
      }

      pcl::PointCloud<pcl::Normal>::Ptr normal_src_keypoints_local(new pcl::PointCloud<pcl::Normal>);

      pcl::copyPointCloud(*input_, ind_src, *src_keypoints_local);
      if (input_normals_) {
        pcl::copyPointCloud(*input_normals_, ind_src, *normal_src_keypoints_local);
        transformNormals(*normal_src_keypoints_local, *normal_src_keypoints_local, cur_node->accum_transform_);
      }

      PointTPtr src_keypoints_local_trans(new pcl::PointCloud<PointT>);
      pcl::transformPointCloud(*src_keypoints_local, *src_keypoints_local_trans, cur_node->accum_transform_);
      src_keypoints_local = src_keypoints_local_trans;

      // get correspondences
      pcl::CorrespondencesPtr correspondences_alive_node(new pcl::Correspondences);

      {
        // pcl::ScopeTime t("Compute correspondences...");
        typename std::shared_ptr<pcl::registration::CorrespondenceEstimation<PointT, PointT, float>> corresp_est;
        corresp_est.reset(new pcl::registration::CorrespondenceEstimation<PointT, PointT, float>);

        corresp_est->setInputSource(src_keypoints_local);
        corresp_est->setInputTarget(tgt_keypoints);
        corresp_est->determineCorrespondences(*correspondences_alive_node, corr_dist_threshold_);

        //              corresp_est->setInputSource (tgt_keypoints);
        //              corresp_est->setInputTarget (src_keypoints_local);
        //
        //              pcl::CorrespondencesPtr reverse_correspondences (new pcl::Correspondences ());
        //              corresp_est->determineCorrespondences (*reverse_correspondences, corr_dist_threshold_);
        //
        //              for (size_t i = 0; i < reverse_correspondences->size (); i++)
        //              {
        //                pcl::Correspondence rev_corresp;
        //                rev_corresp.index_match = (*reverse_correspondences)[i].index_query;
        //                rev_corresp.index_query = (*reverse_correspondences)[i].index_match;
        //                rev_corresp.distance = (*reverse_correspondences)[i].distance;
        //                correspondences_alive_node->push_back (rev_corresp);
        //              }
      }

      // vis_corresp.addPointCloud(src_keypoints_local, "src");
      // vis_corresp.spin();
      // vis_corresp.removePointCloud("src");

      std::vector<pcl::Correspondences> corresp_clusters;
      if (no_cg_) {
        corresp_clusters.push_back(*correspondences_alive_node);
      } else {
        if (!standard_cg_) {
          // pcl::ScopeTime t ("GraphGeometricConsistencyGrouping...");
          GraphGeometricConsistencyGroupingParameter gcg_param;
          gcg_param.gc_threshold_ = min_number_correspondences_;
          gcg_param.gc_size_ = gc_size_;
          gcg_param.ransac_threshold_ = ransac_threshold_;
          gcg_param.dist_for_cluster_factor_ = 1.f;
          gcg_param.thres_dot_distance_ = 0.25f;
          gcg_param.use_graph_ = false;
          gcg_param.prune_ = false;
          GraphGeometricConsistencyGrouping<PointT, PointT> gcg_alg(gcg_param);
          gcg_alg.setModelSceneCorrespondences(correspondences_alive_node);
          gcg_alg.setSceneCloud(tgt_keypoints);
          gcg_alg.setInputCloud(src_keypoints_local);
          gcg_alg.setInputAndSceneNormals(normal_src_keypoints_local, normal_tgt_keypoints);
          gcg_alg.cluster(corresp_clusters);
        } else {
          // pcl::ScopeTime t ("GeometricConsistencyGrouping...");
          pcl::GeometricConsistencyGrouping<PointT, PointT> gcg_alg;
          gcg_alg.setGCThreshold(min_number_correspondences_);
          gcg_alg.setGCSize(gc_size_);
          gcg_alg.setSceneCloud(tgt_keypoints);
          gcg_alg.setInputCloud(src_keypoints_local);
          gcg_alg.setModelSceneCorrespondences(correspondences_alive_node);
          gcg_alg.cluster(corresp_clusters);
        }
      }

      typename pcl::registration::CorrespondenceRejectorSampleConsensus<PointT>::Ptr rej;
      rej.reset(new pcl::registration::CorrespondenceRejectorSampleConsensus<PointT>());

      {
        // pcl::ScopeTime t ("ransac, THE");
        // go through clusters and compute things
        next_level_nodes_by_parent_[an].reserve(corresp_clusters.size());
        int kept = 0;
        for (size_t kk = 0; kk < corresp_clusters.size(); kk++) {
          pcl::CorrespondencesPtr temp_correspondences(new pcl::Correspondences(corresp_clusters[kk]));
          pcl::CorrespondencesPtr after_rej_correspondences(new pcl::Correspondences());

          rej->setMaximumIterations(10000);
          rej->setInlierThreshold(ransac_threshold_);

          // rej->setInputTarget (target_);
          // rej->setInputSource (input_transformed_local_);
          rej->setInputSource(src_keypoints_local);
          rej->setInputTarget(tgt_keypoints);
          rej->setInputCorrespondences(temp_correspondences);
          rej->getCorrespondences(*after_rej_correspondences);

          size_t cnt = after_rej_correspondences->size();
          if (cnt >= min_number_correspondences_) {
            Eigen::Matrix4f transformation = rej->getBestTransformation();
            typename pcl::registration::TransformationEstimationSVD<PointT, PointT> t_est;
            // t_est.estimateRigidTransformation (*input_transformed_local_, *target_, *after_rej_correspondences,
            // transformation);
            t_est.estimateRigidTransformation(*src_keypoints_local, *tgt_keypoints, *after_rej_correspondences,
                                              transformation);

            typename std::shared_ptr<ICPNode<PointT>> child(new ICPNode<PointT>(nr_iterations + 1));
            child->accum_transform_ = transformation * cur_node->accum_transform_;
            child->src_keypoints_ = src_keypoints_local;
            child->normal_src_keypoints_ = normal_src_keypoints_local;
            // child->src_keypoints_ = src_keypoints;
            // child->normal_src_keypoints_ = normal_src_keypoints;
            next_level_nodes_by_parent_[an].push_back(child);
            kept++;
          }
        }

        next_level_nodes_by_parent_[an].resize(kept);
      }
    }

    alive_nodes_.clear();
    for (size_t i = 0; i < next_level_nodes_by_parent_.size(); i++) {
      for (size_t j = 0; j < next_level_nodes_by_parent_[i].size(); j++)
        next_level_nodes_.push_back(next_level_nodes_by_parent_[i][j]);
    }

    // now its time to evaluate the hypotheses!
    {
      // std::cout << "Num hyp to evaluate:" << next_level_nodes_.size() << std::endl;
      // pcl::ScopeTime t ("Evaluate hypotheses...");
      //            float osv_cutoff = 0.1f;
      float fsv_cutoff = 0.02f;
      //#pragma omp parallel for num_threads(8)
      for (const auto& next_level_node : next_level_nodes_) {
        size_t max_points = std::min(next_level_node->src_keypoints_->points.size(), tgt_keypoints->points.size());
        float max_ov = ov_percentage_ * max_points;

        // if (next_level_nodes_[k]->overlap_ > 0)
        //{
        // compute FSV fraction
        VisibilityReasoning<PointT> vr(cam_);
        vr.setThresholdTSS(0.01);

        float fsv_ij =
            vr.computeFSV(target_, next_level_node->src_keypoints_);  //, next_level_nodes_[k]->accum_transform_);
        int used_ij = vr.getFSVUsedPoints();
        float fsv_ji = vr.computeFSV(input_, tgt_keypoints, next_level_node->accum_transform_.inverse());
        int used_ji = vr.getFSVUsedPoints();
        // float fsv_ij, fsv_ji;
        // int used_ij, used_ji;

        next_level_node->overlap_ = std::max(used_ij, used_ji);

        // std::cout << next_level_nodes_[k]->overlap_ << " " << max_ov << std::endl;

        if (next_level_node->overlap_ > 0) {
          float fsv_fraction = std::max(fsv_ij, fsv_ji);
          float ov = std::min(static_cast<float>(next_level_node->overlap_), max_ov) / static_cast<float>(max_points);
          next_level_node->fsv_fraction_ = fsv_fraction;
          next_level_node->reg_error_ = (1.f - (std::max(fsv_fraction, fsv_cutoff))) * ov;
        } else {
          next_level_node->reg_error_ = -1.f;
        }
      }

      // another pass to evaluate normals and maybe color
      //#pragma omp parallel for num_threads(8)
      for (size_t k = 0; k < next_level_nodes_.size(); k++) {
        std::vector<float> values_ij = evaluateHypotheses(target_, next_level_nodes_[k]->src_keypoints_,
                                                          target_normals_, next_level_nodes_[k]->normal_src_keypoints_);
        // next_level_nodes_[k]->accum_transform_);

        // std::cout << "ij:" << values_ij[0] << std::endl;

        std::vector<float> values_ji = evaluateHypotheses(input_, tgt_keypoints, input_normals_, normal_tgt_keypoints,
                                                          next_level_nodes_[k]->accum_transform_.inverse());

        // std::cout << values_ji[0] << std::endl;
        next_level_nodes_[k]->reg_error_ *= std::max(values_ij[0], values_ji[0]);
      }
    }

    std::sort(next_level_nodes_.begin(), next_level_nodes_.end(),
              boost::bind(&ICPNode<PointT>::reg_error_, _1) > boost::bind(&ICPNode<PointT>::reg_error_, _2));

    typename std::vector<std::shared_ptr<ICPNode<PointT>>>
        nodes_survived_after_filtering;  // contains nodes that need to be processed at a certain ICP iteration
    for (size_t k = 0; k < next_level_nodes_.size(); k++) {
      bool f = filterHypothesesByPose(next_level_nodes_[k], nodes_survived_after_filtering, 0.02f);
      if (!f)
        nodes_survived_after_filtering.push_back(next_level_nodes_[k]);
    }

    nodes_survived_after_filtering.resize(std::min(nodes_survived_after_filtering.size(), max_keep_));
    alive_nodes_ = nodes_survived_after_filtering;

    nr_iterations++;
    converged = nr_iterations > max_iterations_ || alive_nodes_.empty();
  } while (!converged);

  result_.resize(alive_nodes_.size());
  for (size_t i = 0; i < alive_nodes_.size(); i++) {
    result_[i] = Result(alive_nodes_[i]->reg_error_, alive_nodes_[i]->accum_transform_);
  }

#ifdef FAAT_PCL_FAST_ICP_VIS_FINAL
  survived_vis.removeAllPointClouds();
  survived_vis.removeAllShapes();
  visualizeICPNodes(alive_nodes_, survived_vis, "final");
#endif
}

template class V4R_EXPORTS ICPNode<pcl::PointXYZRGB>;
template class V4R_EXPORTS FastIterativeClosestPointWithGC<pcl::PointXYZRGB>;

}  // namespace v4r