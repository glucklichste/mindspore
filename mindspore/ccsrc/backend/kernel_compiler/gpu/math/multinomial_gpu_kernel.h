/**
 * Copyright 2020-2021 Huawei Technologies Co., Ltd
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

#ifndef MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_GPU_MATH_MULTINOMIAL_GPU_KERNEL_H_
#define MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_GPU_MATH_MULTINOMIAL_GPU_KERNEL_H_

#include <curand_kernel.h>
#include <cuda_runtime_api.h>
#include <vector>
#include <string>
#include <map>
#include <random>
#include "runtime/device/gpu/gpu_memory_allocator.h"
#include "backend/kernel_compiler/gpu/gpu_kernel.h"
#include "backend/kernel_compiler/gpu/gpu_kernel_factory.h"
#include "backend/kernel_compiler/gpu/cuda_impl/multinomial_impl.cuh"
#include "backend/kernel_compiler/gpu/cuda_impl/cumsum_impl.cuh"

namespace mindspore {
namespace kernel {
template <typename T>
class MultinomialGpuKernel : public GpuKernel {
 public:
  MultinomialGpuKernel()
      : input_size_0_(0),
        output_size_(0),
        distributions_(0),
        categories_{0},
        seed_(0),
        seed2_(0),
        is_null_input_(false),
        rand_state_init_(false),
        rand_state_(nullptr) {}
  ~MultinomialGpuKernel() override = default;

  const std::vector<size_t> &GetInputSizeList() const override { return input_size_list_; }
  const std::vector<size_t> &GetOutputSizeList() const override { return output_size_list_; }
  const std::vector<size_t> &GetWorkspaceSizeList() const override { return workspace_size_list_; }

  bool Launch(const std::vector<AddressPtr> &inputs, const std::vector<AddressPtr> &workspace,
              const std::vector<AddressPtr> &outputs, void *stream_ptr) override {
    if (is_null_input_) {
      return true;
    }
    int *output_addr = GetDeviceAddress<int>(outputs, 0);
    T *probs_addr = GetDeviceAddress<T>(inputs, 0);
    int64_t *num_sample_addr = GetDeviceAddress<int64_t>(inputs, 1);
    if (distributions_ == 0) {
      MS_LOG(ERROR) << "Divide by zero. the distributions_ is 0.";
      return false;
    }

    auto stream = reinterpret_cast<cudaStream_t>(stream_ptr);
    if (!rand_state_init_) {
      int rng_seed = 0;
      std::random_device rd;
      if (seed2_ != 0) {
        rng_seed = seed2_;
      } else if (seed_ != 0) {
        rng_seed = seed_;
      } else {
        rng_seed = static_cast<int>(rd());
      }
      InitRandState(rng_seed, distributions_, rand_state_, stream);
      rand_state_init_ = true;
    }

    Multinomial(distributions_, categories_, probs_addr, rand_state_, num_sample_addr, output_addr, stream);
    return true;
  }

  bool Init(const CNodePtr &kernel_node) override {
    std::string kernel_name = AnfAlgo::GetCNodeName(kernel_node);
    size_t input_num = AnfAlgo::GetInputTensorNum(kernel_node);
    if (input_num != 2) {
      MS_LOG(ERROR) << "Input number is " << input_num << ", but multinomial needs 2 input.";
      return false;
    }
    size_t output_num = AnfAlgo::GetOutputTensorNum(kernel_node);
    if (output_num != 1) {
      MS_LOG(ERROR) << "Output number is " << output_num << ", but multinomial needs 1 output.";
      return false;
    }
    auto input_shape_0 = AnfAlgo::GetPrevNodeOutputInferShape(kernel_node, 0);
    auto output_shape = AnfAlgo::GetOutputInferShape(kernel_node, 0);
    is_null_input_ = CHECK_NULL_INPUT(input_shape_0) || CHECK_NULL_INPUT(output_shape);
    if (is_null_input_) {
      MS_LOG(WARNING) << "For 'MultinomialGpuKernel', input or output is null";
      InitSizeLists();
      return true;
    }
    if (input_shape_0.size() == 1) {
      distributions_ = 1;
      categories_ = input_shape_0[0];
    } else {
      distributions_ = input_shape_0[0];
      categories_ = input_shape_0[1];
    }
    input_size_0_ = sizeof(T);
    for (size_t i = 0; i < input_shape_0.size(); i++) {
      input_size_0_ *= input_shape_0[i];
    }

    output_size_ = sizeof(int);
    for (size_t i = 0; i < output_shape.size(); i++) {
      output_size_ *= output_shape[i];
    }

    auto prim = AnfAlgo::GetCNodePrimitive(kernel_node);
    MS_EXCEPTION_IF_NULL(prim);
    seed_ = static_cast<int>(GetValue<int64_t>(prim->GetAttr("seed")));
    seed2_ = static_cast<int>(GetValue<int64_t>(prim->GetAttr("seed2")));
    auto &allocator = device::gpu::GPUMemoryAllocator::GetInstance();
    rand_state_ = static_cast<curandState *>(allocator.AllocTensorMem(sizeof(curandState) * distributions_));

    InitSizeLists();
    return true;
  }

 protected:
  void InitSizeLists() override {
    input_size_list_.push_back(input_size_0_);
    input_size_list_.push_back(sizeof(int64_t));
    output_size_list_.push_back(output_size_);
  }

 private:
  size_t input_size_0_;
  size_t output_size_;
  size_t distributions_;
  size_t categories_;
  int seed_;
  int seed2_;
  bool is_null_input_;
  bool rand_state_init_;
  curandState *rand_state_;
  std::vector<size_t> input_size_list_;
  std::vector<size_t> output_size_list_;
  std::vector<size_t> workspace_size_list_;
};
}  // namespace kernel
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_GPU_MATH_MULTINOMIAL_GPU_KERNEL_H_
