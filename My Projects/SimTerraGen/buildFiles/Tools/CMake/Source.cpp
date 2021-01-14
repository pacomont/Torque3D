// #include "./gcore/gdal_priv.h"
// #include "./port/cpl_conv.h" // for CPLMalloc()
// int testgdal()
// {
//    GDALDataset  *poDataset;
//    GDALAllRegister();
//    poDataset = (GDALDataset *)GDALOpen("file", GA_ReadOnly);
// 
//    double        adfGeoTransform[6];
//    printf("Driver: %s/%s\n",
//       poDataset->GetDriver()->GetDescription(),
//       poDataset->GetDriver()->GetMetadataItem(GDAL_DMD_LONGNAME));
//    printf("Size is %dx%dx%d\n",
//       poDataset->GetRasterXSize(), poDataset->GetRasterYSize(),
//       poDataset->GetRasterCount());
//    if (poDataset->GetProjectionRef() != NULL)
//       printf("Projection is `%s'\n", poDataset->GetProjectionRef());
//    if (poDataset->GetGeoTransform(adfGeoTransform) == CE_None)
//    {
//       printf("Origin = (%.6f,%.6f)\n",
//          adfGeoTransform[0], adfGeoTransform[3]);
//       printf("Pixel Size = (%.6f,%.6f)\n",
//          adfGeoTransform[1], adfGeoTransform[5]);
//    }
// 
//    GDALRasterBand  *poBand;
//    int             nBlockXSize, nBlockYSize;
//    int             bGotMin, bGotMax;
//    double          adfMinMax[2];
//    poBand = poDataset->GetRasterBand(1);
//    poBand->GetBlockSize(&nBlockXSize, &nBlockYSize);
//    printf("Block=%dx%d Type=%s, ColorInterp=%s\n",
//       nBlockXSize, nBlockYSize,
//       GDALGetDataTypeName(poBand->GetRasterDataType()),
//       GDALGetColorInterpretationName(
//          poBand->GetColorInterpretation()));
//    adfMinMax[0] = poBand->GetMinimum(&bGotMin);
//    adfMinMax[1] = poBand->GetMaximum(&bGotMax);
//    if (!(bGotMin && bGotMax))
//       GDALComputeRasterMinMax((GDALRasterBandH)poBand, TRUE, adfMinMax);
//    printf("Min=%.3fd, Max=%.3f\n", adfMinMax[0], adfMinMax[1]);
//    if (poBand->GetOverviewCount() > 0)
//       printf("Band has %d overviews.\n", poBand->GetOverviewCount());
//    if (poBand->GetColorTable() != NULL)
//       printf("Band has a color table with %d entries.\n",
//          poBand->GetColorTable()->GetColorEntryCount());
// 
//    return 0;
// }
// 
