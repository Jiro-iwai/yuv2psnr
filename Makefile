all:
	g++ -o yuv2psnr -O3 -std=c++11 yuv2psnr_single.cpp

test:
	cd test; make

