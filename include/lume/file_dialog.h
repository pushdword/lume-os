/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef LUME_FILE_DIALOG_H
#define LUME_FILE_DIALOG_H

#define LUME_FILE_DIALOG_PATH "/data/libexec/chromium/lume-file-dialog"
#define LUME_FILE_DIALOG_REQUEST_MAGIC 0x3144464cU
#define LUME_FILE_DIALOG_RESPONSE_MAGIC 0x3152464cU
#define LUME_FILE_DIALOG_REQUEST_LIMIT 65536U
#define LUME_FILE_DIALOG_RESPONSE_LIMIT 1048576U
#define LUME_FILE_DIALOG_PATH_LIMIT 1023U
#define LUME_FILE_DIALOG_SELECTION_LIMIT 256U
#define LUME_FILE_DIALOG_FILTER_LIMIT 128U
#define LUME_FILE_DIALOG_PATTERN_LIMIT 256U

enum lume_file_dialog_action
{
    LUME_FILE_DIALOG_FOLDER = 1,
    LUME_FILE_DIALOG_UPLOAD_FOLDER = 2,
    LUME_FILE_DIALOG_EXISTING_FOLDER = 3,
    LUME_FILE_DIALOG_SAVE = 4,
    LUME_FILE_DIALOG_OPEN = 5,
    LUME_FILE_DIALOG_MULTIPLE = 6
};

/* Records contain little-endian uint32 values and length-prefixed UTF-8 strings.
 * Strings exclude their terminator and cannot contain a NUL byte. The request
 * ends at EOF before a dialog is shown. A response is published only after the
 * entire selection has been validated. */

#endif
