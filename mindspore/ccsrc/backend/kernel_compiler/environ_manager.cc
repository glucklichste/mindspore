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

#include "backend/kernel_compiler/environ_manager.h"
#include "utils/ms_utils.h"
#include "utils/log_adapter.h"

namespace mindspore {
namespace kernel {
constexpr auto kScalarTensorShapeDim = 1;
constexpr auto kScalarTensorShapeSize = 1;

int64_t EnvironMgr::Create() {
  mutex.lock();
  if (env_handles_count_ >= INT64_MAX) {
    MS_LOG(EXCEPTION) << " The handles number is out of range: " << env_handles_count_;
  }
  int64_t ret_handle = ++env_handles_count_;
  auto env = std::make_shared<Environ>(ret_handle);
  MS_EXCEPTION_IF_NULL(env);
  envs_[ret_handle] = env;
  mutex.unlock();

  return ret_handle;
}

EnvironPtr EnvironMgr::Get(int64_t handle) {
  mutex.lock_shared();
  if (envs_.count(handle) > 0) {
    return envs_[handle];
  } else {
    return nullptr;
  }
  mutex.unlock();
}

void EnvironMgr::Clear() {
  for (auto &env : envs_) {
    MS_EXCEPTION_IF_NULL(env.second);
    env.second->Clear();
  }

  envs_.clear();
}

bool EnvironMgr::CheckEnvInput(const CNodePtr &kernel_node) {
  MS_EXCEPTION_IF_NULL(kernel_node);
  // Check the value type attr.
  auto value_type_attr = TypeId(AnfAlgo::GetNodeAttr<int>(kernel_node, kEnvValueTypeAttr));
  if ((value_type_attr != kObjectTypeTensorType) && (value_type_attr != kObjectTypeEnvType)) {
    MS_LOG(ERROR) << "The value type is not supported: " << value_type_attr
                  << ", kernel: " << kernel_node->fullname_with_scope();
    return false;
  }

  // Check the input handle.
  auto handle_type = AnfAlgo::GetInputDeviceDataType(kernel_node, 0);
  auto handle_shapes = AnfAlgo::GetInputDeviceShape(kernel_node, 0);
  if (!IsScalarTensor(handle_type, handle_shapes)) {
    MS_LOG(ERROR) << "The input handle checks invalid, kernel: " << kernel_node->fullname_with_scope();
    return false;
  }

  // Check the input key.
  auto key_type = AnfAlgo::GetInputDeviceDataType(kernel_node, 1);
  auto key_shapes = AnfAlgo::GetInputDeviceShape(kernel_node, 1);
  if (!IsScalarTensor(key_type, key_shapes)) {
    MS_LOG(ERROR) << "The input key checks invalid, kernel: " << kernel_node->fullname_with_scope();
    return false;
  }

  // Check the input value.
  auto value_type = AnfAlgo::GetInputDeviceDataType(kernel_node, 2);
  auto value_shapes = AnfAlgo::GetInputDeviceShape(kernel_node, 2);
  if ((value_type_attr == kObjectTypeEnvType) && (!IsScalarTensor(value_type, value_shapes))) {
    MS_LOG(ERROR) << "The input value checks invalid, kernel: " << kernel_node->fullname_with_scope();
    return false;
  }

  return true;
}

bool EnvironMgr::IsScalarTensor(TypeId type, std::vector<size_t> shape) {
  if (type == kObjectTypeTensorType) {
    MS_LOG(ERROR) << "The type is invalid: " << type;
    return false;
  }

  if (shape.size() != kScalarTensorShapeDim) {
    MS_LOG(ERROR) << "The shape size is invalid: " << shape.size();
    return false;
  }

  if (shape[0] != kScalarTensorShapeSize) {
    MS_LOG(ERROR) << "The shape is invalid: " << shape[0];
    return false;
  }

  return true;
}
}  // namespace kernel
}  // namespace mindspore
