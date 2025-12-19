#ifndef STATS_H
#define STATS_H
#include "etl/vector.h"
#include "etl/variance.h"
#include "etl/mean.h"
#include "config.h"

/*
  Note that integers should be scaled up by at least 1e4
  before filtering, as to avoid possible rounding errors from integer arithmetic.
*/

template <typename T> bool sigma_filter(const T data, const double mean_value, const double variance){
  return sq(data - mean_value) < sq(outlier_discard_tolerance)*variance;
}

template <typename T> double filter_vector(etl::ivector<T> const & data){
  etl::mean<T,double> mean(data.begin(), data.end());
  double d_mean = mean;
  int8_t oldsize = data.size();
  int8_t newsize = data.size();
  if (remove_outliers){
    etl::variance<etl::variance_type::Sample, T, double> variance(data.begin(), data.end());
    double d_variance = variance;
    if (d_variance > 0){
      for (int i = 0; i < oldsize; i++){
        if (!sigma_filter(data[i], d_mean, d_variance)){
          d_mean -= ((double) data[i])/oldsize;
          newsize--;  
        }
      }
      d_mean = newsize > 0 ? d_mean * ((double) oldsize)/newsize : 0;
    }
  }
  return (T) d_mean;
}
#endif