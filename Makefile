
all:
#	g++ -o yuv2psnr -O3 main.cpp
	g++ -o yuv2psnr -g -std=c++11 yuv2psnr.cpp


YUV=\
	test1.yuv \
	test2.yuv \
	test3.yuv \
	test4.yuv \
	test5.yuv \
	test6.yuv \
	test7.yuv \
	test8.yuv \
	test9.yuv \
	test10.yuv

test:
	g++ -o tcg -g -std=c++11 tcg.cpp
	for YU in $(YUV); do\
		echo -n "$$YU..."; ./yuv2psnr -i0 test0.yuv -i1 $$YU -v > tmp; diff tmp $$YU.log && echo OK;\
	done;
	rm -f tmp;