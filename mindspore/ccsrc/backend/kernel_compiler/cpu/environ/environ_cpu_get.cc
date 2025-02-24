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

#include "backend/kernel_compiler/cpu/environ/environ_cpu_get.h"
#include "backend/kernel_compiler/environ_manager.h"
#include "backend/kernel_compiler/common_utils.h"

namespace mindspore {
namespace kernel {
const std::vector<size_t> &EnvironGetCPUKernel::GetInputSizeList() const { return input_size_list_; }

const std::vector<size_t> &EnvironGetCPUKernel::GetOutputSizeList() const { return output_size_list_; }

const std::vector<size_t> &EnvironGetCPUKernel::GetWorkspaceSizeList() const { return workspace_size_list_; }

void EnvironGetCPUKernel::InitKernel(const CNodePtr &node) {
  MS_EXCEPTION_IF_NULL(node);
  if (!EnvironMgr::GetInstance().CheckEnvInput(node)) {
    MS_LOG(EXCEPTION) << "The input checks invalid, kernel: " << node->fullname_with_scope();
  }

  value_type_attr_ = TypeId(AnfAlgo::GetNodeAttr<int>(node, kEnvValueTypeAttr));
  handle_size_ = sizeof(int64_t);
  key_size_ = sizeof(int64_t);

  auto value_type = AnfAlgo::GetOutputDeviceDataType(node, 0);
  auto value_shapes = AnfAlgo::GetOutputDeviceShape(node, 0);
  auto default_value_type = AnfAlgo::GetInputDeviceDataType(node, 2);
  auto default_value_shapes = AnfAlgo::GetInputDeviceShape(node, 2);
  if ((value_type != default_value_type) || (value_shapes != default_value_shapes)) {
    MS_LOG(EXCEPTION) << "The env value checks invalid, kernel: " << node->fullname_with_scope();
  }
  value_size_ = GetTypeByte(TypeIdToType(value_type));
  for (auto &i : value_shapes) {
    value_size_ *= i;
  }

  input_size_list_.push_back(handle_size_);
  input_size_list_.push_back(key_size_);
  input_size_list_.push_back(value_size_);
  output_size_list_.push_back(value_size_);
}

bool EnvironGetCPUKernel::Launch(const std::vector<AddressPtr> &inputs, const std::vector<AddressPtr> &,
                                 const std::vector<AddressPtr> &outputs) {
  auto input_handle = GetDeviceAddress<int64_t>(inputs, 0);
  auto input_key = GetDeviceAddress<int64_t>(inputs, 1);
  auto input_default_value = GetDeviceAddress<void>(inputs, 2);
  auto output_value = GetDeviceAddress<int64_t>(outputs, 0);

  // Get host handle and host key.
  int64_t host_handle = input_handle[0];
  int64_t host_key = input_key[0];

  // Get env and value by handle and key.
  const auto &env = EnvironMgr::GetInstance().Get(host_handle);
  MS_EXCEPTION_IF_NULL(env);
  const auto &env_value = env->Get(host_key);
  // Default value.
  auto value = input_default_value;
  auto value_size = inputs[2]->size;
  auto value_type = value_type_attr_;
  if (env_value != nullptr) {
    value = env_value->addr_;
    value_size = env_value->size_;
    value_type = env_value->value_type_;
  } else {
    auto node = cnode_ptr_.lock();
    const std::string &prim_name = (node == nullptr) ? "" : AnfAlgo::GetCNodeName(node);
    MS_LOG(INFO) << "Use the default input value for kernel: " << prim_name << ", env handle: " << host_handle
                 << ", env key: " << host_key;
  }

  // Check the env value size and type. The value size may be aligned, so must be greater then value_size_.
  if ((value_size < value_size_) || (value_type != value_type_attr_)) {
    MS_LOG(ERROR) << "The env value checks invalid, value_size: " << value_size << ", value_size_: " << value_size_
                  << ", value_type: " << value_type << ", value_type_attr_: " << value_type_attr_;
    return false;
  }

  auto ret = memcpy_s(output_value, value_size_, value, value_size_);
  if (ret != 0) {
    MS_LOG(EXCEPTION) << "Output memcpy error: " << ret;
  }
  return true;
}
}  // namespace kernel
}  // namespace mindspore
