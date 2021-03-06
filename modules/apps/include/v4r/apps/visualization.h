#pragma once

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/search/kdtree.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <v4r/common/pcl_visualization_utils.h>
#include <v4r/core/macros.h>
#include <v4r/recognition/local_feature_matching.h>
#include <v4r/recognition/object_hypothesis.h>
#include <v4r/recognition/source.h>

namespace v4r {

enum class ObjRecoVisLayoutStyle {
  FULL,         // show all (input, processed, generated and verified hypotheses)
  SIMPLE,       // show only input and verified objects
  INTERMEDIATE  // show input, processed, and verified objects
};

std::istream &operator>>(std::istream &in, ObjRecoVisLayoutStyle &cm);
std::ostream &operator<<(std::ostream &out, const ObjRecoVisLayoutStyle &cm);

/**
 * @brief Visualization framework for object recognition
 * @author Thomas Faeulhammer
 * @date May 2016
 */
template <typename PointT>
class V4R_EXPORTS ObjectRecognitionVisualizer {
 private:
  enum class KP_VIS_STATE { NOTHING, OBJECT_SPECIFIC_COLOR, CORR_SPECIFIC_COLOR };

  typename pcl::PointCloud<PointT>::ConstPtr cloud_;            ///< input cloud
  typename pcl::PointCloud<PointT>::ConstPtr processed_cloud_;  ///< input cloud
  typename pcl::PointCloud<pcl::Normal>::ConstPtr normals_;     ///< input normals

  std::vector<ObjectHypothesesGroup> generated_object_hypotheses_;  ///< generated object hypotheses
  //    std::vector< ObjectHypothesesGroup<PointT> > generated_object_hypotheses_refined_;   ///< (ICP refined)
  //    generated object hypotheses
  //    std::vector< typename ObjectHypothesis<PointT>::Ptr > verified_object_hypotheses_; ///< verified object
  //    hypotheses
  mutable pcl::visualization::PCLVisualizer::Ptr vis_;
  mutable int vp1a_, vp2_, vp3_, vp1b_, vp2b_;
  mutable std::vector<std::pair<std::string, int>> coordinate_axis_ids_;  ///< variable which keeps track of coordinate
                                                                          ///< axis and in which viewpoint they are
                                                                          ///< visualized (needed to remove them
                                                                          ///< properly)

  typename Source<PointT>::ConstPtr m_db_;                                      ///< model data base
  std::map<std::string, typename LocalObjectModel::ConstPtr> model_keypoints_;  ///< pointer to local model database
                                                                                ///(optional: required if visualization
                                                                                /// of feature matching is desired)

  PCLVisualizationParams::ConstPtr vis_param_;  ///< visualization parameters
  ObjRecoVisLayoutStyle layout_;                ///< defines the layout for the output
  ///< "0": 3 rows with top: verified hyp; middle: generated and (refined) gener.; bottom: input and processed cloud.
  ///< "1": 2 rows with top: verified hyp; bottom: input cloud
  ///< "2": 2 rows with top: verified hyp; bottom: input and processed cloud

  void keyboardEventOccurred(const pcl::visualization::KeyboardEvent &event) const;
  void pointPickingEventOccured(const pcl::visualization::PointPickingEvent &event) const;

  void updateExtendedVisualization() const;

  int model_resolution_mm_;  ///< resolution of the visualized object model in mm

  mutable pcl::KdTreeFLANN<pcl::PointXYZ>::Ptr kdtree_;

  mutable KP_VIS_STATE visualization_status = KP_VIS_STATE::NOTHING;
  mutable bool scene_overlay_;  ///< defines whether or not scene should be overlaid onto visualization windows
  mutable bool visualize_correspondence_lines_;
  mutable bool visualize_co_axes_;

  /**
   * @brief The Line class is a utility class to visualize and toggle the correspondences between model and scene
   * keypoints
   */
  struct Line {
    PointT p_, q_;
    double r_, g_, b_;
    std::string id_;
    int viewport_;

    Line(const PointT &p, const PointT &q, double r, double g, double b, const std::string &id)
    : p_(p), q_(q), r_(r), g_(g), b_(b), id_(id) {}
  };
  mutable std::vector<Line> corrs_hypothesis_specific_color_;
  mutable std::vector<Line> corrs_corr_specific_color_;

  /**
   * @brief setupLayout sets up the layout of the visualization
   */
  void setupLayout() const;
  void cleanUp() const;

 public:
  ObjectRecognitionVisualizer(const PCLVisualizationParams::ConstPtr &vis_param,
                              ObjRecoVisLayoutStyle layout = ObjRecoVisLayoutStyle::FULL)
  : vis_param_(vis_param), layout_(layout), scene_overlay_(true), visualize_correspondence_lines_(false),
    visualize_co_axes_(true) {}

  ObjectRecognitionVisualizer(ObjRecoVisLayoutStyle layout = ObjRecoVisLayoutStyle::FULL)
  : layout_(layout), scene_overlay_(true), visualize_correspondence_lines_(false), visualize_co_axes_(true) {
    PCLVisualizationParams::Ptr vis_param(new PCLVisualizationParams());
    vis_param_ = vis_param;
  }

  /**
   * @brief visualize
   */
  void visualize() const;

  /**
   * @brief setCloud
   * @param[in] cloud input cloud
   */
  void setCloud(const typename pcl::PointCloud<PointT>::ConstPtr cloud) {
    typename pcl::PointCloud<PointT>::Ptr vis_cloud(new pcl::PointCloud<PointT>(*cloud));
    vis_cloud->sensor_orientation_ = Eigen::Quaternionf::Identity();
    vis_cloud->sensor_origin_ = Eigen::Vector4f::Zero(4);
    cloud_ = vis_cloud;
  }

  /**
   * @brief setProcessedCloud
   * @param[in] cloud processed cloud
   */
  void setProcessedCloud(const typename pcl::PointCloud<PointT>::ConstPtr cloud) {
    typename pcl::PointCloud<PointT>::Ptr vis_cloud(new pcl::PointCloud<PointT>(*cloud));
    vis_cloud->sensor_orientation_ = Eigen::Quaternionf::Identity();
    vis_cloud->sensor_origin_ = Eigen::Vector4f::Zero(4);
    processed_cloud_ = vis_cloud;
  }

  /**
   * @brief setNormals visualizes normals of input cloud
   * @param normal cloud
   */
  void setNormals(const pcl::PointCloud<pcl::Normal>::ConstPtr &normals) {
    normals_ = normals;
  }

  /**
   * @brief setGeneratedObjectHypotheses
   * @param[in] goh generated hypotheses
   */
  void setGeneratedObjectHypotheses(const std::vector<ObjectHypothesesGroup> &goh) {
    generated_object_hypotheses_ = goh;
  }

  //    /**
  //     * @brief setRefinedGeneratedObjectHypotheses
  //     * @param[in] goh (ICP refined) generated hypotheses
  //     */
  //    void
  //    setRefinedGeneratedObjectHypotheses ( const std::vector< ObjectHypothesesGroup<PointT> > &goh )
  //    {
  //        generated_object_hypotheses_refined_ = goh;
  //    }

  //    /**
  //     * @brief setVerifiedObjectHypotheses
  //     * @param[in] voh verified hypotheses
  //     */
  //    void
  //    setVerifiedObjectHypotheses ( const std::vector<typename ObjectHypothesis<PointT>::Ptr > &voh )
  //    {
  //        verified_object_hypotheses_ = voh;
  //    }

  /**
   * @brief setLocalModelDatabase this function allows to additionally show the keypoint correspondences between scene
   * and model
   * @param lomdb Local ModelDatabase
   */
  void setLocalModelDatabase(const std::map<std::string, typename LocalObjectModel::ConstPtr> &model_keypoints) {
    model_keypoints_ = model_keypoints;
  }

  /**
   * @brief setModelDatabase
   * @param m_db model database
   */
  void setModelDatabase(const typename Source<PointT>::ConstPtr &m_db) {
    m_db_ = m_db;
  }

  typedef std::shared_ptr<ObjectRecognitionVisualizer<PointT>> Ptr;
  typedef std::shared_ptr<ObjectRecognitionVisualizer<PointT> const> ConstPtr;
};
}  // namespace v4r
