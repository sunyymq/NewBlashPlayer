#!/bin/sh

BUILD_DIR=`pwd`/build
if [ ! -r $BUILD_DIR ]
then
	mkdir $BUILD_DIR
fi
# change directory to build
cd $BUILD_DIR

# directories
URL_VERSION="7.46.0"
if [[ $CURL_VERSION != "" ]]; then
  URL_VERSION=$CURL_VERSION
fi
SOURCE="curl-$URL_VERSION"
FAT="curl-iOS"

SCRATCH="curl_scratch"
# must be an absolute path
THIN=`pwd`/"curl_thin"

# disable protocols
#     		  --disable-protocols \
#                 --disable-protocol=http \
#                 --disable-protocol=rtp \
#                 --disable-protocol=tcp \
#                 --disable-protocol=udp \

CONFIGURE_FLAGS="--enable-cross-compile \
                 --disable-shared \
                 --enable-static \
                 --disable-manual \
                 --disable-verbose \
                 --without-ldap \
                 --disable-ldap \
                 --enable-ipv6 \
                 --enable-threaded-resolver \
                 --enable-pic"

#./configure --host=${HOST_VAL} --prefix=${CURL_BUILD_DIR}/${ARCH} --disable-shared --enable-static --disable-manual --disable-verbose --without-ldap --disable-ldap --enable-ipv6 --enable-threaded-resolver --with-zlib="${IOS_SDK_PATH}/usr" --with-ssl="/Users/x/Desktop/openssl-1.0.2e-build/universal" &> ${CURL_BUILD_LOG_DIR}/${ARCH}-conf.log


#ARCHS="arm64 armv7 x86_64 i386"
ARCHS="x86_64"

COMPILE="y"
LIPO="y"

DEPLOYMENT_TARGET="8.0"

if [ "$*" ]
then
	if [ "$*" = "lipo" ]
	then
		# skip compile
		COMPILE=
	else
		ARCHS="$*"
		if [ $# -eq 1 ]
		then
			# skip lipo
			LIPO=
		fi
	fi
fi

if [ "$COMPILE" ]
then
	# if [ ! `which yasm` ]
	# then
	# 	echo 'Yasm not found'
	# 	if [ ! `which brew` ]
	# 	then
	# 		echo 'Homebrew not found. Trying to install...'
	#                        ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)" \
	# 			|| exit 1
	# 	fi
	# 	echo 'Trying to install Yasm...'
	# 	brew install yasm || exit 1
	# fi
	# if [ ! `which gas-preprocessor.pl` ]
	# then
	# 	echo 'gas-preprocessor.pl not found. Trying to install...'
	# 	(curl -L https://github.com/libav/gas-preprocessor/raw/master/gas-preprocessor.pl \
	# 		-o /usr/local/bin/gas-preprocessor.pl \
	# 		&& chmod +x /usr/local/bin/gas-preprocessor.pl) \
	# 		|| exit 1
	# fi

	CWD=`pwd`
	if [ ! -r $SOURCE ]
	then
		echo 'curl source not found. Trying to download...'
		axel -n 16 -o $CWD/$SOURCE.tar.bz2 -v https://curl.haxx.se/download/$SOURCE.tar.bz2
		tar zxvf $CWD/$SOURCE.tar.bz2
		# curl http://www.ffmpeg.org/releases/$SOURCE.tar.bz2 | tar xj \
		# 	|| exit 1
	fi


	for ARCH in $ARCHS
	do
		echo "building $ARCH..."
		mkdir -p "$SCRATCH/$ARCH"
		cd "$SCRATCH/$ARCH"

		CFLAGS="-arch $ARCH"
		if [ "$ARCH" = "i386" -o "$ARCH" = "x86_64" ]
		then
		    PLATFORM="iPhoneSimulator"
		    CFLAGS="$CFLAGS -mios-simulator-version-min=$DEPLOYMENT_TARGET"
		else
		    PLATFORM="iPhoneOS"
		    CFLAGS="$CFLAGS -mios-version-min=$DEPLOYMENT_TARGET -fembed-bitcode"
		    if [ "$ARCH" = "arm64" ]
		    then
		        EXPORT="GASPP_FIX_XCODE5=1"
		    fi
		fi

		XCRUN_SDK=`echo $PLATFORM | tr '[:upper:]' '[:lower:]'`
		CC="xcrun -sdk $XCRUN_SDK clang"

		# # force "configure" to use "gas-preprocessor.pl" (FFmpeg 3.4.4)
		# if [ "$ARCH" = "arm64" ]
		# then
		#     AS="gas-preprocessor.pl -arch aarch64 -- $CC"
		# else
		#     AS="gas-preprocessor.pl -- $CC"
		# fi

		CXXFLAGS="$CFLAGS"
		LDFLAGS="$CFLAGS"

		TMPDIR=${TMPDIR/%\/} $CWD/$SOURCE/configure \
		    --host=$ARCH-apple-darwin \
		    CC="$CC" \
		    $CONFIGURE_FLAGS \
		    CFLAGS="$CFLAGS" \
		    LDFLAGS="$LDFLAGS" \
		    --prefix="$THIN/$ARCH" \
		|| exit 1

		make -j8 install $EXPORT || exit 1
		cd $CWD
	done
fi

if [ "$LIPO" ]
then
	echo "building fat binaries..."
	mkdir -p $FAT/lib
	set - $ARCHS
	CWD=`pwd`
	cd $THIN/$1/lib
	for LIB in *.a
	do
		cd $CWD
		echo lipo -create `find $THIN -name $LIB` -output $FAT/lib/$LIB 1>&2
		lipo -create `find $THIN -name $LIB` -output $FAT/lib/$LIB || exit 1
	done

	cd $CWD
	cp -rf $THIN/$1/include $FAT
fi

cp -rf $FAT `pwd`/../curl

echo Done