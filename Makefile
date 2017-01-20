all:
	g++ -o yuv2psnr -O3 -std=c++11 yuv2psnr.cpp

.PHONY: test

test:
	cd test; make

