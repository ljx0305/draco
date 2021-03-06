// Copyright 2016 The Draco Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "draco/javascript/emscripten/decoder_webidl_wrapper.h"

#include "draco/compression/decode.h"
#include "draco/mesh/mesh.h"
#include "draco/mesh/mesh_stripifier.h"

using draco::DecoderBuffer;
using draco::Mesh;
using draco::Metadata;
using draco::PointAttribute;
using draco::PointCloud;
using draco::Status;

MetadataQuerier::MetadataQuerier() {}

bool MetadataQuerier::HasIntEntry(const Metadata &metadata,
                                  const char *entry_name) const {
  int32_t value = 0;
  const std::string name(entry_name);
  if (!metadata.GetEntryInt(name, &value))
    return false;
  return true;
}

long MetadataQuerier::GetIntEntry(const Metadata &metadata,
                                  const char *entry_name) const {
  int32_t value = 0;
  const std::string name(entry_name);
  metadata.GetEntryInt(name, &value);
  return value;
}

bool MetadataQuerier::HasDoubleEntry(const Metadata &metadata,
                                     const char *entry_name) const {
  double value = 0;
  const std::string name(entry_name);
  if (!metadata.GetEntryDouble(name, &value))
    return false;
  return true;
}

double MetadataQuerier::GetDoubleEntry(const Metadata &metadata,
                                       const char *entry_name) const {
  double value = 0;
  const std::string name(entry_name);
  metadata.GetEntryDouble(name, &value);
  return value;
}

bool MetadataQuerier::HasStringEntry(const Metadata &metadata,
                                     const char *entry_name) const {
  std::string return_value;
  const std::string name(entry_name);
  if (!metadata.GetEntryString(name, &return_value))
    return false;
  return true;
}

const char *MetadataQuerier::GetStringEntry(const Metadata &metadata,
                                            const char *entry_name) const {
  std::string return_value;
  const std::string name(entry_name);
  metadata.GetEntryString(name, &return_value);
  const char *value = return_value.c_str();
  return value;
}

DracoFloat32Array::DracoFloat32Array() {}

float DracoFloat32Array::GetValue(int index) const { return values_[index]; }

bool DracoFloat32Array::SetValues(const float *values, int count) {
  if (values) {
    values_.assign(values, values + count);
  } else {
    values_.resize(count);
  }
  return true;
}

DracoInt32Array::DracoInt32Array() {}

int DracoInt32Array::GetValue(int index) const { return values_[index]; }

bool DracoInt32Array::SetValues(const int *values, int count) {
  values_.assign(values, values + count);
  return true;
}

Decoder::Decoder() {}

draco_EncodedGeometryType Decoder::GetEncodedGeometryType(
    DecoderBuffer *in_buffer) {
  return draco::Decoder::GetEncodedGeometryType(in_buffer).value();
}

const Status *Decoder::DecodeBufferToPointCloud(DecoderBuffer *in_buffer,
                                                PointCloud *out_point_cloud) {
  last_status_ = decoder_.DecodeBufferToGeometry(in_buffer, out_point_cloud);
  return &last_status_;
}

const Status *Decoder::DecodeBufferToMesh(DecoderBuffer *in_buffer,
                                          Mesh *out_mesh) {
  last_status_ = decoder_.DecodeBufferToGeometry(in_buffer, out_mesh);
  return &last_status_;
}

long Decoder::GetAttributeId(const PointCloud &pc,
                             draco_GeometryAttribute_Type type) const {
  return pc.GetNamedAttributeId(type);
}

const PointAttribute *Decoder::GetAttribute(const PointCloud &pc, long att_id) {
  return pc.attribute(att_id);
}

const PointAttribute *Decoder::GetAttributeByUniqueId(const PointCloud &pc,
                                                      long unique_id) {
  return pc.GetAttributeByUniqueId(unique_id);
}

long Decoder::GetAttributeIdByName(const PointCloud &pc,
                                   const char *attribute_name) {
  const std::string entry_value(attribute_name);
  return pc.GetAttributeIdByMetadataEntry("name", entry_value);
}

long Decoder::GetAttributeIdByMetadataEntry(const PointCloud &pc,
                                            const char *metadata_name,
                                            const char *metadata_value) {
  const std::string entry_name(metadata_name);
  const std::string entry_value(metadata_value);
  return pc.GetAttributeIdByMetadataEntry(entry_name, entry_value);
}

bool Decoder::GetFaceFromMesh(const Mesh &m,
                              draco::FaceIndex::ValueType face_id,
                              DracoInt32Array *out_values) {
  const Mesh::Face &face = m.face(draco::FaceIndex(face_id));
  return out_values->SetValues(reinterpret_cast<const int *>(face.data()),
                               face.size());
}

long Decoder::GetTriangleStripsFromMesh(const Mesh &m,
                                        DracoInt32Array *strip_values) {
  draco::MeshStripifier stripifier;
  std::vector<int32_t> strip_indices;
  if (!stripifier.GenerateTriangleStripsWithDegenerateTriangles(
          m, std::back_inserter(strip_indices))) {
    return 0;
  }
  if (!strip_values->SetValues(strip_indices.data(), strip_indices.size()))
    return 0;
  return stripifier.num_strips();
}

bool Decoder::GetAttributeFloat(const PointAttribute &pa,
                                draco::AttributeValueIndex::ValueType val_index,
                                DracoFloat32Array *out_values) {
  const int kMaxAttributeFloatValues = 4;
  const int components = pa.num_components();
  float values[kMaxAttributeFloatValues] = {-2.0, -2.0, -2.0, -2.0};
  if (!pa.ConvertValue<float>(draco::AttributeValueIndex(val_index), values))
    return false;
  return out_values->SetValues(values, components);
}

bool Decoder::GetAttributeFloatForAllPoints(const PointCloud &pc,
                                            const PointAttribute &pa,
                                            DracoFloat32Array *out_values) {
  const int components = pa.num_components();
  const int num_points = pc.num_points();
  const int num_entries = num_points * components;
  const int kMaxAttributeFloatValues = 4;
  float values[kMaxAttributeFloatValues] = {-2.0, -2.0, -2.0, -2.0};
  int entry_id = 0;

  out_values->SetValues(nullptr, num_entries);
  for (draco::PointIndex i(0); i < num_points; ++i) {
    const draco::AttributeValueIndex val_index = pa.mapped_index(i);
    if (!pa.ConvertValue<float>(val_index, values))
      return false;
    for (int j = 0; j < components; ++j) {
      out_values->SetValue(entry_id++, values[j]);
    }
  }
  return true;
}

bool Decoder::GetAttributeInt32ForAllPoints(const PointCloud &pc,
                                            const PointAttribute &pa,
                                            DracoInt32Array *out_values) {
  const int components = pa.num_components();
  const int num_points = pc.num_points();
  const int num_entries = num_points * components;
  const int kMaxAttributeIntValues = 4;
  int values[kMaxAttributeIntValues] = {0, 0, 0, 0};
  int entry_id = 0;

  out_values->SetValues(nullptr, num_entries);
  for (draco::PointIndex i(0); i < num_points; ++i) {
    const draco::AttributeValueIndex val_index = pa.mapped_index(i);
    if (!pa.ConvertValue<int>(val_index, values))
      return false;
    for (int j = 0; j < components; ++j) {
      out_values->SetValue(entry_id++, values[j]);
    }
  }
  return true;
}

bool Decoder::GetAttributeIntForAllPoints(const PointCloud &pc,
                                          const PointAttribute &pa,
                                          DracoInt32Array *out_values) {
  return GetAttributeInt32ForAllPoints(pc, pa, out_values);
}

void Decoder::SkipAttributeTransform(draco_GeometryAttribute_Type att_type) {
  decoder_.SetSkipAttributeTransform(att_type);
}

const Metadata *Decoder::GetMetadata(const PointCloud &pc) const {
  if (!pc.GetMetadata()) {
    return nullptr;
  }
  return pc.GetMetadata();
}

const Metadata *Decoder::GetAttributeMetadata(const PointCloud &pc,
                                              long att_id) const {
  return pc.GetAttributeMetadataByAttributeId(att_id);
}
