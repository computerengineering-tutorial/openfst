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
// Creates binary FSTs from simple text format used by AT&T.

#include <cstring>
#include <iostream>
#include <istream>
#include <memory>
#include <string>

#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/log/flags.h"
#include "absl/log/log.h"
#include "absl/flags/usage.h"
#include "openfst/compat/init.h"
#include "openfst/lib/file-util.h"
#include "openfst/lib/symbol-table.h"
#include "openfst/script/compile.h"

ABSL_DECLARE_FLAG(bool, acceptor);
ABSL_DECLARE_FLAG(std::string, arc_type);
ABSL_DECLARE_FLAG(std::string, fst_type);
ABSL_DECLARE_FLAG(std::string, isymbols);
ABSL_DECLARE_FLAG(std::string, osymbols);
ABSL_DECLARE_FLAG(std::string, ssymbols);
ABSL_DECLARE_FLAG(bool, keep_isymbols);
ABSL_DECLARE_FLAG(bool, keep_osymbols);
ABSL_DECLARE_FLAG(bool, keep_state_numbering);

int fstcompile_main(int argc, char** argv) {
  namespace s = fst::script;
  using fst::SymbolTable;

  std::string usage =
      "Creates binary FSTs from simple text format.\n\n  Usage: ";
  usage += argv[0];
  usage += " [text.fst [binary.fst]]\n";

  fst::InitOpenFst(usage.c_str(), &argc, &argv, true);
  if (argc > 3) {
    LOG(INFO) << absl::ProgramUsageMessage();
    return 1;
  }

  std::string source = "standard input";
  file::FileInStream fstrm;
  if (argc > 1 && strcmp(argv[1], "-") != 0) {
    fstrm.open(argv[1]);
    if (!fstrm) {
      LOG(ERROR) << argv[0] << ": Open failed, file = " << argv[1];
      return 1;
    }
    source = argv[1];
  }
  std::istream& istrm = fstrm.is_open() ? fstrm : std::cin;

  std::unique_ptr<const SymbolTable> isyms;
  if (!absl::GetFlag(FLAGS_isymbols).empty()) {
    isyms.reset(
        SymbolTable::ReadText(absl::GetFlag(FLAGS_isymbols),
                              absl::GetFlag(FLAGS_fst_field_separator)));
    if (!isyms) return 1;
  }

  std::unique_ptr<const SymbolTable> osyms;
  if (!absl::GetFlag(FLAGS_osymbols).empty()) {
    osyms.reset(
        SymbolTable::ReadText(absl::GetFlag(FLAGS_osymbols),
                              absl::GetFlag(FLAGS_fst_field_separator)));
    if (!osyms) return 1;
  }

  std::unique_ptr<const SymbolTable> ssyms;
  if (!absl::GetFlag(FLAGS_ssymbols).empty()) {
    ssyms.reset(
        SymbolTable::ReadText(absl::GetFlag(FLAGS_ssymbols),
                              absl::GetFlag(FLAGS_fst_field_separator)));
    if (!ssyms) return 1;
  }

  const std::string dest = argc > 2 && strcmp(argv[2], "-") != 0 ? argv[2] : "";

  s::Compile(istrm, source, dest, absl::GetFlag(FLAGS_fst_type),
             absl::GetFlag(FLAGS_arc_type), isyms.get(), osyms.get(),
             ssyms.get(), absl::GetFlag(FLAGS_acceptor),
             absl::GetFlag(FLAGS_keep_isymbols),
             absl::GetFlag(FLAGS_keep_osymbols),
             absl::GetFlag(FLAGS_keep_state_numbering));

  return 0;
}
