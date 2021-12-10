rm speexenc -f
rm speexdec -f
gcc speexenc.c -I speex_ogg/speex/include/ -L.././speex/libspeex/.libs -lspeex -o speexenc
gcc speexdec.c -I speex_ogg/speex/include/ -L.././speex/libspeex/.libs -lspeex -o speexdec
