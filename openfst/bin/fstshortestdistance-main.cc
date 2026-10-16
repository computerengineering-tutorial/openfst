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
// Find shortest distances in an FST.

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "openfst/lib/queue.h"
#include "openfst/script/arcfilter-impl.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/getters.h"
#include "openfst/script/shortest-distance.h"
#include "openfst/script/text-io.h"
#include "openfst/script/weight-class.h"

ABSL_DECLARE_FLAG(bool, reverse);
ABSL_DECLARE_FLAG(double, delta);
ABSL_DECLARE_FLAG(int64_t, nstate);
ABSL_DECLARE_FLAG(std::string, queue_type);

int fstshortestdistance_main(int argc, char** argv) {
  namespace s = fst::script;
  using fst::AUTO_QUEUE;
  using fst::QueueType;
  using fst::script::FstClass;
  using fst::script::WeightClass;

  std::string usage = "Finds shortest distance(s) in an FST.\n\n  Usage: ";
  usage += argv[0];
  usage += " [in.fst [distance.txt]]\n";

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

  std::vector<WeightClass> distance;

  QueueType queue_type;
  if (!s::GetQueueType(absl::GetFlag(FLAGS_queue_type), &queue_type)) {
    LOG(ERROR) << argv[0] << ": Unknown or unsupported queue type: "
               << absl::GetFlag(FLAGS_queue_type);
    return 1;
  }

  if (absl::GetFlag(FLAGS_reverse) && queue_type != AUTO_QUEUE) {
    LOG(ERROR) << argv[0] << ": Can't use non-default queue with reverse";
    return 1;
  }

  if (absl::GetFlag(FLAGS_reverse)) {
    s::ShortestDistance(*ifst, &distance, absl::GetFlag(FLAGS_reverse),
                        absl::GetFlag(FLAGS_delta));
  } else {
    const s::ShortestDistanceOptions opts(queue_type, s::ArcFilterType::ANY,
                                          absl::GetFlag(FLAGS_nstate),
                                          absl::GetFlag(FLAGS_delta));
    s::ShortestDistance(*ifst, &distance, opts);
  }

  return !s::WritePotentials(out_name, distance);
}
