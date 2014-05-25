// 

#include <cfloat>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include <memory>
#include <thread>
#include <future>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

using namespace std;

double MSE
( const vector<unsigned char>& c0_vec
, const vector<unsigned char>& c1_vec
, unsigned int min_read_unit
)
{
    double MSE = 0.0;
    for( int i = 0; i < min_read_unit; i++ ) {
        unsigned char I = c0_vec[i];
        unsigned char K = c1_vec[i];
        int diff = I - K;
        MSE += diff*diff;
    }
    return MSE;
}

double MSE2PSNR
( double MSE
)
{
    const unsigned char MAX = 0xff;
    double PSNR = INFINITY;

    if( DBL_EPSILON < MSE ) {
        PSNR = 10 * log10( (MAX*MAX)/MSE );
    }

    return PSNR;
}

struct Option {
    int nthread;
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
        } else if( op == "-t" ) {
            i++;
            opt.nthread = 3*boost::lexical_cast<int>(argv[i]);
        } else if( op == "-v" ) {
            opt.showFrame = true;
        }
    }
}

class Fvalue 
{
public:

    Fvalue(){}

    Fvalue( Fvalue&& org ) {
        f = move(org.f);
        c0vec = org.c0vec;
        c1vec = org.c1vec;
    }

    Fvalue& operator= ( Fvalue&& org ) {
        f = move(org.f);
        c0vec = org.c0vec;
        c1vec = org.c1vec;
        return *this;
    }

    future<double> f;
    shared_ptr<vector<unsigned char>> c0vec;
    shared_ptr<vector<unsigned char>> c1vec;
};

class Yuv2psnr {

public:

    Yuv2psnr(int argc, char* argv[]);
    ~Yuv2psnr();
    void check();
    void run();

private:

    //
    void sync_frame();

    //
    Option opt;
    ifstream fin0;
    ifstream fin1;
    unsigned int area;
    unsigned int min_read_unit;
    int count = 0;
    int nframe = 0;
    double totalYMSE = 0.0;
    double totalUMSE = 0.0;
    double totalVMSE = 0.0;
    double frameYMSE = 0.0;
    double frameUMSE = 0.0;
    double frameVMSE = 0.0;
    deque<Fvalue> fydeque;
    deque<Fvalue> fudeque;
    deque<Fvalue> fvdeque;
};

Yuv2psnr::Yuv2psnr(int argc, char* argv[])
: count(0)
, nframe(0)
, totalYMSE(0.0)
, totalUMSE(0.0)
, totalVMSE(0.0)
, frameYMSE(0.0)
, frameUMSE(0.0)
, frameVMSE(0.0)
{
    //Option opt;
    opt.nthread = 1;
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

    area = opt.width*opt.height;
    min_read_unit = area;

    fin0.open( opt.file0.c_str(), ios::in | ios::binary );
    fin1.open( opt.file1.c_str(), ios::in | ios::binary );
}

Yuv2psnr::~Yuv2psnr()
{
    fin0.close();
    fin1.close();
}

void Yuv2psnr::check()
{
    if( fin0.fail() ){
        stringstream ss;
        ss << "Error: cannot open file `" << opt.file0 << "'";
        throw ss.str();
    }

    if( fin1.fail() ){
        stringstream ss;        
        cout << "Error: cannot open file `" << opt.file1 << "'";
        throw ss.str();
    }
}

void Yuv2psnr::sync_frame()
{
    frameYMSE = fydeque.front().f.get();
    fydeque.pop_front();

    frameUMSE = fudeque.front().f.get();
    fudeque.pop_front();

    frameVMSE = fvdeque.front().f.get();
    fvdeque.pop_front();

    totalYMSE += frameYMSE;
    totalUMSE += frameUMSE;
    totalVMSE += frameVMSE;

    frameYMSE /= area;
    frameUMSE /= area/4;
    frameVMSE /= area/4;

    if( opt.showFrame ) {

        double y = MSE2PSNR(frameYMSE);
        double u = MSE2PSNR(frameUMSE);
        double v = MSE2PSNR(frameVMSE);
        cout << boost::format("%-12d%-12.4f%-12.4f%-12.4f") % nframe % y % u % v << endl;
    }

    frameYMSE = 0.0;
    frameUMSE = 0.0;
    frameVMSE = 0.0;
    nframe++;
}

void Yuv2psnr::run()
{
    cout << "Frame       Y-PSNR[dB]  U-PSNR[dB]  V-PSNR[dB]" << endl;

    while( !fin0.eof() ) {

        Fvalue vf;

        shared_ptr<vector<unsigned char>> c0_vec_ptr(new vector<unsigned char>(min_read_unit));
        shared_ptr<vector<unsigned char>> c1_vec_ptr(new vector<unsigned char>(min_read_unit));
        vf.c0vec = c0_vec_ptr;
        vf.c1vec = c1_vec_ptr;

        char* c0 = (char*)&(c0_vec_ptr->at(0));
        char* c1 = (char*)&(c1_vec_ptr->at(0));
        fin0.read( c0, min_read_unit ); 
        fin1.read( c1, min_read_unit ); 

        count += min_read_unit;

        // extract Y
        if( count <= area ) { 
            auto fy  = async(launch::async, MSE, *c0_vec_ptr.get(), *c1_vec_ptr.get(), min_read_unit);
            vf.f = move(fy);
            fydeque.push_back(move(vf));
            min_read_unit = area/4;

        // extract U
        } else if( area < count && count <= area/4*5 ) {
            auto fu  = async(launch::async, MSE, *c0_vec_ptr.get(), *c1_vec_ptr.get(), min_read_unit);
            vf.f = move(fu);
            fudeque.push_back(move(vf));

        // extract V
        } else if( area/4*5 < count ) {
            auto fv  = async(launch::async, MSE, *c0_vec_ptr.get(), *c1_vec_ptr.get(), min_read_unit);
            vf.f = move(fv);
            fvdeque.push_back(move(vf));
        }

        // end of 1 frame
        if( count == area/2*3 ) {
            count = 0;
            min_read_unit = area;
        }

        while( opt.nthread <= fydeque.size() + fudeque.size() + fvdeque.size() ){
            sync_frame();
        }
    }

    while( !fvdeque.empty() ){
        sync_frame();
    }

    totalYMSE /= area*nframe;
    totalUMSE /= area/4*nframe;
    totalVMSE /= area/4*nframe;

    double y = MSE2PSNR(totalYMSE);
    double u = MSE2PSNR(totalUMSE);
    double v = MSE2PSNR(totalVMSE);
    cout << boost::format("Total       %-12.4f%-12.4f%-12.4f") % y % u % v << endl;
}

int main(int argc, char* argv[])
{
    try {

        Yuv2psnr yuv2psnr(argc, argv);
        yuv2psnr.run();

    } catch(string& what) {

        cerr << what << endl;
        return 0x01;

    } catch(...) {

        cerr << "Error: unexpectd expection" << endl;
        return 0x02;
    }

    return 0x00;
}
