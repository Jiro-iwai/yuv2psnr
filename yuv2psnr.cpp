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
)
{
    double MSE = 0.0;
    auto i0 = c0_vec.begin();
    auto i1 = c1_vec.begin();
    auto end = c0_vec.end();
    while( i0 != end ) {
        unsigned char I = *i0++;
        unsigned char K = *i1++;
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

class Option {

public:
    Option();
    void parse(int argc, char* argv[]);

public:
    int nthread;
    int width;
    int height;
    string file0;
    string file1;
    bool showFrame;
};

Option::Option()
: nthread(1)
, width(352)
, height(288)
, file0("")
, file1("")
, showFrame(false)
{
}

void Option::parse(int argc, char* argv[])
{
    for( int i = 1; i < argc; i++ ) {

        string op = argv[i];
    
        if( op == "-w" ) {
            i++;
            width = boost::lexical_cast<int>(argv[i]);
        } else if( op == "-h" ) {
            i++;
            height = boost::lexical_cast<int>(argv[i]);
        } else if( op == "-i0" ) {
            i++;
            file0 = argv[i];
        } else if( op == "-i1" ) {
            i++;
            file1 = argv[i];
        } else if( op == "-t" ) {
            i++;
            nthread = 3*boost::lexical_cast<int>(argv[i]);
        } else if( op == "-v" ) {
            showFrame = true;
        }
    }
}

class Fvalue 
{
public:

    Fvalue(size_t n) {
        c0vec.resize(n);
        c1vec.resize(n);
    }

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
    vector<unsigned char> c0vec;
    vector<unsigned char> c1vec;
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
    unsigned int read_unit;
    int count;
    int nframe;
    double totalYMSE;
    double totalUMSE;
    double totalVMSE;
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
{
    opt.parse(argc, argv);
    area = opt.width*opt.height;
    read_unit = area;
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
    double frameYMSE = fydeque.front().f.get();
    fydeque.pop_front();

    double frameUMSE = fudeque.front().f.get();
    fudeque.pop_front();

    double frameVMSE = fvdeque.front().f.get();
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

    nframe++;
}

void Yuv2psnr::run()
{
    cout << "Frame       Y-PSNR[dB]  U-PSNR[dB]  V-PSNR[dB]" << endl;

    while( !fin0.eof() ) {

        Fvalue vf(read_unit);
        auto c0 = (char*)&(vf.c0vec.at(0));
        auto c1 = (char*)&(vf.c1vec.at(0));
        fin0.read(c0, read_unit); 
        fin1.read(c1, read_unit); 

        count += read_unit;

        auto f = async(launch::async, MSE, vf.c0vec, vf.c1vec);
        vf.f = move(f);

        // extract Y
        if( count <= area ) { 
            fydeque.push_back(move(vf));
            read_unit = area/4;

        // extract U
        } else if( area < count && count <= area/4*5 ) {
            fudeque.push_back(move(vf));

        // extract V
        } else if( area/4*5 < count ) {
            fvdeque.push_back(move(vf));
        }

        // end of 1 frame
        if( count == area/2*3 ) {
            count = 0;
            read_unit = area;
        }

        // sync
        while( opt.nthread < fydeque.size() + fudeque.size() + fvdeque.size() ){
            sync_frame();
        }
    }

    // flush
    while( !fvdeque.empty() ){
        sync_frame();
    }

    totalYMSE /= area*nframe;
    totalUMSE /= area/4*nframe;
    totalVMSE /= area/4*nframe;

    auto y = MSE2PSNR(totalYMSE);
    auto u = MSE2PSNR(totalUMSE);
    auto v = MSE2PSNR(totalVMSE);
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
