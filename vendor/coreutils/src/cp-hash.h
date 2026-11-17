#include <stdint.h>
void hash_init (void);
void forget_created (uintmax_t ino, dev_t dev);
char *remember_copied (char const *node, uintmax_t ino, dev_t dev)
  _GL_ATTRIBUTE_NONNULL ();
char *src_to_dest_lookup (uintmax_t ino, dev_t dev);
