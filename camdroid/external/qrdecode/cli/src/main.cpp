// -*- mode:c++; tab-width:2; indent-tabs-mode:nil; c-basic-offset:2 -*-
/*
 *  Copyright 2010-2011 ZXing authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <fstream>
#include <string>

#ifdef ZXING
#include "ImageReaderSource.h"
#include <zxing/common/Counted.h>
#include <zxing/Binarizer.h>
#include <zxing/MultiFormatReader.h>
#include <zxing/Result.h>
#include <zxing/ReaderException.h>
#include <zxing/common/GlobalHistogramBinarizer.h>
#include <zxing/common/HybridBinarizer.h>
#include <exception>
#include <zxing/Exception.h>
#include <zxing/common/IllegalArgumentException.h>
#include <zxing/BinaryBitmap.h>
#include <zxing/DecodeHints.h>
#include <zxing/qrcode/QRCodeReader.h>
#include <zxing/multi/qrcode/QRCodeMultiReader.h>
#include <zxing/multi/ByQuadrantReader.h>
#include <zxing/multi/MultipleBarcodeReader.h>
#include <zxing/multi/GenericMultipleBarcodeReader.h>
#include "BufferBitmapSource.h"
#endif

#include "mybmp.h"

#include <qrdecode_interface.h>

#ifdef ZXING
using namespace std;
using namespace zxing;
using namespace zxing::multi;
using namespace zxing::qrcode;

namespace {

bool more = false;
bool test_mode = false;
bool try_harder = false;
bool search_multi = false;
bool use_hybrid = false;
bool use_global = false;
bool verbose = false;

}


vector<Ref<Result> > decode(Ref<BinaryBitmap> image, DecodeHints hints) {
  Ref<Reader> reader(new MultiFormatReader);
  return vector<Ref<Result> >(1, reader->decode(image, hints));
}

vector<Ref<Result> > decode_multi(Ref<BinaryBitmap> image, DecodeHints hints) {
  MultiFormatReader delegate;
  GenericMultipleBarcodeReader reader(delegate);
  return reader.decodeMultiple(image, hints);
}

int read_image(Ref<LuminanceSource> source, bool hybrid, string expected) {
  vector<Ref<Result> > results;
  string cell_result;
  int res = -1;

  try {
    Ref<Binarizer> binarizer;
    if (hybrid) {
      binarizer = new HybridBinarizer(source);
    } else {
      binarizer = new GlobalHistogramBinarizer(source);
    }
    DecodeHints hints(DecodeHints::DEFAULT_HINT);
    hints.setTryHarder(try_harder);
    Ref<BinaryBitmap> binary(new BinaryBitmap(binarizer));
    if (search_multi) {
      results = decode_multi(binary, hints);
    } else {
      results = decode(binary, hints);
    }
    res = 0;
  } catch (const ReaderException& e) {
    cell_result = "zxing::ReaderException: " + string(e.what());
    res = -2;
  } catch (const zxing::IllegalArgumentException& e) {
    cell_result = "zxing::IllegalArgumentException: " + string(e.what());
    res = -3;
  } catch (const zxing::Exception& e) {
    cell_result = "zxing::Exception: " + string(e.what());
    res = -4;
  } catch (const std::exception& e) {
    cell_result = "std::exception: " + string(e.what());
    res = -5;
  }

  if (test_mode && results.size() == 1) {
    std::string result = results[0]->getText()->getText();
    if (expected.empty()) {
      cout << "  Expected text or binary data for image missing." << endl
           << "  Detected: " << result << endl;
      res = -6;
    } else {
      if (expected.compare(result) != 0) {
        cout << "  Expected: " << expected << endl
             << "  Detected: " << result << endl;
        cell_result = "data did not match";
        res = -6;
      }
    }
  }

  if (res != 0 && (verbose || (use_global ^ use_hybrid))) {
    cout << (hybrid ? "Hybrid" : "Global")
         << " binarizer failed: " << cell_result << endl;
  } else if (!test_mode) {
    if (verbose) {
      cout << (hybrid ? "Hybrid" : "Global")
           << " binarizer succeeded: " << endl;
    }
    for (size_t i = 0; i < results.size(); i++) {
      if (more) {
        cout << "  Format: "
             << BarcodeFormat::barcodeFormatNames[results[i]->getBarcodeFormat()]
             << endl;
        for (int j = 0; j < results[i]->getResultPoints()->size(); j++) {
          cout << "  Point[" << j <<  "]: "
               << results[i]->getResultPoints()[j]->getX() << " "
               << results[i]->getResultPoints()[j]->getY() << endl;
        }
      }
      if (verbose) {
        cout << "    ";
      }
      cout << results[i]->getText()->getText() << endl;
    }
  }

  return res;
}

string read_expected(string imagefilename) {
  string textfilename = imagefilename;
  string::size_type dotpos = textfilename.rfind(".");

  textfilename.replace(dotpos + 1, textfilename.length() - dotpos - 1, "txt");
  ifstream textfile(textfilename.c_str(), ios::binary);
  textfilename.replace(dotpos + 1, textfilename.length() - dotpos - 1, "bin");
  ifstream binfile(textfilename.c_str(), ios::binary);
  ifstream *file = 0;
  if (textfile.is_open()) {
    file = &textfile;
  } else if (binfile.is_open()) {
    file = &binfile;
  } else {
    return std::string();
  }
  file->seekg(0, ios_base::end);
  size_t size = size_t(file->tellg());
  file->seekg(0, ios_base::beg);

  if (size == 0) {
    return std::string();
  }

  char* data = new char[size + 1];
  file->read(data, size);
  data[size] = '\0';
  string expected(data);
  delete[] data;

  return expected;
}
#endif

int main(int argc, char** argv) {
  if (argc != 3) {
    printf("need filename and scale\n");
    return 1;
  }
#ifdef ZXING
  int total = 0;
  int gonly = 0;
  int honly = 0;
  int both = 0;
  int neither = 0;

    string filename = argv[1];
    if (filename.compare("--verbose") == 0 ||
        filename.compare("-v") == 0) {
      verbose = true;
      continue;
    }
    if (filename.compare("--hybrid") == 0 ||
        filename.compare("-h") == 0) {
      use_hybrid = true;
      continue;
    }
    if (filename.compare("--global") == 0 ||
        filename.compare("-g") == 0) {
      use_global = true;
      continue;
    }
    if (filename.compare("--more") == 0) {
      more = true;
      continue;
    }
    if (filename.compare("--test-mode") == 0) {
      test_mode = true;
      continue;
    }
    if (filename.compare("--try-harder") == 0) {
      try_harder = true;
      continue;
    }
    if (filename.compare("--search-multi") == 0){
      search_multi = true;
      continue;
    }

    if (filename.length() > 3 &&
        (filename.substr(filename.length() - 3, 3).compare("txt") == 0 ||
         filename.substr(filename.length() - 3, 3).compare("bin") == 0)) {
      continue;
    }
#endif    
	if (strstr(argv[1], ".bmp")!=NULL || strstr(argv[1], ".Bmp")!=NULL || strstr(argv[1], ".BMP")!=NULL) {
        ClImage* img = clLoadImage(argv[1]);
        int width = img->width;
        int height = img->height;
        double scale = atof(argv[2]);
        //int scale = (img->width/300)<(img->height/300)?(img->width/300):(img->height/300);
        //scale = scale>1?scale:1;
        width /= scale;
        height/= scale;
        char* raw = (char*)malloc(width*height);
        if (img->channels == 1) {
            for (int i=0; i<height; i++)
                for (int j=0; j<width; j++) { 
					int idx = img->channels*i*img->width*scale+img->channels*j*scale;
					raw[i*width+j]=img->imageData[idx];
            }
		}
        else {
            for (int i=0; i<height; i++)
                for (int j=0; j<width; j++) {
                    int idx = img->channels*i*img->width*scale+img->channels*j*scale;
                    raw[i*width+j]=(img->imageData[idx+2]*299+img->imageData[idx+1]*587+img->imageData[idx]*114+500)/1000;
                }
        }
		ClImage test;
		test.width = width;
		test.height = height;
		test.channels = 1;
		test.imageData = (unsigned char*)raw;
//		clSaveImage("/mnt/extsd/9.bmp", &test);
        printf("width(%d), height(%d), img->channels(%d)\n", width, height, img->channels);
		char zbar_result[256]="";
        int zbar_result_cnt = zbar_dec(width, height, raw, zbar_result, 256);
		printf("result cnt %d", zbar_result_cnt);
		for (int i = 0; i < zbar_result_cnt; i++) {
			char* tmp = zbar_result;
			printf("result (%d) (%s)\n", i, zbar_result);
			tmp += strlen(tmp)+sizeof('\0');
		}
		free(raw);
#ifdef ZXING
	     try
       {
				unsigned char* zxing_raw =  (unsigned char*)malloc(width*height);
				if (img->channels == 1) {
					for (int i=0; i<height; i++)
						for (int j=0; j<width; j++) {
							int idx = img->channels*i*img->width*scale+img->channels*j*scale;
							zxing_raw[i*width+j]=img->imageData[idx];
					}
				}
				else {
					for (int i=0; i<height; i++)
						for (int j=0; j<width; j++) {
							int idx = img->channels*i*img->width*scale+img->channels*j*scale;
							zxing_raw[i*width+j]=(img->imageData[idx+2]*299+img->imageData[idx+1]*587+img->imageData[idx]*114+500)/1000;
						}
				}
	        	// Convert the buffer to something that the library understands. 
         		Ref<LuminanceSource> source (new BufferBitmapSource(width, height, (unsigned char*)zxing_raw));

        		// Turn it into a binary image. 
        		Ref<Binarizer> binarizer (new GlobalHistogramBinarizer(source)); 
        		Ref<BinaryBitmap> image(new BinaryBitmap(binarizer));

        		// Tell the decoder to try as hard as possible. 
        		DecodeHints hints(DecodeHints::DEFAULT_HINT); 
	        	hints.setTryHarder(true);

	        	// Perform the decoding. 
		        QRCodeReader reader;
		        Ref<Result> result(reader.decode(image, hints));

		        // Output the result. 
		        cout << result->getText()->getText() << endl;

	      } 
	      catch (zxing::Exception& e) 
	      {
		        cerr << "Error: " << e.what() << endl;
	      }
#endif
        free(img->imageData);
        free(img);
        
		return 0;
	}
#ifdef ZXING
    if (!use_global && !use_hybrid) {
      use_global = use_hybrid = true;
    }

    if (test_mode) {
      cerr << "Testing: " << filename << endl;
    }

    Ref<LuminanceSource> source;
    try {
      source = ImageReaderSource::create(filename);
    } catch (const zxing::IllegalArgumentException &e) {
      cerr << e.what() << " (ignoring)" << endl;
      continue;
    }

    string expected = read_expected(filename);

    int gresult = 1;
    int hresult = 1;
    if (use_hybrid) {
      hresult = read_image(source, true, expected);
    }
    if (use_global && (verbose || hresult != 0)) {
      gresult = read_image(source, false, expected);
      if (!verbose && gresult != 0) {
        cout << "decoding failed" << endl;
      }
    }
    gresult = gresult == 0;
    hresult = hresult == 0;
    gonly += gresult && !hresult;
    honly += hresult && !gresult;
    both += gresult && hresult;
    neither += !gresult && !hresult;
    total = total + 1;
  

  if (test_mode) {
    cout << endl
         << "Summary:" << endl
         << " " << total << " images tested total," << endl
         << " " << (honly + both)  << " passed hybrid, " << (gonly + both)
         << " passed global, " << both << " pass both, " << endl
         << " " << honly << " passed only hybrid, " << gonly
         << " passed only global, " << neither << " pass neither." << endl;
  }
#endif
  getchar();
  return 0;
}
