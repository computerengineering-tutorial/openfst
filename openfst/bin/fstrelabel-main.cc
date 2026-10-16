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
// Relabels input or output space of an FST.

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/lib/util.h"
#include "openfst/script/fst-class.h"
#include "openfst/script/relabel.h"

ABSL_DECLARE_FLAG(std::string, isymbols);
ABSL_DECLARE_FLAG(std::string, osymbols);
ABSL_DECLARE_FLAG(std::string, relabel_isymbols);
ABSL_DECLARE_FLAG(std::string, relabel_osymbols);
ABSL_DECLARE_FLAG(std::string, relabel_ipairs);
ABSL_DECLARE_FLAG(std::string, relabel_opairs);
ABSL_DECLARE_FLAG(std::string, unknown_isymbol);
ABSL_DECLARE_FLAG(std::string, unknown_osymbol);

int fstrelabel_main(int argc, char** argv) {
  namespace s = fst::script;
  using fst::ReadLabelPairs;
  using fst::SymbolTable;
  using fst::script::MutableFstClass;

  std::string usage =
      "Relabels the input and/or the output labels of the FST.\n\n"
      "  Usage: ";
  usage += argv[0];
  usage += " [in.fst [out.fst]]\n";
  usage += "\n Using SymbolTables flags:\n";
  usage += "  --relabel_isymbols isyms.map\n";
  usage += "  --relabel_osymbols osyms.map\n";
  usage += "\n Using numeric labels flags:\n";
  usage += "  --relabel_ipairs ipairs.txt\n";
  usage += "  --relabel_opairs opairs.txt\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  const std::string in_name =
      (argc > 1 && strcmp(argv[1], "-") != 0) ? argv[1] : "";
  const std::string out_name =
      (argc > 2 && strcmp(argv[2], "-") != 0) ? argv[2] : "";

  std::unique_ptr<MutableFstClass> fst(MutableFstClass::Read(in_name, true));
  if (!fst) return 1;

  if (!absl::GetFlag(FLAGS_relabel_isymbols).empty() ||
      !absl::GetFlag(FLAGS_relabel_osymbols).empty()) {
    bool attach_new_isymbols = (fst->InputSymbols() != nullptr);
    std::unique_ptr<const SymbolTable> old_isymbols(
        absl::GetFlag(FLAGS_isymbols).empty()
            ? nullptr
            : SymbolTable::ReadText(absl::GetFlag(FLAGS_isymbols),
                                    absl::GetFlag(FLAGS_fst_field_separator)));
    const std::unique_ptr<const SymbolTable> relabel_isymbols(
        absl::GetFlag(FLAGS_relabel_isymbols).empty()
            ? nullptr
            : SymbolTable::ReadText(absl::GetFlag(FLAGS_relabel_isymbols),
                                    absl::GetFlag(FLAGS_fst_field_separator)));
    bool attach_new_osymbols = (fst->OutputSymbols() != nullptr);
    std::unique_ptr<const SymbolTable> old_osymbols(
        absl::GetFlag(FLAGS_osymbols).empty()
            ? nullptr
            : SymbolTable::ReadText(absl::GetFlag(FLAGS_osymbols),
                                    absl::GetFlag(FLAGS_fst_field_separator)));
    const std::unique_ptr<const SymbolTable> relabel_osymbols(
        absl::GetFlag(FLAGS_relabel_osymbols).empty()
            ? nullptr
            : SymbolTable::ReadText(absl::GetFlag(FLAGS_relabel_osymbols),
                                    absl::GetFlag(FLAGS_fst_field_separator)));
    s::Relabel(fst.get(),
               old_isymbols ? old_isymbols.get() : fst->InputSymbols(),
               relabel_isymbols.get(), absl::GetFlag(FLAGS_unknown_isymbol),
               attach_new_isymbols,
               old_osymbols ? old_osymbols.get() : fst->OutputSymbols(),
               relabel_osymbols.get(), absl::GetFlag(FLAGS_unknown_osymbol),
               attach_new_osymbols);
  } else {
    // Reads in relabeling pairs.
    std::vector<std::pair<int64_t, int64_t>> ipairs;
    if (!absl::GetFlag(FLAGS_relabel_ipairs).empty() &&
        !ReadLabelPairs(absl::GetFlag(FLAGS_relabel_ipairs), &ipairs)) {
      return 1;
    }
    std::vector<std::pair<int64_t, int64_t>> opairs;
    if (!absl::GetFlag(FLAGS_relabel_opairs).empty() &&
        !ReadLabelPairs(absl::GetFlag(FLAGS_relabel_opairs), &opairs)) {
      return 1;
    }
    s::Relabel(fst.get(), ipairs, opairs);
  }

  return !fst->Write(out_name);
}
