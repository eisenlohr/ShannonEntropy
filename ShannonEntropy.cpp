#include <format>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <unistd.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
using namespace std;


unsigned int* loadImageAsIntensityArray(const string& filepath, unsigned int pdim[2]) {
    int width, height, channels;
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);
    if (!data) {
        throw runtime_error("Failed to load image: " + filepath);
    }

    pdim[0] = width;  // x dimension (fast index)
    pdim[1] = height; // y dimension (slow index)

    unsigned int* intensityArray = new unsigned int[width * height];

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = (y * width + x);
            int base = idx * channels;

            int intensity = 0;

            if (channels == 1) {
                intensity = static_cast<int>(data[base]);
            } else if (channels >= 3) {
                unsigned char r = data[base + 0];
                unsigned char g = data[base + 1];
                unsigned char b = data[base + 2];

                int red   = r >> 3;
                int green = g >> 3;
                int blue  = b >> 3;

                intensity = (red << 10) | (green << 5) | blue;
            }

            intensityArray[idx] = intensity;
        }
    }

    stbi_image_free(data);  // Free image memory
    return intensityArray;
}


float * nlog2nTable(const int N)
{
    // precalculate (0...N)log2(0...N)
    float * memblock;
    memblock = new float [N+1]();

    for (int i=1 ; i<=N ; i++) memblock[i] = float(i)*log2(float(i));

    return memblock;
}


string strip_ext(string s)
{
    string pathdelim = "/";
    string extdelim = ".";
    unsigned long a = 0U,b;

    while ((b = s.find(pathdelim,a)) != std::string::npos)
        a = b + pathdelim.length();

    return s.substr(0, a+s.substr(a,b).find(extdelim));
}


void write_float(const float image[], unsigned int dim[], const filesystem::path name)
{   // export image content to formatted text file
    ofstream f(name);
    f.precision(10);
    // pretty print image content
    for (unsigned int j=0;j<dim[1];j++){
      for (unsigned int i=0;i<dim[0];i++)
        f << image[j*dim[0]+i] << (i+1==dim[0] ? "" : " ") ;
      f << endl;
    }
    f.close();
}


void write_uint(const unsigned int image[], unsigned int dim[], const filesystem::path name)
{   // export image content to formatted text file
    ofstream f(name);
    // pretty print image content
    for (unsigned int j=0;j<dim[1];j++){
      for (unsigned int i=0;i<dim[0];i++)
        f << image[j*dim[0]+i] << (i+1==dim[0] ? "" : " ") ;
      f << endl;
    }
    f.close();
}


void write_int(const int image[], unsigned int dim[], const filesystem::path name)
{   // export image content to formatted text file
    ofstream f(name);
    // pretty print image content
    for (unsigned int j=0;j<dim[1];j++){
      for (unsigned int i=0;i<dim[0];i++)
        f << image[j*dim[0]+i] << (i+1==dim[0] ? "" : " ") ;
      f << endl;
    }
    f.close();
}


unsigned int * pad(const unsigned int image[], unsigned int dim[], int p)
{   // add padding of zero values around image
    int width  = dim[0] + 2*p;
    int height = dim[1] + 2*p;
    unsigned int * memblock  = new unsigned int [width*height]();

    for (int i=0 ; i<dim[1] ; i++) memcpy(memblock+(i+p)*width+p, image+i*dim[0], dim[0]*sizeof(unsigned int));

    return memblock;
}


unsigned int * unpad(const unsigned int image[], unsigned int dim[], int p)
{   // remove padding of zero values around image
    int width  = dim[0] + 2*p;
    int height = dim[1] + 2*p;
    unsigned int * memblock = new unsigned int [dim[0]*dim[1]]();

    for (int i=0 ; i<dim[1] ; i++) memcpy(memblock+i*dim[0], image+(i+p)*width+p, dim[0]*sizeof(unsigned int));

    return memblock;
}


float * score(const unsigned int field[], unsigned int dim[], int p, bool verbose = false)
{
    // e = - sum_x prob(x) ln(prob(x))
    // prob(x) = n(x)/w^2
    // e = -sum [n/w/w ln(n/w/w)]
    //   = -1/w/w sum [n (ln n - 2 ln w )]
    //   = -1/w/w [sum n ln n - 2 ln w sum n]
    //   = 2 w*w/w/w ln w - 1/w/w sum [n ln n]
    //   = 2 ln w - 1/w/w sum [n ln n]

    const int width  = dim[0] + 2*p;
    const int height = dim[1] + 2*p;
    const int N = dim[0]*dim[1];
    const int w = 1+2*p;                                                        // window size
    unsigned int *hist = new unsigned int [*max_element(field,field+N)+1]();    // histogram of field values
    float *entropy = new float [N]();                                           // initialize entropy field to zero

    unsigned int *padded = pad(field,dim,p);
    float *nlogn = nlog2nTable(w*w);
    float log2ww = log2(w*w);

    int x_,_x, y_,_y;
    int i,j;
    int x = 0,y = 0;
    unsigned int v_,_v;
    float e,e_,_e;
    bool down = false;
    bool right = true;

    hist[0] = w*w - (p+1)*(p+1);                                                // # of zeros in upper and left quadrants (3)
    e = nlogn[hist[0]];

    for (j=p ; j<w ; j++) {                                                     // scan lower...
      for (i=p ; i<w ; i++) {                                                   // ...right quadrant for rest of first pixel entropy
        v_ = padded[j*width+i];
        e += nlogn[hist[v_]+1] - nlogn[hist[v_]];                               // add current and remove former contribution
        hist[v_]++;                                                             // adjust histogram
      }
    }
    entropy[y*dim[0]+x] = -e;

    while (true)
    {
        if (down)
        {
            if (y == height-w) break;                                           // reached bottom of field
            down = false;
            right = !right;                                                     // switch left/right sense with down step
            y_ = y+w;                                                           // index of row to add
            _y = y++;                                                           // index of row to subtract

            for (j=0 ; j<w ; j++)
            {
                _v = padded[_y*width+x+j];
                v_ = padded[y_*width+x+j];
                e -= nlogn[hist[_v]] + nlogn[hist[v_]];                         // remove former contributions
                hist[_v]--;                                                     // adjust histogram
                hist[v_]++;                                                     // adjust histogram
                e += nlogn[hist[_v]] + nlogn[hist[v_]];                         // add current contributions
            }
        }
        else
        {
            x_ = right ? x+w : x-1;                                             // index of column to add
            _x = right ? x   : x-1+w;                                           // index of column to subtract
            x += right ? 1   :  -1;

            down = x==0 || x==width-w;                                          // move down at left or right boundary

            for (i=0 ; i<w ; i++)
            {
                _v = padded[(y+i)*width+_x];
                v_ = padded[(y+i)*width+x_];
                e -= nlogn[hist[_v]] + nlogn[hist[v_]];                         // remove former contributions
                hist[_v]--;                                                     // adjust histogram
                hist[v_]++;                                                     // adjust histogram
                e += nlogn[hist[_v]] + nlogn[hist[v_]];                         // add current contributions
            }
        }
        entropy[y*dim[0]+x] = -e;
    }

    free(padded);
    free(hist);

    for (i=0 ; i<N ; i++)
     // entropy[i] = log2ww+entropy[i]/float(w*w);                              // non-normalized entropy
        entropy[i] = 1.+entropy[i]/float(w*w)/log2ww;                           // normalize entropy value

    if (verbose) {
        _e = *min_element(entropy,entropy+N);
        e_ = *max_element(entropy,entropy+N);
        cout << "entropy range " << _e << " to " << e_ << endl;
    }

    return entropy;
}



int main(int argc, char* argv[])
{
    filesystem::path path, outpath, outdir="";
    unsigned int dim[2];
    unsigned int *image;
    float *scored;
    int i, N, p = 0;
    bool verbose = false;

    int option;

    while ((option = getopt(argc, argv, ":p:o:vh")) != -1) {                    // put ':' at the beginning of the string so compiler can distinguish between '?' and ':'
        switch (option) {
            case 'p':                                                           // one-sided padding
                p = atoi(optarg);
                break;
            case 'o':                                                           // output directory
                outdir = optarg;
                break;
            case 'v':                                                           // verbosity flag
                verbose = true;
                break;
            case 'h':                                                           // help
                printf("Calculate normalized Shannon entropy of grayscale or RGB images.\n\n");

                printf("Usage\n");
                printf("-----\n");
                printf("  %s [options] <image1> <image2> ...\n\n", argv[0]);

                printf("Options\n");
                printf("-------\n");
                printf("  -p padding : single-sided padding of pixel window (default: 4)\n");
                printf("  -o directory : output directory (defaults to parent directory of input file)\n");
                printf("  -v : verbose mode (prints additional information)\n");
                printf("  -h : display this help and exit\n\n");

                printf("Example\n");
                printf("-------\n");
                printf("  %s -p 4 -o output_dir image1.png image2.jpg\n\n", argv[0]);
                printf("  This will calculate the normalized Shannon entropy for each input image\n");
                printf("  with a padding of 4 pixels and save the results in 'output_dir'.\n");
                printf("  The output files will be named 'image1_entropy_9x9.txt' and 'image2_entropy_9x9.txt',\n");
                printf("  where '9x9' corresponds to the window size of 1+2*padding (in this case, 1+2*4 = 9).\n\n");

                printf("Notes\n");
                printf("-----\n");
                printf("  The input images should be in a format supported by stb_image.h, such as PNG or JPEG.\n");
                printf("  For grayscale images, the pixel values are directly interpreted as intensity values.\n");
                printf("  For RGB images, a 15-bit grayscale value is calculated by stacking each channel's\n");
                printf("  most-significant 5 bits.\n\n");
                printf("  The output files will contain the normalized Shannon entropy values for each pixel,\n");
                printf("  where 0 corresponds to uniform distribution and 1 corresponds to maximum entropy.\n");
                printf("  The output format is a space-separated text file with one line per row of pixels.\n\n");
                return 0;
            case ':':
                printf("option '-%c' requires a value\n", optopt);
                break;
            case '?': // used for any unknown options
                printf("option '-%c' unknown\n", optopt);
                break;
        }
    }

    p = p == 0 ? 4 : p;                                                         // default window padding

    if (verbose) cout << "padding: " << p << endl;
    for (; optind < argc; optind++){                                            // loop over input files
        path = filesystem::path(argv[optind]);
        outpath = filesystem::path(path.stem().string()+format("_entropy_{}x{}.txt",1+2*p,1+2*p));
        image = loadImageAsIntensityArray(argv[optind], dim);                   // load image
        scored = score(image,dim,p,verbose);
        write_float(
            scored,
            dim,
            (outdir == "" ? path.parent_path() : outdir)/outpath
        );
        free(image);
        free(scored);
    }
    return 0;
}
