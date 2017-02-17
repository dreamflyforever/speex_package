rm speexenc -f
rm speexdec -f
gcc speexenc.c -I ../speex/include/ -L.././speex/libspeex/.libs -lspeex -o speexenc
gcc speexdec.c -I ../speex/include/ -L.././speex/libspeex/.libs -lspeex -o speexdec
