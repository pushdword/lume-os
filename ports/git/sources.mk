GIT_LIBRARY_SOURCES := \
    abspath.c add-interactive.c add-patch.c advice.c alias.c alloc.c apply.c archive-tar.c \
    archive-zip.c archive.c attr.c base85.c bisect.c blame.c blob.c bloom.c branch.c \
    bundle-uri.c bundle.c cache-tree.c cbtree.c chdir-notify.c checkout.c chunk-format.c \
    color.c column.c combine-diff.c commit-graph.c commit-reach.c commit.c common-exit.c \
    common-init.c compat/nonblock.c compat/obstack.c compat/open.c compat/terminal.c \
    compiler-tricks/not-constant.c config.c connect.c connected.c convert.c copy.c \
    credential.c csum-file.c ctype.c date.c decorate.c delta-islands.c diagnose.c \
    diff-delta.c diff-merges.c diff-lib.c diff-no-index.c diff.c diffcore-break.c \
    diffcore-delta.c diffcore-order.c diffcore-pickaxe.c diffcore-rename.c diffcore-rotate.c \
    dir-iterator.c dir.c editor.c entry.c environment.c ewah/bitmap.c ewah/ewah_bitmap.c \
    ewah/ewah_io.c ewah/ewah_rlw.c exec-cmd.c fetch-negotiator.c fetch-object-info.c \
    fetch-pack.c fmt-merge-msg.c fsck.c fsmonitor.c fsmonitor-ipc.c fsmonitor-settings.c \
    gettext.c git-zlib.c gpg-interface.c graph.c grep.c hash-lookup.c hash.c hashmap.c \
    help.c hex.c hex-ll.c hook.c ident.c json-writer.c kwset.c levenshtein.c line-log.c \
    line-range.c linear-assignment.c list-objects-filter-options.c list-objects-filter.c \
    list-objects.c lockfile.c log-tree.c loose.c ls-refs.c mailinfo.c mailmap.c \
    match-trees.c mem-pool.c merge-blobs.c merge-ll.c merge-ort.c merge-ort-wrappers.c \
    merge.c midx.c midx-write.c name-hash.c negotiator/default.c negotiator/noop.c \
    negotiator/skipping.c notes-cache.c notes-merge.c notes-utils.c notes.c \
    object-file-convert.c object-file.c object-name.c object.c odb.c odb/source.c \
    odb/source-files.c odb/source-inmemory.c odb/source-loose.c odb/source-packed.c \
    odb/streaming.c odb/transaction.c oid-array.c oidmap.c oidset.c oidtree.c \
    pack-bitmap-write.c pack-bitmap.c pack-check.c pack-mtimes.c pack-objects.c pack-refs.c \
    pack-revindex.c pack-write.c packfile.c packfile-list.c pager.c parallel-checkout.c \
    parse.c parse-options-cb.c parse-options.c patch-delta.c patch-ids.c path.c path-walk.c \
    pathspec.c pkt-line.c preload-index.c pretty.c prio-queue.c progress.c promisor-remote.c \
    prompt.c protocol.c protocol-caps.c prune-packed.c pseudo-merge.c quote.c range-diff.c \
    reachable.c read-cache.c rebase-interactive.c rebase.c ref-filter.c reflog-walk.c \
    reflog.c refs.c refs/debug.c refs/files-backend.c refs/reftable-backend.c \
    refs/iterator.c refs/packed-backend.c refs/ref-cache.c refspec.c reftable/basics.c \
    reftable/block.c reftable/blocksource.c reftable/error.c reftable/fsck.c reftable/iter.c \
    reftable/merged.c reftable/pq.c reftable/record.c reftable/stack.c reftable/system.c \
    reftable/table.c reftable/tree.c reftable/writer.c remote.c repack.c repack-cruft.c \
    repack-filtered.c repack-geometry.c repack-midx.c repack-promisor.c replace-object.c \
    replay.c repo-settings.c repository.c rerere.c reset.c resolve-undo.c revision.c \
    run-command.c send-pack.c sequencer.c serve.c server-info.c setup.c shallow.c sideband.c \
    sigchain.c sparse-index.c split-index.c stable-qsort.c statinfo.c strbuf.c string-list.c \
    strmap.c strvec.c sub-process.c submodule-config.c submodule.c symlinks.c tag.c \
    tempfile.c thread-utils.c tmp-objdir.c trace.c trace2.c trace2/tr2_cfg.c \
    trace2/tr2_cmd_name.c trace2/tr2_ctr.c trace2/tr2_dst.c trace2/tr2_sid.c \
    trace2/tr2_sysenv.c trace2/tr2_tbuf.c trace2/tr2_tgt_event.c trace2/tr2_tgt_normal.c \
    trace2/tr2_tgt_perf.c trace2/tr2_tls.c trace2/tr2_tmr.c trailer.c transport-helper.c \
    transport.c tree-diff.c tree-walk.c tree.c unpack-trees.c upload-pack.c url.c urlmatch.c \
    usage.c userdiff.c utf8.c varint.c version.c versioncmp.c walker.c wildmatch.c \
    worktree.c wrapper.c write-or-die.c ws.c wt-status.c xdiff-interface.c xdiff/xdiffi.c \
    xdiff/xemit.c xdiff/xhistogram.c xdiff/xmerge.c xdiff/xpatience.c xdiff/xprepare.c \
    xdiff/xutils.c compat/stub/procinfo.c compat/memmem.c compat/strcasestr.c \
    compat/strlcpy.c compat/strtoumax.c compat/strtoimax.c compat/regex/regex.c \
    sha1dc/sha1.c sha1dc/ubc_check.c sha1dc_git.c sha256/block/sha256.c \
    compat/qsort_s.c unix-socket.c unix-stream-server.c

GIT_BUILTIN_SOURCES := \
    builtin/add.c builtin/am.c builtin/annotate.c builtin/apply.c builtin/archive.c \
    builtin/backfill.c builtin/bisect.c builtin/blame.c builtin/branch.c builtin/bugreport.c \
    builtin/bundle.c builtin/cat-file.c builtin/check-attr.c builtin/check-ignore.c \
    builtin/check-mailmap.c builtin/check-ref-format.c builtin/checkout--worker.c \
    builtin/checkout-index.c builtin/checkout.c builtin/clean.c builtin/clone.c \
    builtin/column.c builtin/commit-graph.c builtin/commit-tree.c builtin/commit.c \
    builtin/config.c builtin/count-objects.c builtin/credential-cache--daemon.c \
    builtin/credential-cache.c builtin/credential-store.c builtin/credential.c \
    builtin/describe.c builtin/diagnose.c builtin/diff-files.c builtin/diff-index.c \
    builtin/diff-pairs.c builtin/diff-tree.c builtin/diff.c builtin/difftool.c \
    builtin/fast-export.c builtin/fast-import.c builtin/fetch-pack.c builtin/fetch.c \
    builtin/fmt-merge-msg.c builtin/for-each-ref.c builtin/for-each-repo.c builtin/fsck.c \
    builtin/fsmonitor--daemon.c builtin/gc.c builtin/get-tar-commit-id.c builtin/grep.c \
    builtin/hash-object.c builtin/help.c builtin/history.c builtin/hook.c \
    builtin/index-pack.c builtin/init-db.c builtin/interpret-trailers.c \
    builtin/last-modified.c builtin/log.c builtin/ls-files.c builtin/ls-remote.c \
    builtin/ls-tree.c builtin/mailinfo.c builtin/mailsplit.c builtin/merge-base.c \
    builtin/merge-file.c builtin/merge-index.c builtin/merge-ours.c \
    builtin/merge-recursive.c builtin/merge-tree.c builtin/merge.c builtin/mktag.c \
    builtin/mktree.c builtin/multi-pack-index.c builtin/mv.c builtin/name-rev.c \
    builtin/notes.c builtin/pack-objects.c builtin/pack-redundant.c builtin/pack-refs.c \
    builtin/patch-id.c builtin/prune-packed.c builtin/prune.c builtin/pull.c builtin/push.c \
    builtin/range-diff.c builtin/read-tree.c builtin/rebase.c builtin/receive-pack.c \
    builtin/reflog.c builtin/refs.c builtin/remote-ext.c builtin/remote-fd.c \
    builtin/remote.c builtin/repack.c builtin/replace.c builtin/replay.c builtin/repo.c \
    builtin/rerere.c builtin/reset.c builtin/rev-list.c builtin/rev-parse.c builtin/revert.c \
    builtin/rm.c builtin/send-pack.c builtin/shortlog.c builtin/show-branch.c \
    builtin/show-index.c builtin/show-ref.c builtin/sparse-checkout.c builtin/stash.c \
    builtin/stripspace.c builtin/submodule--helper.c builtin/symbolic-ref.c builtin/tag.c \
    builtin/unpack-file.c builtin/unpack-objects.c builtin/update-index.c \
    builtin/update-ref.c builtin/update-server-info.c builtin/upload-archive.c \
    builtin/upload-pack.c builtin/url-parse.c builtin/var.c builtin/verify-commit.c \
    builtin/verify-pack.c builtin/verify-tag.c builtin/worktree.c builtin/write-tree.c
