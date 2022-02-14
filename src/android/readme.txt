Android
=======

Prepare the project
-------------------

SDL2 https://www.libsdl.org/download-2.0.php
Source Code: https://www.libsdl.org/release/SDL2-2.0.16.zip

SDL_ttf 2.0 https://www.libsdl.org/projects/SDL_ttf/
Source: https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-2.0.15.zip

SDL_image 2.0 https://www.libsdl.org/projects/SDL_image/
Source: https://www.libsdl.org/projects/SDL_image/release/SDL2_image-2.0.5.zip

SDL_mixer 2.0 https://www.libsdl.org/projects/SDL_mixer/
Source: https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-2.0.4.zip

Extract them, copy or symlink directory into:
  android/pwmangclient/jni/SDL2
  android/pwmangclient/jni/SDL2_image
  android/pwmangclient/jni/SDL2_ttf
  android/pwmangclient/jni/SDL2_mixer

Copy or symlink pwmangband lib directory into pwmangband src/android/pwmangclient/src/main/assets/lib
  cd src/android
  ln -s ../../../../../../lib pwmangclient/src/main/assets/lib

Prepare gradle
--------------

Download and install Android Studio.
https://developer.android.com/studio

TODO
