set -e

# clean any files that might already be present
rm -f src/tmp/vfs.h

# generate new files
bin/util/syrup/vfsgen res -x gfx/ -xp .json -xp .ogmo -x ogmo/sprite > src/tmp/vfs.h

