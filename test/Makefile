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
	for YU in $(YUV); do\
		echo -n "$$YU...";\
		../yuv2psnr_s -i0 test0.yuv -i1 $$YU -v -t 4 > tmp; diff --strip-trailing-cr tmp $$YU.log && echo OK;\
	done;
	rm -f tmp;
