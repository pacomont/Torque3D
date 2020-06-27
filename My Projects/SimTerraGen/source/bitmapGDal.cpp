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


//--------------------------------------
static bool sReadGDal(Stream &stream, GBitmap *bitmap)
{
   GDALDriver *pDriverTiff;
   double transform[6];

   FileStream  *newStream = new FileStream;

   const char *inFileName = "./dummy.dat";

   bool success = newStream->open(inFileName, Torque::FS::File::Write);

   if (!success)
   {
      delete newStream;
      return false;
   }

   newStream->copyFrom(&stream);   

   newStream->close();
   delete newStream;

   GDALDataset *preadDS = (GDALDataset *) GDALOpen(inFileName, GA_ReadOnly);
   if (preadDS == NULL)
   {
      return false;
   }

   


   double        adfGeoTransform[6];
   Con::printf("Driver: %s/%s\n",
      preadDS->GetDriver()->GetDescription(),
      preadDS->GetDriver()->GetMetadataItem(GDAL_DMD_LONGNAME));
   Con::printf("Size is %dx%dx%d\n",
      preadDS->GetRasterXSize(), preadDS->GetRasterYSize(),
      preadDS->GetRasterCount());
   if (preadDS->GetProjectionRef() != NULL)
      Con::printf("Projection is `%s'\n", preadDS->GetProjectionRef());
   if (preadDS->GetGeoTransform(adfGeoTransform) == CE_None)
   {
      Con::printf("Origin = (%.6f,%.6f)\n",
         adfGeoTransform[0], adfGeoTransform[3]);
      Con::printf("Pixel Size = (%.6f,%.6f)\n",
         adfGeoTransform[1], adfGeoTransform[5]);
   }


   //adfGeoTransform[0] /* top left x */
   //adfGeoTransform[1] /* w-e pixel resolution */
   //adfGeoTransform[2] /* 0 */
   //adfGeoTransform[3] /* top left y */
   //adfGeoTransform[4] /* 0 */
   //adfGeoTransform[5] /* n-s pixel resolution (negative value) */

   S32 width = preadDS->GetRasterXSize();
   S32 height = preadDS->GetRasterYSize();

   int             nBlockXSize, nBlockYSize;
   int             bGotMin, bGotMax;
   double          adfMinMax[2];

   GDALDataset::Bands bands = preadDS->GetBands();
   S32 nRasterCount = preadDS->GetRasterCount();

   GDALRasterBand *poBand = preadDS->GetRasterBand(1);
   GDALDataType gddt = poBand->GetRasterDataType();
   GDALColorInterp gdci = poBand->GetColorInterpretation();

   GFXFormat format = GFXFormatR8G8B8;
   S32 bytes_detph = 1;  
   GDALColorTable* ct = NULL;
   

   if (poBand->GetColorTable() != NULL)
   {
      Con::printf("Band has a color table with %d entries.\n",
         poBand->GetColorTable()->GetColorEntryCount());
      ct = poBand->GetColorTable();

      Con::printf("PaletteInterpretation returning: %d", ct->GetPaletteInterpretation());
   }

   bool convertToPng = true;

   if (nRasterCount == 1)
   {
      if (gddt == GDT_Byte)
      {
         if (ct == NULL)
         {
            format = GFXFormatA8;
            bytes_detph = 1;

            convertToPng = false;
         }
         else
         {
            format = GFXFormatR8G8B8;
            bytes_detph = 3;
            convertToPng = true;
         }         
      }
      else if (gddt == GDT_UInt16 || gddt == GDT_Int16)
      {
         format = GFXFormatR5G6B5;
         bytes_detph = 2;
         convertToPng = false;
      }
      else
      {
         format = GFXFormatR32F;
         bytes_detph = 4;
         convertToPng = false;
      }
   }
   else if (nRasterCount == 3)
   {
      convertToPng = true;
      if(gddt == GDT_Byte)
      {
         format = GFXFormatR8G8B8;
         bytes_detph = 1;
      }
   }
   else if (nRasterCount == 4)
   {
      convertToPng = true;
      if (gddt == GDT_Byte)
      {
         format = GFXFormatR8G8B8A8;
         bytes_detph = 1;
      }
      else if(gddt == GDT_UInt16)
      {
         format = GFXFormatR16G16B16A16;
         bytes_detph = 2;
      }
   }
   else
   {
      convertToPng = true;
      Con::errorf("Error: Formato no soportado.");
   }


      int   nXSize = poBand->GetXSize();
      int   nYSize = poBand->GetYSize();
      double min = poBand->GetMinimum();
      double max = poBand->GetMaximum();

#pragma region GeoRef
      bitmap->sGeoRef.topLeftX = adfGeoTransform[0];
      bitmap->sGeoRef.topLeftY = adfGeoTransform[3];
      bitmap->sGeoRef.pixelResolX = adfGeoTransform[1];
      bitmap->sGeoRef.pixelResolY = adfGeoTransform[5];
      bitmap->sGeoRef.nXSize = nXSize;
      bitmap->sGeoRef.nYSize = nYSize;
      bitmap->sGeoRef.minimum = min;
      bitmap->sGeoRef.maximum = max;
      bitmap->sGeoRef.defined = true;
#pragma endregion GeoRef

#pragma region cambio de tama�o
      if (width != height || !isPow2(width))
      {
         printf("Height map must be square and power of two in size!");
         //return false;

         S32 maxdim = getMax(width, height);
         if (!isPow2(maxdim))
         {
            //Rounding up to next power of 2
            int power = 1;
            while (power < maxdim)
               power *= 2;

            maxdim = power;
         }

         width = maxdim;
         height = maxdim;
      }
#pragma endregion cambio de tama�o

   // allocate the bitmap space and init internal variables...

   if(convertToPng)
   {      
      GDALDriver *poDriver;
      char **papszMetadata;
      poDriver = GetGDALDriverManager()->GetDriverByName("PNG");
      if (poDriver == NULL)
         return false;
     
      GDALDataset* poDstDS = poDriver->CreateCopy("./dummy.png", preadDS, FALSE,
         NULL, NULL, NULL);
      /* Once we're done, close properly the dataset */
      if (poDstDS != NULL)
         GDALClose((GDALDatasetH)poDstDS);
      GDALClose((GDALDatasetH)preadDS);

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
         //delete bmp;
         return false;
         bitmap = NULL;
      }

      bitmap->setHasTransparency(false);
      return true;
   }


   bitmap->allocateBitmap(width, height, false, format); //32 bit float
   U8 *pafScanline = static_cast<U8*>(CPLMalloc(bytes_detph * nXSize));

   for (S32 irast = 1; irast <= nRasterCount; irast++)
   {
      GDALRasterBand *poBand = preadDS->GetRasterBand(irast);
      
      S32 nChannel = irast - 1;
      
      GDALDataType gddt = poBand->GetRasterDataType();
      GDALColorInterp gdci = poBand->GetColorInterpretation();

      
#pragma region Vervose
      poBand->GetBlockSize(&nBlockXSize, &nBlockYSize);
      Con::printf("Block=%dx%d Type=%s, ColorInterp=%s\n",
         nBlockXSize, nBlockYSize,
         GDALGetDataTypeName(poBand->GetRasterDataType()),
         GDALGetColorInterpretationName(
            poBand->GetColorInterpretation()));
      adfMinMax[0] = poBand->GetMinimum(&bGotMin);
      adfMinMax[1] = poBand->GetMaximum(&bGotMax);
      if (!(bGotMin && bGotMax))
         GDALComputeRasterMinMax((GDALRasterBandH)poBand, TRUE, adfMinMax);
      Con::printf("Min=%.3fd, Max=%.3f\n", adfMinMax[0], adfMinMax[1]);
      if (poBand->GetOverviewCount() > 0)
         Con::printf("Band has %d overviews.\n", poBand->GetOverviewCount());

#pragma endregion Vervose

      for (int i = 0; i < nYSize; i++)
      {
         poBand->RasterIO(GF_Read, 0, i, nXSize, 1,
            pafScanline, nXSize, 1, gddt,
            0, 0);

//          for (S32 j = 0; j < nXSize; j++)
//          {
//             F32 val = pafScanline[j];
//             Con::printf("%f", val);
//          }

         U8 *rowDest = bitmap->getAddress(0, i);

         switch (gddt)
         {
         case GDT_Byte:
         {
            GDALColorEntry ce;
            for (S32 j = 0; j < nXSize; j++)
            {
               U8 * pixelLocation = reinterpret_cast<U8 *>(&rowDest[(j + nChannel) * bytes_detph]);
               if (ct == NULL)
                  pixelLocation[0] = (U8)pafScanline[j];
               else
               {

                  ct->GetColorEntryAsRGB(pafScanline[j], &ce);
                  pixelLocation[0] = ce.c1;
                  pixelLocation[1] = ce.c2;
                  pixelLocation[2] = ce.c3;
               }
            }
         }
            break;
         case GDT_UInt16:
            for (S32 j = 0; j < nXSize; j++)
            {
               U16 * pixelLocation = reinterpret_cast<U16 *>(&rowDest[(j + nChannel) * bytes_detph]);
               pixelLocation[0] = (U16)pafScanline[j];
            }
            break;
         case GDT_Int16: break;
         case GDT_UInt32: break;
         case GDT_Int32: break;
         case GDT_Float32:
            for (S32 j = 0; j < nXSize; j++)
            {
               F32 * pixelLocation = reinterpret_cast<F32 *>(&rowDest[(j + nChannel) * bytes_detph]);
               pixelLocation[0] = (F32)pafScanline[j];
            }
            break;

         default: 
            Con::errorf("Tipo de datos no usado.");
            ;
         }

      }

   }

   CPLFree(pafScanline);

   GDALClose(preadDS);

//    // We gotta attach the extension ourselves.
//    char filename[256];
//    dSprintf(filename, 256, "%s.%s", "imgdebug", "png");
// 
//    // Open up the file on disk.
//    FileStream fs;
//    if (!fs.open(filename, Torque::FS::File::Write))
//       printf("ScreenShot::_singleCapture() - Failed to open output file '%s'!", filename);
//    else
//    {
//       bitmap->writeBitmap("png", fs);
//       fs.close();
//    }

//    switch (cinfo.out_color_space)
//    {
//       case JCS_GRAYSCALE:  format = GFXFormatA8; break;
//       case JCS_RGB:        format = GFXFormatR8G8B8;   break;
//       default:
//          gdal_destroy_decompress(&cinfo);
//          return false;
//    }



   // We know JPEG's don't have any transparency
   bitmap->setHasTransparency(false);
   
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
