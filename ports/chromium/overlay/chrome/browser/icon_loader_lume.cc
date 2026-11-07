// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/icon_loader.h"

#include "base/functional/bind.h"
#include "base/nix/mime_util_xdg.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

IconLoader::IconGroup IconLoader::GroupForFilepath(const base::FilePath &file_path)
{
    return base::nix::GetFileMimeType(file_path);
}

scoped_refptr<base::TaskRunner> IconLoader::GetReadIconTaskRunner()
{
    return content::GetUIThreadTaskRunner({});
}

void IconLoader::ReadIcon()
{
    target_task_runner_->PostTask(FROM_HERE,
                                  base::BindOnce(std::move(callback_), gfx::Image(), group_));
    delete this;
}
