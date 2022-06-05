set -e
mkdir -p src/tmp
bin/util/EzSpriteSheet-cli -i res/gfx -o src/tmp/gfx.h -s c99 -m maxrects -a 1024x1024 -e -t -d -c 00ff00
mv src/tmp/*.png res
