
#include <cfloat>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

using namespace std;

void MSE
( unsigned char* c0
, unsigned char* c1
, unsigned int min_read_unit
, double& MSE
)
{
  for( int i = 0; i < min_read_unit; i++ ) {
    unsigned char I = c0[i];
    unsigned char K = c1[i];
    double diff = I - K;
    MSE += diff*diff;
  }
}

double MSE2PSNR
( double MSE
, double MAX
)
{
  double PSNR = INFINITY;

  if( DBL_EPSILON < MSE ) {
    PSNR = 10 * log10( (MAX*MAX)/MSE );
  }

  return PSNR;
}

struct Option {
  int width;
  int height;
  string file0;
  string file1;
  bool showFrame;
};

void parse_option
( int argc
, char* argv[]
, Option& opt
)
{
  for( int i = 1; i < argc; i++ ) {

    string op = argv[i];
    
    if( op == "-w" ) {
      i++;
      opt.width = boost::lexical_cast<int>(argv[i]);
    } else if( op == "-h" ) {
      i++;
      opt.height = boost::lexical_cast<int>(argv[i]);
    } else if( op == "-i0" ) {
      i++;
      opt.file0 = argv[i];
    } else if( op == "-i1" ) {
      i++;
      opt.file1 = argv[i];
    } else if( op == "-v" ) {
      opt.showFrame = true;
    }
  }
}

int main(int argc, char* argv[])
{
  Option opt;
  // CIF
  opt.width = 352;
  opt.height = 288;
  // HD
  //int width  = 1920;
  //int height = 1080;
  opt.file0 = "";
  opt.file1 = "";
  opt.showFrame = false;
  parse_option(argc, argv, opt);

  const unsigned int area = opt.width*opt.height;
  //const unsigned int min_read_unit = 16;
  const unsigned int min_read_unit = 8;
  const unsigned char MAX = 255;

  ifstream fin0( opt.file0.c_str(), ios::in | ios::binary );
  ifstream fin1( opt.file1.c_str(), ios::in | ios::binary );
    
  if( !fin0 ){
    cout << "Error: cannot open file `" << opt.file0 << "'" << endl;
    return 1;
  }

  if( !fin1 ){
    cout << "Error: cannot open file `" << opt.file1 << "'" << endl;
    return 1;
  }

  int count = 0;    
  int nframe = 0;
  unsigned char* c0 = new unsigned char[min_read_unit];
  unsigned char* c1 = new unsigned char[min_read_unit];

  double totalYMSE  = 0.0;
  double totalCbMSE = 0.0;
  double totalCrMSE = 0.0;

  double frameYMSE  = 0.0;
  double frameCbMSE = 0.0;
  double frameCrMSE = 0.0;

  cout << "Frame       Y-PSNR[dB]  U-PSNR[dB]  V-PSNR[dB]" << endl;

  while( !fin0.eof() ){

    fin0.read( (char*)c0, min_read_unit ); 
    fin1.read( (char*)c1, min_read_unit ); 
    count += min_read_unit;

    // extract Y
    if( count < area ) { 
      MSE(c0, c1, min_read_unit, frameYMSE);

    // extract Cb
    } else if( area <= count && count < area/4*5 ) { // YUV420
      MSE(c0, c1, min_read_unit, frameCbMSE);

    // extract Cr
    } else if( area/4*5 <= count ) { // YUV420
      MSE(c0, c1, min_read_unit, frameCrMSE);
    }

    // end of 1 frame
    if( count == area/2*3 ) {

      totalYMSE  += frameYMSE;
      totalCbMSE += frameCbMSE;
      totalCrMSE += frameCrMSE;

      frameYMSE  /= area;
      frameCbMSE /= area/4;
      frameCrMSE /= area/4;

      if( opt.showFrame ) {

	double y  = MSE2PSNR(frameYMSE,  MAX);
	double cb = MSE2PSNR(frameCbMSE, MAX);
	double cr = MSE2PSNR(frameCrMSE, MAX);
	cout << boost::format("%-12d%-12.4f%-12.4f%-12.4f") % nframe % y % cb % cr << endl;
      }

      frameYMSE  = 0.0;
      frameCbMSE = 0.0;
      frameCrMSE = 0.0;

      nframe++;
      count = 0;
    }
  }

  totalYMSE  /= area*nframe;
  totalCbMSE /= area/4*nframe;
  totalCrMSE /= area/4*nframe;

  double y  = MSE2PSNR(totalYMSE,  MAX);
  double cb = MSE2PSNR(totalCbMSE, MAX);
  double cr = MSE2PSNR(totalCrMSE, MAX);
  cout << boost::format("Total       %-12.4f%-12.4f%-12.4f") % y % cb % cr << endl;

  delete [] c0;
  delete [] c1;

  fin0.close();
  fin1.close();
    
  return 0;
}
