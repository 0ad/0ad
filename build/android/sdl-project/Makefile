ANDROID=${HOME}/android
NDK=${ANDROID}/android-ndk-r7-crystax-4
SDK=${ANDROID}/android-sdk-linux
TOOLCHAIN=${ANDROID}/toolchain-0ad
SYSROOT=${TOOLCHAIN}/sysroot

AAPT=${SDK}/platform-tools/aapt
ADB=${SDK}/platform-tools/adb
DX=${SDK}/platform-tools/dx
ZIPALIGN=${SDK}/tools/zipalign
NDKBUILD=${NDK}/ndk-build

ANDROIDJAR=${SDK}/platforms/android-10/android.jar

LIBPYRO=libpyrogenesis_dbg.so
#LIBPYRO=libpyrogenesis.so

PACKAGE=org/libsdl/app
#PACKAGE=com/wildfiregames/zeroad

all: bin/0ad.apk

gen/${PACKAGE}/R.java:
	mkdir -p bin/res
	mkdir -p gen
	${AAPT} package -f -m -J gen/ -M AndroidManifest.xml -S bin/res -S res -I ${ANDROIDJAR}

bin/classes/${PACKAGE}/SDLActivity.class: src/${PACKAGE}/SDLActivity.java gen/${PACKAGE}/R.java
	mkdir -p bin/classes/${PACKAGE}/
	javac \
		-d bin/classes \
		-classpath bin/classes:. \
		-sourcepath src:gen \
		-target 1.5 \
		-bootclasspath ${ANDROIDJAR} \
		-encoding UTF-8 \
		-g \
		-source 1.5 \
		$^

bin/classes.dex: bin/classes/${PACKAGE}/SDLActivity.class
	${DX} --dex --output $@ bin/classes

libs/armeabi/libmain.so libs/armeabi/libSDL2.so: jni/src/pyrogenesis_wrapper.cpp
	${NDKBUILD}

lib/armeabi/libmain.so lib/armeabi/libSDL2.so: libs/armeabi/libmain.so libs/armeabi/libSDL2.so
	mkdir -p lib/armeabi
	cp $^ lib/armeabi/

bin/0ad.unaligned.apk: bin/classes.dex lib/armeabi/libmain.so lib/armeabi/libSDL2.so
	${AAPT} package -f -F $@ -M AndroidManifest.xml -S bin/res -S res -I ${ANDROIDJAR}
	zip $@ -j bin/classes.dex
	zip $@ -r lib
	jarsigner -keystore ${HOME}/.android/debug.keystore -storepass android $@ androiddebugkey

bin/0ad.apk: bin/0ad.unaligned.apk
	${ZIPALIGN} 4 $< $@

push-apk: bin/0ad.apk
	${ADB} push $< /sdcard/

push-so:
	cp ../../../binaries/system/${LIBPYRO} libs/armeabi/
	${TOOLCHAIN}/bin/arm-linux-androideabi-strip libs/armeabi/${LIBPYRO}
	${ADB} push libs/armeabi/${LIBPYRO} /data/local/
	
clean:
	rm -rf bin gen lib libs obj
