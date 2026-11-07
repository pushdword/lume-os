// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FEATURE_ENGAGEMENT_PUBLIC_LUME_PERFORMANCE_FEATURES_H_
#define COMPONENTS_FEATURE_ENGAGEMENT_PUBLIC_LUME_PERFORMANCE_FEATURES_H_

#include "base/component_export.h"
#include "base/feature.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_LUME)
namespace feature_engagement
{

COMPONENT_EXPORT(FEATURE_ENGAGEMENT_FEATURE_CONSTANTS)
BASE_DECLARE_FEATURE(kIPHPerformanceInterventionDialogFeature);

} // namespace feature_engagement
#endif

#endif // COMPONENTS_FEATURE_ENGAGEMENT_PUBLIC_LUME_PERFORMANCE_FEATURES_H_
