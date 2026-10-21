//===--- Targets.cpp - Implement target feature support -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements construction of a TargetInfo object from a
// target triple.
//
//===----------------------------------------------------------------------===//

#include "Targets.h"

#include "Targets/AArch64.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticFrontend.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/TargetParser/Triple.h"

using namespace clang;

namespace clang {
namespace targets {
//===----------------------------------------------------------------------===//
//  Common code shared among targets.
//===----------------------------------------------------------------------===//

/// DefineStd - Define a macro name and standard variants.  For example if
/// MacroName is "unix", then this will define "__unix", "__unix__", and "unix"
/// when in GNU mode.
void DefineStd(MacroBuilder &Builder, StringRef MacroName,
               const LangOptions &Opts) {
  assert(MacroName[0] != '_' && "Identifier should be in the user's namespace");

  // If in GNU mode (e.g. -std=gnu99 but not -std=c99) define the raw identifier
  // in the user's namespace.
  if (Opts.GNUMode)
    Builder.defineMacro(MacroName);

  // Define __unix.
  Builder.defineMacro("__" + MacroName);

  // Define __unix__.
  Builder.defineMacro("__" + MacroName + "__");
}

void defineCPUMacros(MacroBuilder &Builder, StringRef CPUName, bool Tuning) {
  Builder.defineMacro("__" + CPUName);
  Builder.defineMacro("__" + CPUName + "__");
  if (Tuning)
    Builder.defineMacro("__tune_" + CPUName + "__");
}

void addCygMingDefines(const LangOptions &Opts, MacroBuilder &Builder) {
  // Mingw and cygwin define __declspec(a) to __attribute__((a)).  Clang
  // supports __declspec natively under -fdeclspec (also enabled with
  // -fms-extensions), but we define a no-op __declspec macro anyway for
  // pre-processor compatibility.
  if (Opts.DeclSpecKeyword)
    Builder.defineMacro("__declspec", "__declspec");
  else
    Builder.defineMacro("__declspec(a)", "__attribute__((a))");

  if (!Opts.MicrosoftExt) {
    // Provide macros for all the calling convention keywords.  Provide both
    // single and double underscore prefixed variants.  These are available on
    // x64 as well as x86, even though they have no effect.
    const char *CCs[] = {"cdecl", "stdcall", "fastcall", "thiscall", "pascal"};
    for (const char *CC : CCs) {
      std::string GCCSpelling = "__attribute__((__";
      GCCSpelling += CC;
      GCCSpelling += "__))";
      Builder.defineMacro(Twine("_") + CC, GCCSpelling);
      Builder.defineMacro(Twine("__") + CC, GCCSpelling);
    }
  }
}

//===----------------------------------------------------------------------===//
// Driver code
//===----------------------------------------------------------------------===//

std::unique_ptr<TargetInfo> AllocateTarget(const llvm::Triple &Triple,
                                           const TargetOptions &Opts) {
  if (Triple.getArch() == llvm::Triple::aarch64)
    return std::make_unique<AArch64leTargetInfo>(Triple, Opts);
  return nullptr;
}
} // namespace targets
} // namespace clang

using namespace clang::targets;
/// CreateTargetInfo - Return the target info object for the specified target
/// options.
TargetInfo *TargetInfo::CreateTargetInfo(DiagnosticsEngine &Diags,
                                         TargetOptions &OptsRef) {
  TargetOptions *Opts = &OptsRef;

  llvm::Triple Triple(llvm::Triple::normalize(Opts->Triple));

  // Construct the target
  std::unique_ptr<TargetInfo> Target = AllocateTarget(Triple, *Opts);
  if (!Target) {
    Diags.Report(diag::err_target_unknown_triple) << Triple.str();
    return nullptr;
  }
  Target->TargetOpts = Opts;

  // Set the target CPU if specified.
  if (!Opts->CPU.empty() && !Target->setCPU(Opts->CPU)) {
    Diags.Report(diag::err_target_unknown_cpu) << Opts->CPU;
    SmallVector<StringRef, 32> ValidList;
    Target->fillValidCPUList(ValidList);
    if (!ValidList.empty())
      Diags.Report(diag::note_valid_options) << llvm::join(ValidList, ", ");
    return nullptr;
  }

  // Check the TuneCPU name if specified.
  if (!Opts->TuneCPU.empty() &&
      !Target->isValidTuneCPUName(Opts->TuneCPU)) {
    Diags.Report(diag::err_target_unknown_cpu) << Opts->TuneCPU;
    SmallVector<StringRef, 32> ValidList;
    Target->fillValidTuneCPUList(ValidList);
    if (!ValidList.empty())
      Diags.Report(diag::note_valid_options) << llvm::join(ValidList, ", ");
    return nullptr;
  }

  // Set the target ABI if specified.
  if (!Opts->ABI.empty() && !Target->setABI(Opts->ABI)) {
    Diags.Report(diag::err_target_unknown_abi) << Opts->ABI;
    return nullptr;
  }

  // Set the fp math unit.
  if (!Opts->FPMath.empty() && !Target->setFPMath(Opts->FPMath)) {
    Diags.Report(diag::err_target_unknown_fpmath) << Opts->FPMath;
    return nullptr;
  }

  // Compute the default target features, we need the target to handle this
  // because features may have dependencies on one another.
  llvm::erase_if(Opts->FeaturesAsWritten, [&](StringRef Name) {
    if (Target->isReadOnlyFeature(Name.substr(1))) {
      Diags.Report(diag::warn_fe_backend_readonly_feature_flag) << Name;
      return true;
    }
    return false;
  });
  if (!Target->initFeatureMap(Opts->FeatureMap, Diags, Opts->CPU,
                              Opts->FeaturesAsWritten))
    return nullptr;

  // Add the features to the compile options.
  Opts->Features.clear();
  for (const auto &F : Opts->FeatureMap)
    Opts->Features.push_back((F.getValue() ? "+" : "-") + F.getKey().str());
  // Sort here, so we handle the features in a predictable order. (This matters
  // when we're dealing with features that overlap.)
  llvm::sort(Opts->Features);

  if (!Target->handleTargetFeatures(Opts->Features, Diags))
    return nullptr;

  Target->setSupportedOpenCLOpts();
  Target->setCommandLineOpenCLOpts();
  Target->setDependentOpenCLOpts();
  Target->setMaxAtomicWidth();

  if (!Opts->DarwinTargetVariantTriple.empty())
    Target->DarwinTargetVariantTriple =
        llvm::Triple(Opts->DarwinTargetVariantTriple);

  if (!Target->validateTarget(Diags))
    return nullptr;

  Target->CheckFixedPointBits();

  return Target.release();
}
/// validateOpenCLTarget  - Check that OpenCL target has valid
/// options setting based on OpenCL version.
bool TargetInfo::validateOpenCLTarget(const LangOptions &Opts,
                                      DiagnosticsEngine &Diags) const {
  const llvm::StringMap<bool> &OpenCLFeaturesMap = getSupportedOpenCLOpts();

  auto diagnoseNotSupportedCore = [&](llvm::StringRef Name, auto... OptArgs) {
    if (OpenCLOptions::isOpenCLOptionCoreIn(Opts, OptArgs...) &&
        !hasFeatureEnabled(OpenCLFeaturesMap, Name))
      Diags.Report(diag::warn_opencl_unsupported_core_feature)
          << Name << Opts.OpenCLCPlusPlus
          << Opts.getOpenCLVersionTuple().getAsString();
  };
#define OPENCL_GENERIC_EXTENSION(Ext, ...)                                     \
  diagnoseNotSupportedCore(#Ext, __VA_ARGS__);
#include "clang/Basic/OpenCLExtensions.def"

  // Validate that feature macros are set properly for OpenCL C 3.0.
  // In other cases assume that target is always valid.
  if (Opts.getOpenCLCompatibleVersion() < 300)
    return true;

  return OpenCLOptions::diagnoseUnsupportedFeatureDependencies(*this, Diags) &&
         OpenCLOptions::diagnoseFeatureExtensionDifferences(*this, Diags);
}
