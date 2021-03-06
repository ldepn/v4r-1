#include <v4r/common/normal_estimator_integral_image.h>

#include <pcl/features/integral_image_normal.h>
#include <pcl/impl/instantiate.hpp>

#include <glog/logging.h>

namespace v4r {

template <typename PointT>
pcl::PointCloud<pcl::Normal>::Ptr NormalEstimatorIntegralImage<PointT>::compute() {
  CHECK(input_ && input_->isOrganized());
  normal_.reset(new pcl::PointCloud<pcl::Normal>);

  pcl::IntegralImageNormalEstimation<PointT, pcl::Normal> ne;
  ne.setNormalEstimationMethod(ne.COVARIANCE_MATRIX);
  ne.setMaxDepthChangeFactor(param_.max_depth_change_factor_);
  ne.setNormalSmoothingSize(param_.smoothing_size_);
  ne.setDepthDependentSmoothing(param_.use_depth_depended_smoothing_);
  ne.setInputCloud(input_);
  ne.compute(*normal_);

  for (pcl::Normal &n : normal_->points) {
    auto normal = n.getNormalVector3fMap();
    if (normal.dot(Eigen::Vector3f::UnitZ()) > 0)  // flip normal towards viewpoint
      normal = -normal;

    normal.normalize();
  }

  return normal_;
}

#define PCL_INSTANTIATE_NormalEstimatorIntegralImage(T) template class V4R_EXPORTS NormalEstimatorIntegralImage<T>;
PCL_INSTANTIATE(NormalEstimatorIntegralImage, PCL_XYZ_POINT_TYPES)
}  // namespace v4r
