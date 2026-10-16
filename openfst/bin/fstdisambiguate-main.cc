// Copyright 2026 The OpenFst Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Disambiguates an FST.

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "openfst/script/disambiguate.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/weight-class.h"

ABSL_DECLARE_FLAG(double, delta);
ABSL_DECLARE_FLAG(int64_t, nstate);
ABSL_DECLARE_FLAG(std::string, weight);
ABSL_DECLARE_FLAG(int64_t, subsequential_label);

int fstdisambiguate_main(int argc, char** argv) {
  namespace s = fst::script;
  using fst::script::FstClass;
  using fst::script::VectorFstClass;
  using fst::script::WeightClass;

  std::string usage = "Disambiguates an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  const std::string in_name =
      (argc > 1 && strcmp(argv[1], "-") != 0) ? argv[1] : "";
  const std::string out_name =
      (argc > 2 && strcmp(argv[2], "-") != 0) ? argv[2] : "";

  std::unique_ptr<FstClass> ifst(FstClass::Read(in_name));
  if (!ifst) return 1;

  VectorFstClass ofst(ifst->ArcType());

  const auto weight_threshold =
      absl::GetFlag(FLAGS_weight).empty()
          ? WeightClass::Zero(ifst->WeightType())
          : WeightClass(ifst->WeightType(), absl::GetFlag(FLAGS_weight));

  const s::DisambiguateOptions opts(
      absl::GetFlag(FLAGS_delta), weight_threshold, absl::GetFlag(FLAGS_nstate),
      absl::GetFlag(FLAGS_subsequential_label));

  s::Disambiguate(*ifst, &ofst, opts);

  return !ofst.Write(out_name);
}
