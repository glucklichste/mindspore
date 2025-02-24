/**
 * Copyright 2021 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MINDSPORE_LITE_TOOLS_CONVERTER_QUANTIZER_BIASCORRECTION_H
#define MINDSPORE_LITE_TOOLS_CONVERTER_QUANTIZER_BIASCORRECTION_H
#include <memory>
#include <map>
#include <string>
#include <vector>
#include "base/base.h"
#include "ir/anf.h"
#include "tools/converter/quantizer/calibrator.h"

namespace mindspore::lite::quant {
enum OperationType {
  STORE,
  FETCH,
};

class BiasCorrectionStrategy {
 public:
  BiasCorrectionStrategy(const converter::Flags &flags, const std::shared_ptr<Calibrator> &calibrator,
                         session::LiteSession *fp32_session, Model *fp32_model, int activation_q_min,
                         int activation_q_max)
      : flags_(flags),
        calibrator_(calibrator),
        fp32_session_(fp32_session),
        fp32_model_(fp32_model),
        activation_q_min_(activation_q_min),
        activation_q_max_(activation_q_max) {}
  ~BiasCorrectionStrategy() {
    if (int8_session_ != nullptr) {
      delete int8_session_;
    }
    if (int8_model_ != nullptr) {
      delete int8_model_;
    }
  }
  int DoCPUBiasCorrection(const FuncGraphPtr &quant_func_graph);

 private:
  int CreateQuantModel(const FuncGraphPtr &quant_func_graph);
  int DoCNodeBiasCorrection(const FuncGraphPtr &quant_func_graph, const CNodePtr &cnode);
  int Int8Inference();
  bool OpInputDataHandle(OperationType type, const string &op_name, std::vector<float> *data);
  bool OpOutputChMeanDataHandle(OperationType type, const string &op_name, std::vector<float> *data);
  KernelCallBack GetBeforeCallBack(bool int8_op);
  KernelCallBack GetFloatBeforeCallBack();
  KernelCallBack GetInt8BeforeCallBack();
  KernelCallBack GetAfterCallBack(bool int8_op);
  KernelCallBack GetInt8AfterCallBack();
  KernelCallBack GetFloatAfterCallBack();

  template <typename T>
  int CalculatePerChannelMeans(const T *tensor_data, size_t elem_count, std::vector<int> shapes,
                               std::vector<float> *per_channel_mean) {
    //  const auto *tensor_data = static_cast<const float *>(tensor->data());
    //  size_t elem_count = tensor->ElementsNum();
    MS_CHECK_GT(elem_count, 0, false);
    //  auto shapes = tensor->shape();
    if (shapes.size() != DIMENSION_4D) {
      MS_LOG(ERROR) << "unexpected shape size: " << shapes.size();
      return RET_ERROR;
    }
    // suppose the activation format: NHWC
    auto channels = shapes[FOURTH_INPUT];
    if (channels == 0) {
      MS_LOG(ERROR) << "unexpected channels: 0";
      return RET_ERROR;
    }
    per_channel_mean->resize(channels);
    auto bucket_size = elem_count / channels;
    for (int i = 0; i < channels; i++) {
      float sum = 0;
      for (size_t j = 0; j < bucket_size; j++) {
        auto index = j * channels + i;
        if (index >= elem_count) {
          MS_LOG(ERROR) << "over flow!";
          return RET_ERROR;
        }
        sum += tensor_data[index];
      }
      MS_CHECK_GT(bucket_size, 0, false);
      sum = sum / bucket_size;
      per_channel_mean->at(i) = sum;
    }
    return RET_OK;
  }

 private:
  converter::Flags flags_;
  std::shared_ptr<Calibrator> calibrator_{nullptr};
  session::LiteSession *fp32_session_{nullptr};
  Model *fp32_model_{nullptr};
  int activation_q_min_{INT8_MIN};
  int activation_q_max_{INT8_MAX};

  session::LiteSession *int8_session_{nullptr};
  Model *int8_model_{nullptr};

  std::map<std::string, std::vector<float>> fp32_op_input_map_;           // concurrency
  std::map<std::string, std::vector<float>> fp32_op_output_ch_mean_map_;  // concurrency
  std::map<std::string, std::vector<float>> op_bias_diff_sum_map_;        // Record the sum of diffs in tensor
  std::mutex mutex_op_input_;
  std::mutex mutex_op_output_;
};
}  // namespace mindspore::lite::quant
#endif  // MINDSPORE_LITE_TOOLS_CONVERTER_QUANTIZER_BIASCORRECTION_H
