brew install libtool shtool autogen autoconf
autoreconf -i
./configure
make CFLAGS="-D_XOPEN_SOURCE=1 -DUNW_REMOTE_ONLY=1 -I../include/darwin -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk/usr/include/i386 -include forced-include.h"