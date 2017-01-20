
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
    int diff = I - K;
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
  //const unsigned int min_read_unit = 8;
  unsigned int min_read_unit = area;
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

  double totalYMSE = 0.0;
  double totalUMSE = 0.0;
  double totalVMSE = 0.0;

  double frameYMSE = 0.0;
  double frameUMSE = 0.0;
  double frameVMSE = 0.0;

  cout << "Frame       Y-PSNR[dB]  U-PSNR[dB]  V-PSNR[dB]" << endl;

  while( !fin0.eof() ){

    fin0.read( (char*)c0, min_read_unit ); 
    fin1.read( (char*)c1, min_read_unit ); 
    count += min_read_unit;

    // extract Y
    if( count <= area ) { 
      MSE(c0, c1, min_read_unit, frameYMSE);
      min_read_unit = area/4;

    // extract U
    } else if( area < count && count <= area/4*5 ) { // YUV420
      MSE(c0, c1, min_read_unit, frameUMSE);

    // extract V
    } else if( area/4*5 < count ) { // YUV420
      MSE(c0, c1, min_read_unit, frameVMSE);
    }

    // end of 1 frame
    if( count == area/2*3 ) {

      totalYMSE += frameYMSE;
      totalUMSE += frameUMSE;
      totalVMSE += frameVMSE;

      frameYMSE /= area;
      frameUMSE /= area/4;
      frameVMSE /= area/4;

      if( opt.showFrame ) {

        double y = MSE2PSNR(frameYMSE, MAX);
        double u = MSE2PSNR(frameUMSE, MAX);
        double v = MSE2PSNR(frameVMSE, MAX);
        cout << boost::format("%-12d%-12.4f%-12.4f%-12.4f") % nframe % y % u % v << endl;
      }

      frameYMSE = 0.0;
      frameUMSE = 0.0;
      frameVMSE = 0.0;

      nframe++;
      count = 0;
      min_read_unit = area;
    }
  }

  totalYMSE /= area*nframe;
  totalUMSE /= area/4*nframe;
  totalVMSE /= area/4*nframe;

  double y = MSE2PSNR(totalYMSE, MAX);
  double u = MSE2PSNR(totalUMSE, MAX);
  double v = MSE2PSNR(totalVMSE, MAX);
  cout << boost::format("Total       %-12.4f%-12.4f%-12.4f") % y % u % v << endl;

  delete [] c0;
  delete [] c1;

  fin0.close();
  fin1.close();
    
  return 0;
}
