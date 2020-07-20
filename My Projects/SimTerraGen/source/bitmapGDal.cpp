//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

//Compilador en D:\ACART\2020\Downloads\gdal-2.4.4

#include "./gcore/gdal_priv.h"
#include <stdexcept>
#include <sstream>
#include <iostream>

#include "./port/cpl_conv.h" // for CPLMalloc()

#include "core/stream/stream.h"

#include "gfx/bitmap/gBitmap.h"

#include "core/stream/fileStream.h"

#include "console/console.h"
#include "terrain/terrFile.h"

void CPL_STDCALL HobusGDALErrorHandler(CPLErr eErrClass, int err_no, const char *msg);

 void CPL_STDCALL HobusGDALErrorHandler(CPLErr eErrClass, int err_no, const char* msg)
 {
    std::ostringstream oss;

    if (eErrClass == CE_Failure || eErrClass == CE_Fatal)
    {
       oss << "GDAL Failure number=" << err_no << ": " << msg;
    }
    else if (eErrClass == CE_Debug)
    {
       std::cout << "GDAL Debug: " << msg << std::endl;
    }
    else
    {
       return;
    }
 }


static bool sReadGDal(Stream &stream, GBitmap *bitmap);
static bool sWriteDGal(GBitmap *bitmap, Stream &stream, U32 compressionLevel);

static struct _privateRegisterGDal
{
   _privateRegisterGDal()
   {
      GBitmap::Registration reg;

      //reg.extensions.push_back( "tiff" );
      reg.extensions.push_back("tif");
      //reg.extensions.push_back( "terr" );

      reg.readFunc = sReadGDal;
      reg.writeFunc = sWriteDGal;

      GBitmap::sRegisterFormat( reg );

      GDALAllRegister();
   }
} sStaticRegisterGDal;



//--------------------------------------
static S32 gdalFlushDataFn(void *)
{
   // do nothing since we can't flush the stream object
   return 0;
}


//--------------------------------------
static S32 gdalErrorFn(void *client_data)
{
   Stream *stream = (Stream*)client_data;
   AssertFatal(stream != NULL, "gdalErrorFn::No stream.");
   return (stream->getStatus() != Stream::Ok);
}


static GDALDataset* crop(GDALDataset* outDS, GDALDataset* inDS, int xoff, int yoff, int xsize, int ysize)
{
   const int readRows = 512;
   const int readCols = 512;
   int nBlockXSize;
   int nBlockYSize;
   int rowOff = 0;
   int colOff = 0;
   int nRows = 0;
   int nCols = 0;
   double noData = -9999;

   // Here I also copy metadata from my own domain
   GDALRasterBand* poSrcBand1 = inDS->GetRasterBand(1);
   GDALDataType eBandType1 = poSrcBand1->GetRasterDataType();
   const U8 DataTypesize[] = {1,1,2,2,4,4,4,8,2,4,8};
   S32 nBytesPerBand = DataTypesize[eBandType1];

   U8 *abyRaster = (U8 *)CPLMalloc(readRows*readCols*nBytesPerBand);


   for (int iband = 0; iband < inDS->GetRasterCount(); ++iband)
   {
      GDALRasterBand* poSrcBand = inDS->GetRasterBand(iband + 1);
      GDALDataType eBandType = poSrcBand->GetRasterDataType();

      nCols = poSrcBand->GetXSize();
      nRows = poSrcBand->GetYSize();
      noData = poSrcBand->GetNoDataValue();

      int nXBlocks = (poSrcBand->GetXSize() + readCols - 1) / readCols;
      int nYBlocks = (poSrcBand->GetYSize() + readRows - 1) / readRows;

      for (int i = 0; i < nYBlocks; i++)
      {
         for (int j = 0; j < nXBlocks; j++)
         {
            int nXSize = getMin(readCols, poSrcBand->GetXSize() - j * readCols);
            int nYSize = getMin(readRows, poSrcBand->GetYSize() - i * readRows);


            poSrcBand->RasterIO(GF_Read, j * readCols, i * readRows, readCols, readRows,
               abyRaster, nXSize, nYSize, eBandType, 0, 0);

            GDALRasterBand* poVRTBand = outDS->GetRasterBand(iband + 1);
            poVRTBand->RasterIO(GF_Write, j * readCols, i * readRows, readCols, readRows,
               abyRaster, nXSize, nYSize, eBandType, 0, 0);
         }
      }
   }

   CPLFree(abyRaster);

   return outDS;
}



static void GetGeoRef(GBitmap * bitmap, double * adfGeoTransform, int nXSize, int nYSize, double min, double max)
{
   bitmap->sGeoRef.topLeftX = adfGeoTransform[0];
   bitmap->sGeoRef.topLeftY = adfGeoTransform[3];
   bitmap->sGeoRef.pixelResolX = adfGeoTransform[1];
   bitmap->sGeoRef.pixelResolY = adfGeoTransform[5];
   bitmap->sGeoRef.nXSize = nXSize;
   bitmap->sGeoRef.nYSize = nYSize;
   bitmap->sGeoRef.minimum = min;
   bitmap->sGeoRef.maximum = max;
   bitmap->sGeoRef.defined = true;
}

//--------------------------------------
static bool sReadGDal(Stream &stream, GBitmap *bitmap)
{
   GDALDriver *pDriverTiff;
   double transform[6];

   GDALAllRegister();
   
   const Torque::FS::FileRef fref = static_cast<FileStream*>(&stream)->getFileRef();
   Torque::Path path = fref->getName();

   // Write out the actual Collada file
   char inFileName[4096];

   String file = (path.getRoot().compare("game", 0) == 0) ? path.getFullPathWithoutRoot() : path.getFullPath();

   Platform::makeFullPathName(file, inFileName, 4096);

   GDALDataset *preadDS = (GDALDataset *)GDALOpen(inFileName, GA_ReadOnly);
   if (preadDS == NULL)
      return false;

    
   #pragma region GeoRef    
   int             nBlockXSize, nBlockYSize;
   int             bGotMin, bGotMax;
   double          adfMinMax[2];
    
   GDALDataset::Bands bands = preadDS->GetBands();
   S32 nRasterCount = preadDS->GetRasterCount();
    
   GDALRasterBand *poBand = preadDS->GetRasterBand(1);
   GDALDataType gddt = poBand->GetRasterDataType();
   GDALColorInterp gdci = poBand->GetColorInterpretation();


   int   nXSize = poBand->GetXSize();
   int   nYSize = poBand->GetYSize();
   poBand->DeleteNoDataValue();
   double nodataval = poBand->GetNoDataValue();
   poBand->SetNoDataValue(0.0);
   //double min = poBand->GetMinimum();
   //double max = poBand->GetMaximum();

   double        adfGeoTransform[6];
   if (preadDS->GetGeoTransform(adfGeoTransform) == CE_None)
   {
      Con::printf("Origin = (%.6f,%.6f)\n",
         adfGeoTransform[0], adfGeoTransform[3]);
      Con::printf("Pixel Size = (%.6f,%.6f)\n",
         adfGeoTransform[1], adfGeoTransform[5]);
   }
   
#pragma endregion GeoRef

   F32 min = 50000;
   F32 max = -20000;

   if (nRasterCount==1 && gddt == GDT_Float32)
   {
      // actually allocate the bitmap space...
      bitmap->allocateBitmap(nXSize, nYSize,
         false,            // don't extrude miplevels...
         GFXFormatR5G5B5A1);          // use 16bit format...

      F32 *pafScanline = static_cast<float *>(CPLMalloc(sizeof(F32) * nXSize));

      for (int i = 0; i < nYSize; i++)
      {
         poBand->RasterIO(GF_Read, 0, i, nXSize, 1,
            pafScanline, nXSize, 1, GDT_Float32,
            0, 0);

         U8 *rowDest = bitmap->getAddress(0, i);

         for (S32 j = 0; j < nXSize; j++)
         {
            U16 * pixelLocation = reinterpret_cast<U16 *>(&rowDest[j * 2]);
            F32 height = pafScanline[j];

            if (height > 50000 || height < -20000)
            {
               pixelLocation[0] = floatToFixed(0.0f);
            }
            else
            {
               min = getMin(min, height);
               max = getMax(max, height);

               pixelLocation[0] = floatToFixed(height);
            }

         }
      }

      CPLFree(pafScanline);

      GetGeoRef(bitmap, adfGeoTransform, nXSize, nYSize, min, max);

      sprintf_s(bitmap->sGeoRef.driver, 128, "%s/t%s",
         preadDS->GetDriver()->GetDescription(),
         preadDS->GetDriver()->GetMetadataItem(GDAL_DMD_LONGNAME));

      sprintf_s(bitmap->sGeoRef.size, 512, "%d %d %d",
         preadDS->GetRasterXSize(), preadDS->GetRasterYSize(),
         preadDS->GetRasterCount());

      sprintf_s(bitmap->sGeoRef.projection, 512, "%s",
         preadDS->GetProjectionRef() == NULL ? "0" : preadDS->GetProjectionRef());

      GDALClose(preadDS);

      // We know TIFF's don't have any transparency
      bitmap->setHasTransparency(false);
   } 
   else
   {  
      min = poBand->GetMinimum();
      max = poBand->GetMaximum();
      //Carga TIFF imagen mediante trasformación a PNG
      GDALDriver *poDriver;
      char **papszMetadata;
      poDriver = GetGDALDriverManager()->GetDriverByName("PNG");
      if (poDriver == NULL)
         return false;

      GDALDataset* pngDS = poDriver->CreateCopy("./dummy.png", preadDS, FALSE,
         NULL, NULL, NULL);
      /* Once we're done, close properly the dataset */
      /* Once we're done, close properly the dataset */
      if (preadDS != NULL)
         GDALClose((GDALDatasetH)preadDS);

      if (pngDS != NULL)
         GDALClose((GDALDatasetH)pngDS);

      FileStream  streamPng;

      streamPng.open("./dummy.png", Torque::FS::File::Read);

      if (streamPng.getStatus() != Stream::Ok)
      {
         Con::errorf("Resource<GBitmap>::create - failed to open '%s'", "./dummy.png");
         return NULL;
      }

      //       GBitmap *bmp = new GBitmap;

      if (!bitmap->readBitmap("png", streamPng))
      {
         //Con::errorf("Resource<GBitmap>::create - error reading '%s'", path.getFullPath().c_str());
         //delete bitmap;
         return false;
      }


      //    GDALDriver *poDriver;
      //    char **papszMetadata;
      //    poDriver = GetGDALDriverManager()->GetDriverByName("MEM");
      //    if (poDriver == NULL)
      //       return false;
      //    char **cropOptions = NULL;
      // 
      //    GDALDataset* poDstDS = poDriver->Create("./dummy.dat", bitmap->getWidth(), bitmap->getHeight(), 1, GDT_Byte, cropOptions);
      // 
      //    GDALRasterBand* band_out = poDstDS->GetRasterBand(1);
      //    U8 *bitmapbuff = bitmap->getAddress(0, 0);
      //    band_out->RasterIO(GF_Write, 0, 0, bitmap->getWidth(), bitmap->getHeight(), bitmapbuff, bitmap->getWidth(), bitmap->getHeight(), GDT_Byte, 0, 0);
      // 

#pragma region cambio de tamaño
      S32 width = preadDS->GetRasterXSize();
      S32 height = preadDS->GetRasterYSize();

      //    if (width != height || !isPow2(width))
      //    {
      //       printf("Height map must be square and power of two in size!");
      //       //return false;
      // 
      //       S32 maxdim = getMax(width, height);
      //       if (!isPow2(maxdim))
      //       {
      //          //Rounding up to next power of 2
      //          int power = 1;
      //          while (power < maxdim)
      //             power *= 2;
      // 
      //          maxdim = power;
      //       }
      // 
      //       width = maxdim;
      //       height = maxdim;
      //    }

      //    bitmap->deleteImage();
      // 
      //    // actually allocate the bitmap space...
      //    bitmap->allocateBitmap(width, height,
      //       false,            // don't extrude miplevels...
      //       format);          // use determined format...

      // 
      // #pragma endregion cambio de tamaño
      // 
      //    GDALDriver *poDriver;
      //    //char **papszMetadata;
      //    poDriver = GetGDALDriverManager()->GetDriverByName("BMP");
      //    if (poDriver == NULL)
      //       return false;
      // 
      //    //create the new (cropped) dataset
      //    //pCroppedRaster = pDriver->Create(cropPath, xSize, ySize, 1, GDT_Float32, NULL) //or something similar
      //    GDALDataset* pngDS = poDriver->Create("./dummy.bmp", width, height, nRasterCount, gddt, NULL);
      // 
      //    crop(pngDS, preadDS, 0, 0, 0, 0);
      // 
      //    //GDALDataset* pngDS = poDriver->CreateCopy("./dummy.png", preadDS, FALSE,
      //    //   NULL, NULL, NULL);
      //    /* Once we're done, close properly the dataset */
      //    /* Once we're done, close properly the dataset */
      //    if (preadDS != NULL)
      //       GDALClose((GDALDatasetH)preadDS);
      // 
      //    if (pngDS != NULL)
      //       GDALClose((GDALDatasetH)pngDS);
      // 
      //    
      // //    FileStream  streamPng;
      // // 
      // //    streamPng.open("./dummy.bmp", Torque::FS::File::Read);
      // // 
      // //    if (streamPng.getStatus() != Stream::Ok)
      // //    {
      // //       Con::errorf("Resource<GBitmap>::create - failed to open '%s'", "./dummy.png");
      // //       return NULL;
      // //    }
      // 
      // //    //       GBitmap *bmp = new GBitmap;
      // // 
      // //    if (!bitmap->readBitmap("bmp", streamPng))
      // //    {
      // //       //Con::errorf("Resource<GBitmap>::create - error reading '%s'", path.getFullPath().c_str());
      // //       //delete bitmap;
      // //       return false;
      // //    }
      // 
      // 
      // 
      // 
      //    /*
      // 

      GetGeoRef(bitmap, adfGeoTransform, nXSize, nYSize, min, max);
   }

   
   
   return true;

}


//--------------------------------------------------------------------------
static bool sWriteDGal(GBitmap *bitmap, Stream &stream, U32 compressionLevel)
{
   TORQUE_UNUSED(compressionLevel); // compression level not currently hooked up

   GFXFormat   format = bitmap->getFormat();

   // JPEG format does not support transparency so any image
   // in Alpha format should be saved as a grayscale which coincides
   // with how the readJPEG function will read-in a JPEG. So the
   // only formats supported are RGB and Alpha, not RGBA.
   AssertFatal(format == GFXFormatR8G8B8 || format == GFXFormatA8,
	            "GBitmap::writeJPEG: ONLY RGB bitmap writing supported at this time.");
   if (format != GFXFormatR8G8B8 && format != GFXFormatA8)
      return false;

   // maximum image size allowed
   #define MAX_HEIGHT 4096
   if (bitmap->getHeight() > MAX_HEIGHT)
      return false;
   /*
   // Bind our own stream writing, error, and memory flush functions
   // to the gdal library interface
   JFWRITE = gdalWriteDataFn;
   JFFLUSH = gdalFlushDataFn;
   JFERROR = gdalErrorFn;

   // Allocate and initialize our gdal compression structure and error manager
   gdal_compress_struct cinfo;
   gdal_error_mgr jerr;

   cinfo.err = gdal_std_error(&jerr);    // set up the normal JPEG error routines.
   cinfo.client_data = (void*)&stream;   // set the stream into the client_data
   gdal_create_compress(&cinfo);         // allocates a small amount of memory

   // specify the destination for the compressed data(our stream)
   gdal_stdio_dest(&cinfo);

   // set the image properties
   cinfo.image_width = bitmap->getWidth();           // image width
   cinfo.image_height = bitmap->getHeight();         // image height
   cinfo.input_components = bitmap->getBytesPerPixel();   // samples per pixel(RGB:3, Alpha:1)
   
   switch (format)
   {
      case GFXFormatA8:  // no alpha support in JPEG format, so turn it into a grayscale
         cinfo.in_color_space = JCS_GRAYSCALE;
         break;
      case GFXFormatR8G8B8:    // otherwise we are writing in RGB format
         cinfo.in_color_space = JCS_RGB;
         break;
      default:
         AssertFatal( false, "Format not handled in GBitmap::writeJPEG() switch" );
         break;
   }
   // use default compression params(75% compression)
   gdal_set_defaults(&cinfo);

   // begin JPEG compression cycle
   gdal_start_compress(&cinfo, true);

   // Set up the row pointers...
   U32 rowBytes = cinfo.image_width * cinfo.input_components;

   U8* pBase = (U8*)bitmap->getBits();
   for (U32 i = 0; i < bitmap->getHeight(); i++)
   {
      // write the image data
      JSAMPROW rowPointer = pBase + (i * rowBytes);
	   gdal_write_scanlines(&cinfo, &rowPointer, 1);
   }

   // complete the compression cycle
   gdal_finish_compress(&cinfo);

   // release the JPEG compression object
   gdal_destroy_compress(&cinfo);

   */

   // return success
   return true;
}
