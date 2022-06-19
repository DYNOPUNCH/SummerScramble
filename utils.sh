set -e

# where tools will reside
mkdir -p bin/util/syrup

# ogmo2c
gcc -o bin/util/syrup/ogmo2c dep/syrup/util/ogmo2c/*.c -Idep/syrup/util/ogmo2c dep/syrup/util/ogmo2c/cjson/*.c -Wall -Wextra -lm -Os -s -flto

# vfsgen
gcc -o bin/util/syrup/vfsgen dep/syrup/util/vfsgen/*.c -Wall -Wextra

