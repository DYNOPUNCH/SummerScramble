set -e
mkdir -p bin
mkdir -p src/tmp
bin/util/syrup/ogmo2c res/ogmo/ogmo.ogmo "\t" "obj" src/entity src/entity/headers src/tmp/entity-all.h src/tmp/entity-ids.h src/tmp/entity-classes.h src/tmp/rooms.h
gcc -o bin/SummerScramble -Idep dep/*.c -Idep/syrup dep/syrup/*.c -Isrc src/*.c src/entity/*.c -Isrc/entity/headers -lSDL2 -lm -pedantic -std=c99

# testing other compilers/settings; please ignore this
#tcc -o bin/SummerScramble -Idep dep/*.c -D_INC_VADEFS -IC:\\msys64\\mingw64\\include -Idep/syrup dep/syrup/*.c -Isrc src/*.c src/entity/*.c -lSDL2 -lm
#tcc -o bin/SummerScramble -Idep dep/*.c -DSTBI_NO_SIMD -Ilib/include -IC:\\tcc\\include -Idep/syrup dep/syrup/*.c -Isrc src/*.c src/entity/*.c -lSDL2 -lm
#x86_64-w64-mingw32-gcc -o bin/SummerScramble -I/home/suse/c/bin/sdl2/include -Idep dep/*.c -Idep/syrup dep/syrup/*.c -Isrc src/*.c src/entity/*.c -L/home/suse/c/bin/sdl2/lib -lSDL2 -lm -Os -s -flto

# create symbolic link so ogmo decals can be accessed from build directory
#ln -sfn ../res/ogmo/decal bin/decal

# fonts too
#ln -sfn ../res/font bin/font
