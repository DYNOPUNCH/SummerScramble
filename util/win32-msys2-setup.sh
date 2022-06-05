pacman --noconfirm -S --needed base-devel mingw-w64-x86_64-toolchain
yes | pacman -S mingw64/mingw-w64-x86_64-cjson
yes | pacman -S mingw64/mingw-w64-x86_64-SDL2
echo "export PATH=\"C:\\\\msys64\\\\mingw64\\\\bin:\$PATH\"" >> ~/.bashrc
source ~/.bashrc
