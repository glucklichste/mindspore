/**
 * Copyright 2019-2021 Huawei Technologies Co., Ltd
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

#include <string>
#include <sstream>

#include "ir/dtype.h"
#include "utils/ms_utils.h"
#include "base/core_ops.h"
#include "abstract/param_validator.h"
#include "abstract/infer_functions.h"
#include "abstract/utils.h"
#include "utils/ms_context.h"
#include "utils/symbolic.h"
#include "utils/shape_utils.h"

namespace {
constexpr auto kRankSize = "rank_size";
}

namespace mindspore {
namespace abstract {
constexpr auto kCSRDenseShape = "dense_shape";
constexpr auto kCSRAxis = "axis";
constexpr auto kCSRAvgRows = "csr_avg_rows";
AbstractBasePtr InferImplIdentity(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                  const AbstractBasePtrList &args_spec_list) {
  // An object of a subclass of AbstractBase
  CheckArgsSize(primitive->name(), args_spec_list, 1);
  return args_spec_list[0];
}

AbstractBasePtr InferImplEnvGetItem(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                    const AbstractBasePtrList &args_spec_list) {
  MS_EXCEPTION_IF_NULL(primitive);
  // args: Three objects of a subclass of AbstractBase, env, key, dflt(default).
  CheckArgsSize(primitive->name(), args_spec_list, 3);
  auto key = args_spec_list[1];
  auto dflt = args_spec_list[2];
  TypePtr type = key->GetTypeTrack();
  MS_EXCEPTION_IF_NULL(type);
  if (type->type_id() != kObjectTypeSymbolicKeyType) {
    MS_LOG(EXCEPTION) << "EnvGetItem evaluator args[1] should be a SymbolicKeyInstance but: " << key->ToString();
  }

  auto context = MsContext::GetInstance();
  MS_EXCEPTION_IF_NULL(context);
  bool enable_sparse = context->get_param<bool>(MS_CTX_ENABLE_SPARSE);
  if (enable_sparse && dflt->isa<AbstractTensor>()) {
    auto dflt_tensor = dflt->cast<AbstractTensorPtr>();
    return std::make_shared<AbstractUndetermined>(dflt_tensor->element()->Clone(), dflt_tensor->shape()->Clone());
  }

  if (!key->GetValueTrack()->isa<SymbolicKeyInstance>()) {
    return dflt;
  }
  ValuePtr key_value_ptr = key->GetValueTrack();
  MS_EXCEPTION_IF_NULL(key_value_ptr);
  auto key_value_track = key_value_ptr->cast<SymbolicKeyInstancePtr>();
  auto expected = key_value_track->abstract();
  MS_EXCEPTION_IF_NULL(expected);
  (void)expected->Join(dflt);
  return expected;
}

AbstractBasePtr InferImplEnvSetItem(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                    const AbstractBasePtrList &args_spec_list) {
  // args: Three objects of a subclass of AbstractBase, env, key, dflt(default).
  CheckArgsSize(primitive->name(), args_spec_list, 3);

  auto key = args_spec_list[1];
  ValuePtr key_value_ptr = key->GetValueTrack();
  MS_EXCEPTION_IF_NULL(key_value_ptr);
  auto key_value_track = key_value_ptr->cast<SymbolicKeyInstancePtr>();
  if (key_value_track == nullptr) {
    MS_LOG(EXCEPTION) << "EnvGetItem evaluator args[1] expected should be able to cast to SymbolicKeyInstancePtrbut: "
                      << key_value_ptr->ToString();
  }
  auto expected = key_value_track->abstract();
  MS_EXCEPTION_IF_NULL(expected);
  return std::make_shared<AbstractScalar>(kAnyValue, std::make_shared<EnvType>());
}

AbstractBasePtr InferImplEnvAdd(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                const AbstractBasePtrList &args_spec_list) {
  // args: Three objects of a subclass of AbstractBase, env, key, dflt(default).
  CheckArgsSize(primitive->name(), args_spec_list, 2);
  return std::make_shared<AbstractScalar>(kAnyValue, std::make_shared<EnvType>());
}

AbstractBasePtr InferImplMakeRefKey(const AnalysisEnginePtr &, const PrimitivePtr &prim, const AbstractBasePtrList &) {
  ValuePtr name_value = prim->GetAttr("tag");
  MS_EXCEPTION_IF_NULL(name_value);
  auto name = name_value->cast<StringImmPtr>();
  if (name == nullptr) {
    MS_LOG(EXCEPTION) << "MakeRefKey attr tag should be a String " << name_value->ToString() << ".";
  }
  auto refkey = std::make_shared<RefKey>(name->value());
  if (refkey == nullptr) {
    MS_LOG(EXCEPTION) << "MakeRefKey std::make_shared<RefKey> failed";
  }
  return refkey->ToAbstract();
}

AbstractBasePtr InferImplMakeRef(const AnalysisEnginePtr &, const PrimitivePtr &,
                                 const AbstractBasePtrList &args_spec_list) {
  // arguments: key, value, target type(None if no target type)
  if (args_spec_list.size() != 3) {
    MS_LOG(EXCEPTION) << "make_ref evaluator requires 3 parameters, while the input size is " << args_spec_list.size()
                      << ".";
  }
  auto tensor = args_spec_list[1]->cast<abstract::AbstractTensorPtr>();
  return std::make_shared<AbstractRef>(args_spec_list[0], tensor);
}

AbstractBasePtr InferImplGetRefKey(const AnalysisEnginePtr &, const PrimitivePtr &,
                                   const AbstractBasePtrList &args_spec_list) {
  // arguments: value
  if (args_spec_list.size() != 1) {
    MS_LOG(EXCEPTION) << "get_ref_key requires 1 parameters, while the input size is " << args_spec_list.size() << ".";
  }
  TypePtr type = args_spec_list[0]->GetTypeTrack();
  if (type->type_id() != kObjectTypeRef) {
    MS_LOG(EXCEPTION) << "First input of get_ref_key should be a Ref but a " << type->ToString();
  }
  auto abs_ref = args_spec_list[0]->cast<AbstractRefPtr>();
  MS_EXCEPTION_IF_NULL(abs_ref);
  return abs_ref->ref();
}

AbstractBasePtr InferImplGetRefValue(const AnalysisEnginePtr &, const PrimitivePtr &,
                                     const AbstractBasePtrList &args_spec_list) {
  // arguments: value
  if (args_spec_list.size() != 1) {
    MS_LOG(EXCEPTION) << "get_ref_value requires 1 parameters, while the input size is " << args_spec_list.size()
                      << ".";
  }
  TypePtr type = args_spec_list[0]->GetTypeTrack();
  if (type->type_id() != kObjectTypeRef) {
    return args_spec_list[0];
  }
  auto abs_ref = args_spec_list[0]->cast<AbstractRefPtr>();
  MS_EXCEPTION_IF_NULL(abs_ref);
  return abs_ref->ref();
}

AbstractBasePtr InferImplStateSetItem(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                      const AbstractBasePtrList &args_spec_list) {
  // args: Two objects of a subclass of AbstractBase, key and value.
  CheckArgsSize(primitive->name(), args_spec_list, 2);

  TypePtr type = args_spec_list[0]->GetTypeTrack();
  MS_EXCEPTION_IF_NULL(type);
  if (type->type_id() != kObjectTypeRefKey && type->type_id() != kObjectTypeSymbolicKeyType) {
    MS_LOG(EXCEPTION) << "First input of StateSetItem should be a RefKey or SymbolicKeyType but a " << type->ToString();
  }
  return std::make_shared<AbstractScalar>(kAnyValue, kBool);
}

AbstractBasePtr InferImplDepend(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                const AbstractBasePtrList &args_spec_list) {
  CheckArgsSize(primitive->name(), args_spec_list, 2);

  // If the dependent has a value, just return depended node.
  // If depended node is not Any, the dependent maybe eliminated.
  auto dependant_abstract = args_spec_list[1];
  auto dependant_value = dependant_abstract->BuildValue();
  MS_EXCEPTION_IF_NULL(dependant_value);
  if (dependant_value != kAnyValue) {
    return args_spec_list[0];
  }

  auto depends = args_spec_list[0]->Broaden();  // Avoid eliminating the dependent node.
  if (!MsContext::GetInstance()->get_param<bool>(MS_CTX_GRAD_FOR_SCALAR)) {
    // For scalar, need to set value to kAnyValue, because broaden scalar will not change the value.
    if (depends->isa<AbstractScalar>()) {
      depends->set_value(kAnyValue);
    }
  }
  return depends;
}

AbstractBasePtr InferImplUpdateState(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                     const AbstractBasePtrList &args_spec_list) {
  if (args_spec_list.empty()) {
    MS_LOG(EXCEPTION) << primitive->name() << " input args size should be at least 1, but got 0";
  }
  MS_EXCEPTION_IF_NULL(args_spec_list[0]);
  return args_spec_list[0]->Broaden();
}

AbstractBasePtr InferImplMakeRowTensor(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                       const AbstractBasePtrList &args_spec_list) {
  // Inputs: two tensors and a tuple.
  const std::string op_name = primitive->name();
  constexpr size_t size_expected = 3;
  CheckArgsSize(op_name, args_spec_list, size_expected);
  auto indices = CheckArg<AbstractTensor>(op_name, args_spec_list, 0);
  auto values = CheckArg<AbstractTensor>(op_name, args_spec_list, 1);
  auto dense_shape = CheckArg<AbstractTuple>(op_name, args_spec_list, 2);

  auto indices_dtype = indices->element()->BuildType();
  if (!indices_dtype->isa<Int>()) {
    MS_EXCEPTION(TypeError) << "The dtype of indices must be a Int, but got " << indices_dtype->ToString();
  }
  auto indices_shp = indices->shape()->shape();
  if (indices_shp.size() != 1) {
    MS_EXCEPTION(TypeError) << "Indices must be a 1 dimension tensor, but got a " << indices_shp.size()
                            << " dimension tensor";
  }
  auto values_shp = values->shape()->shape();
  if (indices_shp[0] != values_shp[0]) {
    MS_EXCEPTION(TypeError) << "The first dimension of indices must be the same with the first dimension of values "
                            << values_shp[0] << ", but got " << indices_shp[0];
  }

  for (const auto &elem_type : dense_shape->ElementsType()) {
    if (!elem_type->isa<Int>()) {
      MS_EXCEPTION(TypeError) << "The element type of dense_shape must be Int, but got " << elem_type->ToString();
    }
  }
  auto dense_shape_value = dense_shape->BuildValue();
  MS_EXCEPTION_IF_NULL(dense_shape_value);
  auto dense_shape_valuetuple = dense_shape_value->cast<ValueTuplePtr>();
  MS_EXCEPTION_IF_NULL(dense_shape_valuetuple);
  auto shp = dense_shape_valuetuple->value();
  ShapeVector dense_shape_vec;
  (void)std::transform(std::begin(shp), std::end(shp), std::back_inserter(dense_shape_vec),
                       [](const ValuePtr &e) -> int64_t {
                         auto elem = GetValue<int64_t>(e);
                         return elem;
                       });
  if (dense_shape_vec.size() != values_shp.size()) {
    MS_EXCEPTION(TypeError) << "The size of dense_shape must be the same with the dimension of values "
                            << values_shp.size() << ", but got " << dense_shape_valuetuple->size();
  }
  for (size_t i = 0; i < dense_shape_vec.size(); i++) {
    if (dense_shape_vec[i] < 0) {
      MS_EXCEPTION(TypeError) << "The " << i << "th element of dense_shape must be positive, but got "
                              << dense_shape_vec[i];
    }
    // The 0th mode might be less or exceed dense_shape[0] due to duplicated selection
    if (i != 0 && dense_shape_vec[i] != values_shp[i]) {
      MS_EXCEPTION(TypeError) << "The " << i << "th element of dense_shape must be same with the " << i
                              << "th dimension of values " << values_shp[i] << ", but got " << dense_shape_vec[i];
    }
  }
  auto ret = std::make_shared<AbstractRowTensor>(values->element()->BuildType(), dense_shape_vec);
  ret->set_indices(indices);
  ret->set_values(values);
  ret->set_dense_shape(dense_shape);
  return ret;
}

AbstractBasePtr InferImplRowTensorGetValues(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                            const AbstractBasePtrList &args_spec_list) {
  // Inputs: two tensors and a tuple.
  const std::string op_name = primitive->name();
  CheckArgsSize(op_name, args_spec_list, 1);
  auto row_tensor = CheckArg<AbstractRowTensor>(op_name, args_spec_list, 0);
  MS_EXCEPTION_IF_NULL(row_tensor->values());
  return row_tensor->values();
}

AbstractBasePtr InferImplRowTensorGetIndices(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                             const AbstractBasePtrList &args_spec_list) {
  // Inputs: two tensors and a tuple.
  const std::string op_name = primitive->name();
  CheckArgsSize(op_name, args_spec_list, 1);
  auto row_tensor = CheckArg<AbstractRowTensor>(op_name, args_spec_list, 0);
  MS_EXCEPTION_IF_NULL(row_tensor->indices());
  return row_tensor->indices();
}

AbstractBasePtr InferImplRowTensorGetDenseShape(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                                const AbstractBasePtrList &args_spec_list) {
  // Inputs: two tensors and a tuple.
  const std::string op_name = primitive->name();
  CheckArgsSize(op_name, args_spec_list, 1);
  auto row_tensor = CheckArg<AbstractRowTensor>(op_name, args_spec_list, 0);
  MS_EXCEPTION_IF_NULL(row_tensor->dense_shape());
  return row_tensor->dense_shape();
}

AbstractBasePtr InferImplRowTensorAdd(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                      const AbstractBasePtrList &args_spec_list) {
  // Inputs: row tensor and tensor.
  const std::string op_name = primitive->name();
  constexpr size_t args_size = 2;
  CheckArgsSize(op_name, args_spec_list, args_size);
  auto row_tensor = CheckArg<AbstractRowTensor>(op_name, args_spec_list, 0);
  auto tensor = CheckArg<AbstractTensor>(op_name, args_spec_list, 1);
  MS_EXCEPTION_IF_NULL(row_tensor->dense_shape());
  MS_EXCEPTION_IF_NULL(tensor->shape());
  return args_spec_list[0];
}

AbstractBasePtr InferImplMakeSparseTensor(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                          const AbstractBasePtrList &args_spec_list) {
  // Inputs: two tensors and a tuple.
  constexpr auto kMakeSparseInputNum = 3;
  const std::string op_name = primitive->name();
  CheckArgsSize(op_name, args_spec_list, kMakeSparseInputNum);
  auto indices = CheckArg<AbstractTensor>(op_name, args_spec_list, 0);
  auto values = CheckArg<AbstractTensor>(op_name, args_spec_list, 1);
  auto dense_shape = CheckArg<AbstractTuple>(op_name, args_spec_list, 2);

  auto indices_dtype = indices->element()->BuildType();
  if (!indices_dtype->isa<Int>()) {
    MS_EXCEPTION(TypeError) << "The dtype of indices must be a Int, but got " << indices_dtype->ToString();
  }
  auto indices_shp = indices->shape()->shape();
  if (indices_shp.size() != 2) {
    MS_EXCEPTION(TypeError) << "Indices must be a 2 dimension tensor, but got a " << indices_shp.size()
                            << " dimension tensor";
  }
  auto values_shp = values->shape()->shape();
  if (values_shp.size() != 1) {
    MS_EXCEPTION(TypeError) << "Values must be a 1 dimension tensor, but got a " << values_shp.size()
                            << " dimension tensor";
  }
  if (indices_shp[0] != values_shp[0]) {
    MS_EXCEPTION(TypeError) << "The first dimension of indices must be the same with the first dimension of values "
                            << values_shp[0] << ", but got " << indices_shp[0];
  }

  for (const auto &elem_type : dense_shape->ElementsType()) {
    if (!elem_type->isa<Int>()) {
      MS_EXCEPTION(TypeError) << "The element type of dense_shape must be Int, but got " << elem_type->ToString();
    }
  }
  auto dense_shape_value = dense_shape->BuildValue()->cast<ValueTuplePtr>();
  MS_EXCEPTION_IF_NULL(dense_shape_value);
  auto shp = dense_shape_value->value();
  ShapeVector dense_shape_vec;
  (void)std::transform(std::begin(shp), std::end(shp), std::back_inserter(dense_shape_vec),
                       [](const ValuePtr &e) -> int64_t {
                         auto elem = GetValue<int64_t>(e);
                         return elem;
                       });
  if (LongToSize(indices_shp[1]) != dense_shape_vec.size()) {
    MS_EXCEPTION(TypeError) << "The size of dense_shape must be equal with the second dimension of indices "
                            << indices_shp[1] << ", but got " << dense_shape_vec.size();
  }
  for (auto dense_shape_elem : dense_shape_vec) {
    if (dense_shape_elem < 0) {
      MS_EXCEPTION(TypeError) << "The element of dense_shape must be positive, but got "
                              << dense_shape_value->ToString();
    }
  }
  auto ret = std::make_shared<AbstractSparseTensor>(values->element()->BuildType(), dense_shape_vec);
  ret->set_indices(indices);
  ret->set_values(values);
  ret->set_dense_shape(dense_shape);
  return ret;
}

AbstractBasePtr InferImplSparseTensorGetValues(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                               const AbstractBasePtrList &args_spec_list) {
  // Inputs: two tensors and a tuple.
  const std::string op_name = primitive->name();
  CheckArgsSize(op_name, args_spec_list, 1);
  auto sparse_tensor = CheckArg<AbstractSparseTensor>(op_name, args_spec_list, 0);
  MS_EXCEPTION_IF_NULL(sparse_tensor->values());
  return sparse_tensor->values();
}

AbstractBasePtr InferImplSparseTensorGetIndices(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                                const AbstractBasePtrList &args_spec_list) {
  // Inputs: two tensors and a tuple.
  const std::string op_name = primitive->name();
  CheckArgsSize(op_name, args_spec_list, 1);
  auto sparse_tensor = CheckArg<AbstractSparseTensor>(op_name, args_spec_list, 0);
  MS_EXCEPTION_IF_NULL(sparse_tensor->indices());
  return sparse_tensor->indices();
}

AbstractBasePtr InferImplSparseTensorGetDenseShape(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                                   const AbstractBasePtrList &args_spec_list) {
  // Inputs: two tensors and a tuple.
  const std::string op_name = primitive->name();
  CheckArgsSize(op_name, args_spec_list, 1);
  auto sparse_tensor = CheckArg<AbstractSparseTensor>(op_name, args_spec_list, 0);
  MS_EXCEPTION_IF_NULL(sparse_tensor->dense_shape());
  return sparse_tensor->dense_shape();
}

AbstractBasePtr InferImplCSRMul(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                const AbstractBasePtrList &args_spec_list) {
  // Inputs: a sparse tensor and a dense tensor.
  constexpr auto kCSRMulInputsNum = 2;
  constexpr auto kCSRMulShapeSize = 2;
  const std::string op_name = primitive->name();
  CheckArgsSize(op_name, args_spec_list, kCSRMulInputsNum);
  auto sparse = CheckArg<AbstractCSRTensor>(op_name, args_spec_list, 0);
  auto dense = CheckArg<AbstractTensor>(op_name, args_spec_list, 1);
  MS_EXCEPTION_IF_NULL(sparse);
  MS_EXCEPTION_IF_NULL(sparse->shape());
  MS_EXCEPTION_IF_NULL(sparse->values());
  MS_EXCEPTION_IF_NULL(sparse->indices());
  MS_EXCEPTION_IF_NULL(dense);

  auto sparse_shape = sparse->shape()->shape();
  auto dense_shape = dense->shape()->shape();
  if (sparse_shape.size() != kCSRMulShapeSize || dense_shape.size() != kCSRMulShapeSize) {
    MS_EXCEPTION(ValueError) << "Currently, only support " << kCSRMulShapeSize << "-D inputs!"
                             << "but sparse tensor has " << sparse_shape.size() << " dimensions, "
                             << "and dense tensor has " << dense_shape.size() << " dimensions, ";
  }
  auto ret = sparse->values()->Broaden();

  MS_EXCEPTION_IF_NULL(sparse->indices()->shape());
  auto nnz_vec = sparse->indices()->shape()->shape();
  int csr_avg_rows = nnz_vec[0] / dense_shape[0];
  primitive->set_attr(kCSRAvgRows, MakeValue(csr_avg_rows));
  primitive->set_attr(kCSRDenseShape, MakeValue(sparse_shape));

  return ret;
}

AbstractBasePtr InferImplCSRMV(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                               const AbstractBasePtrList &args_spec_list) {
  // Inputs: a sparse tensor and a dense tensor.
  constexpr auto kCSRMVInputsNum = 2;
  constexpr auto kCSRMVShapeSize = 2;
  const std::string op_name = primitive->name();
  CheckArgsSize(op_name, args_spec_list, kCSRMVInputsNum);
  auto sparse = CheckArg<AbstractCSRTensor>(op_name, args_spec_list, 0);
  auto dense = CheckArg<AbstractTensor>(op_name, args_spec_list, 1);
  MS_EXCEPTION_IF_NULL(sparse);
  MS_EXCEPTION_IF_NULL(sparse->shape());
  MS_EXCEPTION_IF_NULL(sparse->values());
  MS_EXCEPTION_IF_NULL(sparse->indices());
  MS_EXCEPTION_IF_NULL(dense);

  auto sparse_shape = sparse->shape()->shape();
  auto dense_shape = dense->shape()->shape();
  if (sparse_shape.size() != kCSRMVShapeSize || dense_shape.size() != kCSRMVShapeSize) {
    MS_EXCEPTION(ValueError) << "Currently, only support " << kCSRMVShapeSize << "-D inputs!"
                             << "but sparse tensor has " << sparse_shape.size() << " dimensions, "
                             << "and dense tensor has " << dense_shape.size() << " dimensions, ";
  }

  MS_EXCEPTION_IF_NULL(sparse->values()->element());
  ShapeVector out_shape = {sparse_shape[0], dense_shape[1]};
  auto ret = std::make_shared<AbstractTensor>(sparse->values()->element()->BuildType(), out_shape);

  MS_EXCEPTION_IF_NULL(sparse->indices()->shape());
  auto nnz_vec = sparse->indices()->shape()->shape();
  int csr_avg_rows = nnz_vec[0] / dense_shape[0];
  primitive->set_attr(kCSRAvgRows, MakeValue(csr_avg_rows));
  primitive->set_attr(kCSRDenseShape, MakeValue(sparse_shape));

  return ret;
}

AbstractBasePtr InferImplCSRReduceSum(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                      const AbstractBasePtrList &args_spec_list) {
  // Inputs: a sparse tensor and an axis.
  constexpr auto kCSRReduceSumInputsNum = 2;
  constexpr auto kCSRReduceSumShapeSize = 2;
  const std::string op_name = primitive->name();
  CheckArgsSize(op_name, args_spec_list, kCSRReduceSumInputsNum);
  auto sparse = CheckArg<AbstractCSRTensor>(op_name, args_spec_list, 0);
  auto axis = CheckArg<AbstractScalar>(op_name, args_spec_list, 1);
  MS_EXCEPTION_IF_NULL(sparse);
  MS_EXCEPTION_IF_NULL(sparse->shape());
  MS_EXCEPTION_IF_NULL(sparse->values());
  MS_EXCEPTION_IF_NULL(sparse->indices());
  MS_EXCEPTION_IF_NULL(axis);

  auto sparse_shape = sparse->shape()->shape();
  if (sparse_shape.size() != kCSRReduceSumShapeSize) {
    MS_EXCEPTION(ValueError) << "Currently, only support " << kCSRReduceSumShapeSize << "-D inputs!"
                             << "but sparse tensor has " << sparse_shape.size() << " dimensions.";
  }

  ShapeVector out_shape = sparse_shape;
  MS_EXCEPTION_IF_NULL(axis->BuildValue());
  if (axis->BuildValue()->isa<Int32Imm>() || axis->BuildValue()->isa<Int64Imm>()) {
    int64_t axis_value = GetValue<int64_t>(axis->BuildValue());
    int64_t dim = static_cast<int64_t>(sparse_shape.size());
    if (axis_value < -dim || axis_value >= dim) {
      MS_LOG(EXCEPTION) << "axis should be in [" << -dim << ", " << dim << "). But got axis = " << axis_value;
    }
    if (axis_value >= -dim && axis_value < 0) {
      axis_value += dim;
    }
    out_shape[LongToSize(axis_value)] = 1;
    primitive->set_attr(kCSRAxis, MakeValue(axis_value));
  } else {
    MS_EXCEPTION(ValueError) << "Currently, only support Integer axis.";
  }

  MS_EXCEPTION_IF_NULL(sparse->values()->element());
  auto ret = std::make_shared<AbstractTensor>(sparse->values()->element()->BuildType(), out_shape);

  MS_EXCEPTION_IF_NULL(sparse->indices()->shape());
  auto nnz_vec = sparse->indices()->shape()->shape();
  int csr_avg_rows = nnz_vec[0] / sparse_shape[0];
  primitive->set_attr(kCSRAvgRows, MakeValue(csr_avg_rows));
  primitive->set_attr(kCSRDenseShape, MakeValue(sparse_shape));

  return ret;
}

AbstractBasePtr InferImplMakeCSRTensor(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                       const AbstractBasePtrList &args_spec_list) {
  // Inputs: three tensors and a tuple.
  constexpr auto kMakeCSRInputNum = 4;
  const std::string op_name = primitive->name();
  CheckArgsSize(op_name, args_spec_list, kMakeCSRInputNum);
  auto indptr = CheckArg<AbstractTensor>(op_name, args_spec_list, 0);
  auto indices = CheckArg<AbstractTensor>(op_name, args_spec_list, 1);
  auto values = CheckArg<AbstractTensor>(op_name, args_spec_list, 2);
  auto shape = CheckArg<AbstractTuple>(op_name, args_spec_list, 3);

  auto indices_dtype = indices->element()->BuildType();
  if (!indices_dtype->isa<Int>()) {
    MS_EXCEPTION(TypeError) << "The dtype of indices must be a Int, but got " << indices_dtype->ToString();
  }
  auto indptr_shp = indptr->shape()->shape();
  if (indptr_shp.size() != 1) {
    MS_EXCEPTION(ValueError) << "Indptr must be a 1 dimension tensor, but got a " << indptr_shp.size()
                             << " dimension tensor";
  }
  auto indices_shp = indices->shape()->shape();
  if (indices_shp.size() != 1) {
    MS_EXCEPTION(ValueError) << "Indices must be a 1 dimension tensor, but got a " << indices_shp.size()
                             << " dimension tensor";
  }
  auto values_shp = values->shape()->shape();
  if (values_shp.size() != 1) {
    MS_EXCEPTION(ValueError) << "Values must be a 1 dimension tensor, but got a " << values_shp.size()
                             << " dimension tensor";
  }
  if (indices_shp[0] != values_shp[0]) {
    MS_EXCEPTION(ValueError) << "indices and values must have same size, but got: values length: " << values_shp[0]
                             << ", indices length " << indices_shp[0];
  }
  for (const auto &elem_type : shape->ElementsType()) {
    if (!elem_type->isa<Int>()) {
      MS_EXCEPTION(TypeError) << "The element type of shape must be Int, but got " << elem_type->ToString();
    }
  }
  auto shape_value = shape->BuildValue()->cast<ValueTuplePtr>();
  MS_EXCEPTION_IF_NULL(shape_value);
  auto shp = shape_value->value();
  ShapeVector shape_vec;
  (void)std::transform(std::begin(shp), std::end(shp), std::back_inserter(shape_vec), [](const ValuePtr &e) -> int64_t {
    auto elem = GetValue<int64_t>(e);
    return elem;
  });

  for (auto shape_elem : shape_vec) {
    if (shape_elem < 0) {
      MS_EXCEPTION(TypeError) << "The element of shape must be positive, but got " << shape_value->ToString();
    }
  }
  if (shape_vec[0] + 1 != indptr_shp[0]) {
    MS_EXCEPTION(ValueError) << "indptr must have length (1 + shape[0]), but got: " << indptr_shp[0];
  }
  auto ret = std::make_shared<AbstractCSRTensor>(values->element()->BuildType(), shape_vec);
  ret->set_indptr(indptr);
  ret->set_indices(indices);
  ret->set_values(values);
  ret->set_dense_shape(shape);
  return ret;
}

template <typename T>
std::shared_ptr<T> InferSparseAttr(const PrimitivePtr &primitive, const AbstractBasePtrList &args_spec_list) {
  const std::string op_name = primitive->name();
  CheckArgsSize(op_name, args_spec_list, 1);
  return CheckArg<T>(op_name, args_spec_list, 0);
}

AbstractBasePtr InferImplCSRTensorGetValues(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                            const AbstractBasePtrList &args_spec_list) {
  auto csr_tensor = InferSparseAttr<AbstractCSRTensor>(primitive, args_spec_list);
  MS_EXCEPTION_IF_NULL(csr_tensor->values());
  return csr_tensor->values();
}

AbstractBasePtr InferImplCSRTensorGetIndptr(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                            const AbstractBasePtrList &args_spec_list) {
  auto csr_tensor = InferSparseAttr<AbstractCSRTensor>(primitive, args_spec_list);
  MS_EXCEPTION_IF_NULL(csr_tensor->indptr());
  return csr_tensor->indptr();
}

AbstractBasePtr InferImplCSRTensorGetIndices(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                             const AbstractBasePtrList &args_spec_list) {
  auto csr_tensor = InferSparseAttr<AbstractCSRTensor>(primitive, args_spec_list);
  MS_EXCEPTION_IF_NULL(csr_tensor->indices());
  return csr_tensor->indices();
}

AbstractBasePtr InferImplCSRTensorGetDenseShape(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                                const AbstractBasePtrList &args_spec_list) {
  auto csr_tensor = InferSparseAttr<AbstractCSRTensor>(primitive, args_spec_list);
  MS_EXCEPTION_IF_NULL(csr_tensor->dense_shape());
  return csr_tensor->dense_shape();
}

AbstractBasePtr InferImplAllSwap(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                 const AbstractBasePtrList &args_spec_list) {
  const std::string op_name = primitive->name();
  CheckArgsSize(op_name, args_spec_list, 3);
  auto tensor_in = CheckArg<AbstractTensor>(op_name, args_spec_list, 0);
  MS_EXCEPTION_IF_NULL(tensor_in);
  MS_EXCEPTION_IF_NULL(tensor_in->shape());
  auto tensor_in_shape = tensor_in->shape()->shape();

  auto send_size = CheckArg<AbstractTensor>(op_name, args_spec_list, 1);
  MS_EXCEPTION_IF_NULL(send_size);
  auto recv_size = CheckArg<AbstractTensor>(op_name, args_spec_list, 2);
  MS_EXCEPTION_IF_NULL(recv_size);

  // Get the content of the recv size
  auto recv_size_value_ptr = recv_size->BuildValue();
  MS_EXCEPTION_IF_NULL(recv_size_value_ptr);
  auto recv_size_tensor = recv_size_value_ptr->cast<tensor::TensorPtr>();
  MS_EXCEPTION_IF_NULL(recv_size_tensor);
  auto data_pos = reinterpret_cast<int64_t *>(recv_size_tensor->data_c());
  MS_EXCEPTION_IF_NULL(data_pos);
  int64_t infer_max_size = 0;
  for (size_t i = 0; i < recv_size_tensor->DataSize(); ++i) {
    infer_max_size += *(data_pos + i);
  }

  ShapeVector tensor_out_shape = {Shape::SHP_ANY, tensor_in_shape[1]};
  ShapeVector min_shape = {1, tensor_in_shape[1]};

  ShapeVector max_shape = {infer_max_size / tensor_in_shape[1], tensor_in_shape[1]};

  auto tensor_out = std::make_shared<AbstractTensor>(tensor_in->element(),
                                                     std::make_shared<Shape>(tensor_out_shape, min_shape, max_shape));

  AbstractTensorPtr ret = std::make_shared<AbstractTensor>(
    tensor_out->element(), std::make_shared<Shape>(tensor_out_shape, min_shape, max_shape));
  return ret;
}

AbstractBasePtr InferImplAllReduce(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                   const AbstractBasePtrList &args_spec_list) {
  const std::string op_name = primitive->name();
  CheckArgsSize(op_name, args_spec_list, 1);
  auto x = CheckArg<AbstractTensor>(op_name, args_spec_list, 0);
  MS_EXCEPTION_IF_NULL(x);
  MS_EXCEPTION_IF_NULL(x->shape());
  return std::make_shared<AbstractTensor>(x->element(), std::make_shared<Shape>(x->shape()->shape()));
}

AbstractBasePtr InferImplBroadcast(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                   const AbstractBasePtrList &args_spec_list) {
  const std::string op_name = primitive->name();
  CheckArgsSize(op_name, args_spec_list, 1);
  auto x = CheckArg<AbstractTensor>(op_name, args_spec_list, 0);
  MS_EXCEPTION_IF_NULL(x);
  MS_EXCEPTION_IF_NULL(x->shape());
  return std::make_shared<AbstractTensor>(x->element(), std::make_shared<Shape>(x->shape()->shape()));
}

AbstractBasePtr InferImplAllGather(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                   const AbstractBasePtrList &args_spec_list) {
  const std::string op_name = primitive->name();
  CheckArgsSize(op_name, args_spec_list, 1);
  auto x = CheckArg<AbstractTensor>(op_name, args_spec_list, 0);
  MS_EXCEPTION_IF_NULL(x);
  MS_EXCEPTION_IF_NULL(x->shape());
  auto tmp_shape = x->shape()->shape();
  if (!primitive->HasAttr(kRankSize)) {
    MS_LOG(EXCEPTION) << "Primitive don't have rank_size attr";
  }
  auto rank_size = GetValue<int>(primitive->GetAttr(kRankSize));
  if (rank_size == 0) {
    MS_LOG(EXCEPTION) << "rank_size is 0";
  }
  if (tmp_shape.empty()) {
    MS_LOG(EXCEPTION) << "shape size is 0";
  }
  if (tmp_shape[0] > 0) {
    tmp_shape[0] = tmp_shape[0] * rank_size;
  }
  return std::make_shared<AbstractTensor>(x->element(), std::make_shared<Shape>(tmp_shape));
}

AbstractBasePtr InferImplReduceScatter(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                       const AbstractBasePtrList &args_spec_list) {
  const std::string op_name = primitive->name();
  CheckArgsSize(op_name, args_spec_list, 1);
  auto x = CheckArg<AbstractTensor>(op_name, args_spec_list, 0);
  MS_EXCEPTION_IF_NULL(x);
  MS_EXCEPTION_IF_NULL(x->shape());
  auto tmp_shape = x->shape()->shape();
  if (!primitive->HasAttr(kRankSize)) {
    MS_LOG(EXCEPTION) << "Primitive don't have rank_size attr";
  }
  auto rank_size = GetValue<int>(primitive->GetAttr(kRankSize));
  if (tmp_shape.empty()) {
    MS_LOG(EXCEPTION) << "shape size is 0";
  }
  tmp_shape[0] = LongMulWithOverflowCheck(tmp_shape[0], rank_size);
  return std::make_shared<AbstractTensor>(x->element(), std::make_shared<Shape>(tmp_shape));
}

AbstractBasePtr InferImplMemCpyAsync(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                     const AbstractBasePtrList &args_spec_list) {
  const std::string op_name = primitive->name();
  CheckArgsSize(op_name, args_spec_list, 1);
  auto x = CheckArg<AbstractTensor>(op_name, args_spec_list, 0);
  MS_EXCEPTION_IF_NULL(x);
  MS_EXCEPTION_IF_NULL(x->shape());
  return std::make_shared<AbstractTensor>(x->element(), std::make_shared<Shape>(x->shape()->shape()));
}

AbstractBasePtr InferImplCast(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                              const AbstractBasePtrList &args_spec_list) {
  const std::string op_name = primitive->name();
  // GPU has 2 inputs while tbe has 1 only. Skip CheckArgsSize.
  auto input_x = CheckArg<AbstractTensor>(op_name, args_spec_list, 0);
  MS_EXCEPTION_IF_NULL(input_x);
  auto attr = primitive->GetAttr("dst_type");
  if (attr == nullptr) {
    auto type_abs = CheckArg<AbstractType>(op_name, args_spec_list, 1);
    attr = type_abs->BuildValue();
    MS_EXCEPTION_IF_NULL(attr);
    primitive->set_attr("dst_type", attr);
  }
  auto input_type = attr->cast<TypePtr>();
  auto ret = std::make_shared<AbstractTensor>(input_type, input_x->shape());
  return ret;
}

AbstractBasePtr InferImplExpandDims(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                    const AbstractBasePtrList &args_spec_list) {
  constexpr auto kExpandDimsInputsNum = 2;
  const std::string op_name = primitive->name();
  auto x = CheckArg<AbstractTensor>(op_name, args_spec_list, 0);
  MS_EXCEPTION_IF_NULL(x);
  MS_EXCEPTION_IF_NULL(x->shape());

  std::vector<int64_t> shape;
  std::vector<int64_t> x_shape = x->shape()->shape();
  (void)shape.insert(shape.end(), x_shape.begin(), x_shape.end());
  int64_t value = 0;
  if (args_spec_list.size() == kExpandDimsInputsNum) {
    value = GetValue<int64_t>(args_spec_list[1]->BuildValue());
  } else if (args_spec_list.size() == 1) {
    auto axis = primitive->GetAttr("axis");
    value = GetValue<int64_t>(axis);
  } else {
    MS_LOG(EXCEPTION) << " The num of ExpandDims must be 1 or 2, but got " << args_spec_list.size();
  }

  if (value < -(SizeToInt(x_shape.size()) + 1) || value > SizeToInt(x_shape.size())) {
    MS_LOG(EXCEPTION) << " axis value should be in range [-input_x.dim-1,input_x.dim], but axis value is" << value
                      << " and input_x.dim is" << x_shape.size();
  }
  if (value < 0) {
    value = value + SizeToInt(x_shape.size()) + 1;
  }
  (void)shape.insert(shape.begin() + value, 1);

  auto ret = std::make_shared<AbstractTensor>(x->element(), std::make_shared<Shape>(shape));
  return ret;
}

AbstractBasePtr InferImplGpuConvertToDynamicShape(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                                  const AbstractBasePtrList &args_spec_list) {
  const std::string &op_name = primitive->name();
  CheckArgsSize(op_name, args_spec_list, 1);
  AbstractTensorPtr input = CheckArg<AbstractTensor>(op_name, args_spec_list, 0);

  ShapeVector input_shape = input->shape()->shape();
  int32_t input_rank = SizeToInt(input_shape.size());
  ShapeVector inferred_shape(input_rank, Shape::SHP_ANY);
  ShapeVector min_shape(input_rank, 1);
  ShapeVector max_shape = input_shape;

  ShapePtr shape = std::make_shared<Shape>(inferred_shape, min_shape, max_shape);
  return std::make_shared<AbstractTensor>(input->element(), shape);
}

AbstractBasePtr InferImplLoad(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                              const AbstractBasePtrList &args_spec_list) {
  // Inputs: Ref/Tensor, universal
  CheckArgsSize(primitive->name(), args_spec_list, 2);
  auto ref_abs = dyn_cast<abstract::AbstractRef>(args_spec_list[0]);
  if (ref_abs != nullptr) {
    // Return tensor value if input is Ref.
    return ref_abs->CloneAsTensor();
  }
  return args_spec_list[0]->Broaden();
}

AbstractBasePtr InferImplTransData(const AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                   const AbstractBasePtrList &args_spec_list) {
  // An object of a subclass of AbstractBase
  CheckArgsSize(primitive->name(), args_spec_list, 1);
  auto output = args_spec_list[0];
  MS_EXCEPTION_IF_NULL(output);
  return output;
}
}  // namespace abstract
}  // namespace mindspore
