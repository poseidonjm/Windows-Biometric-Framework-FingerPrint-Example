﻿/*
 * Author: Tomáš Růžička, t_ruzicka (at) email.cz
 * 2015
 */

#include "headers.h"
#include<conio.h> //required for _getch()

void BmpSetImageData(SBmpImage *bmp, const std::vector<uint8> &data, uint32 width, uint32 height, uint32 horzRes, uint32 vertRes)
{
  bmp->data.resize(data.size());

  for(uint32 y = 0; y < height; y++)
  {
    const uint32 yy = (height - y - 1) * width;

    for(uint32 x = 0; x < width; x++)
    {
      const uint32 xy = yy + x;

      bmp->data[xy].r = data[y * width + x];
      bmp->data[xy].g = data[y * width + x];
      bmp->data[xy].b = data[y * width + x];
      bmp->data[xy].a = 255;
    }
  }

  bmp->dataPaddingSize = sizeof(uint32) - ((width * NBmp::BMP_24B_COLORS) % sizeof(uint32));
  if(bmp->dataPaddingSize == sizeof(uint32))
    bmp->dataPaddingSize = 0;

  bmp->signature = NBmp::BMP_HEADER_SIGNATURE;
  bmp->fileSize = NBmp::BMP_HEADER_FILE_DATA_OFFSET + bmp->colorTable.size() * NBmp::BMP_32B_COLORS + (width * NBmp::BMP_24B_COLORS + bmp->dataPaddingSize) * height;
  bmp->reserved = NBmp::BMP_HEADER_RESERVED;
  bmp->dataOffset = NBmp::BMP_HEADER_FILE_DATA_OFFSET + bmp->colorTable.size() * NBmp::BMP_32B_COLORS;
  bmp->infoHeader.headerSize = NBmp::BMP_INFO_HEADER_SIZE;
  bmp->infoHeader.width = width;
  bmp->infoHeader.height = height;
  bmp->infoHeader.planes = NBmp::BMP_INFO_HEADER_PLANES;
  bmp->infoHeader.bitsPerPixel = NBmp::BMP_INFO_HEADER_BITS_PER_PIXEL_24;
  bmp->infoHeader.compression = NBmp::BMP_INFO_HEADER_COMPRESSION;
  bmp->infoHeader.imageSize = (width * NBmp::BMP_24B_COLORS + bmp->dataPaddingSize) * height;
  //bmp->infoHeader.pixelPerMeterX = NBmp::BMP_INFO_HEADER_PIXEL_PER_METER_X;
  //bmp->infoHeader.pixelPerMeterY = NBmp::BMP_INFO_HEADER_PIXEL_PER_METER_Y;
  bmp->infoHeader.pixelPerMeterX = (int)(10000 * horzRes / 254);	// conversion inch<->meter
  bmp->infoHeader.pixelPerMeterY = (int)(10000 * vertRes / 254);
  bmp->infoHeader.colors = NBmp::BMP_INFO_HEADER_COLORS;
  bmp->infoHeader.usedColors = NBmp::BMP_INFO_HEADER_USED_COLORS;
}

void BmpSave(const SBmpImage *bmp, std::string filename)
{
  const uint32 dataPadding = 0;

  CFile file(filename);

  file.write(&bmp->signature[0], sizeof(char)* NBmp::BMP_HEADER_SIGNATURE_LENGHT);
  file.write(&bmp->fileSize, sizeof(uint32));
  file.write(&bmp->reserved, sizeof(uint32));
  file.write(&bmp->dataOffset, sizeof(uint32));
  file.write(&bmp->infoHeader.headerSize, sizeof(uint32));
  file.write(&bmp->infoHeader.width, sizeof(uint32));
  file.write(&bmp->infoHeader.height, sizeof(uint32));
  file.write(&bmp->infoHeader.planes, sizeof(uint16));
  file.write(&bmp->infoHeader.bitsPerPixel, sizeof(uint16));
  file.write(&bmp->infoHeader.compression, sizeof(uint32));
  file.write(&bmp->infoHeader.imageSize, sizeof(uint32));
  file.write(&bmp->infoHeader.pixelPerMeterX, sizeof(uint32));
  file.write(&bmp->infoHeader.pixelPerMeterY, sizeof(uint32));
  file.write(&bmp->infoHeader.colors, sizeof(uint32));
  file.write(&bmp->infoHeader.usedColors, sizeof(uint32));

  for(uint32 i = 0; i < bmp->colorTable.size(); i++)
  {
    file.write(&bmp->colorTable[i].r, sizeof(uint8));
    file.write(&bmp->colorTable[i].g, sizeof(uint8));
    file.write(&bmp->colorTable[i].b, sizeof(uint8));
    file.write(&bmp->colorTable[i].a, sizeof(uint8));
  }

  for(uint32 y = 0; y < bmp->infoHeader.height; y++)
  {
    const uint32 yy = (bmp->infoHeader.height - y - 1) * bmp->infoHeader.width;

    for(uint32 x = 0; x < bmp->infoHeader.width; x++)
    {
      const uint32 xy = yy + x;

      file.write(&bmp->data[xy].b, sizeof(uint8));
      file.write(&bmp->data[xy].g, sizeof(uint8));
      file.write(&bmp->data[xy].r, sizeof(uint8));
    }

    file.write(&dataPadding, sizeof(uint8)* bmp->dataPaddingSize);
  }

  file.close();
}

HRESULT CaptureSample()
{
  HRESULT hr = S_OK;
  WINBIO_SESSION_HANDLE sessionHandle = NULL;
  WINBIO_UNIT_ID unitId = 0;
  WINBIO_REJECT_DETAIL rejectDetail = 0;
  PWINBIO_BIR sample = NULL;
  SIZE_T sampleSize = 0;

  // Connect to the system pool. 
  hr = WinBioOpenSession(
    WINBIO_TYPE_FINGERPRINT,    // Service provider
    WINBIO_POOL_SYSTEM,         // Pool type
	WINBIO_FLAG_RAW,            // Access: Capture raw data //To call WinBioCaptureSample function successfully, you must open the session handle by specifying WINBIO_FLAG_RAW
								//WINBIO_FLAG_RAW: The client application captures raw biometric data using WinBioCaptureSample.
    NULL,                       // Array of biometric unit IDs //NULL if the PoolType parameter is WINBIO_POOL_SYSTEM
    0,                          // Count of biometric unit IDs//zero if the PoolType parameter is WINBIO_POOL_SYSTEM.
    WINBIO_DB_DEFAULT,          // Default database
    &sessionHandle              // [out] Session handle
    );

  if(FAILED(hr))
  {
    std::cout << "WinBioOpenSession failed. hr = 0x" << std::hex << hr << std::dec << "\n";

    if(sample != NULL)
    {
      WinBioFree(sample);
      sample = NULL;
    }

    if(sessionHandle != NULL)
    {
      WinBioCloseSession(sessionHandle);
      sessionHandle = NULL;
    }
    
    return hr;
  }

  // Capture a biometric sample.
  std::cout << "Calling WinBioCaptureSample - Swipe sensor...\n";

   hr = WinBioCaptureSample(
    sessionHandle,
    WINBIO_NO_PURPOSE_AVAILABLE,
	WINBIO_DATA_FLAG_RAW,//WINBIO_DATA_FLAG_RAW
    &unitId,
    &sample,
    &sampleSize,
    &rejectDetail
    );

  if(FAILED(hr))
  {
    if(hr == WINBIO_E_BAD_CAPTURE)
      std:: cout << "Bad capture; reason: " << rejectDetail << "\n";
    else
      std::cout << "WinBioCaptureSample failed.hr = 0x" << std::hex << hr << std::dec << "\n";

    if(sample != NULL)
    {
      WinBioFree(sample);
      sample = NULL;
    }

    if(sessionHandle != NULL)
    {
      WinBioCloseSession(sessionHandle);
      sessionHandle = NULL;
    }

    return hr;
  }

  std::cout << "Swipe processed - Unit ID: " << unitId << "\n";
  std::cout << "Captured " << sampleSize << " bytes.\n";

  if(sample != NULL)
  {
    PWINBIO_BIR_HEADER BirHeader = (PWINBIO_BIR_HEADER)(((PBYTE)sample) + sample->HeaderBlock.Offset);
    PWINBIO_BDB_ANSI_381_HEADER AnsiBdbHeader = (PWINBIO_BDB_ANSI_381_HEADER)(((PBYTE)sample) + sample->StandardDataBlock.Offset);
    PWINBIO_BDB_ANSI_381_RECORD AnsiBdbRecord = (PWINBIO_BDB_ANSI_381_RECORD)(((PBYTE)AnsiBdbHeader) + sizeof(WINBIO_BDB_ANSI_381_HEADER));

    DWORD width = AnsiBdbRecord->HorizontalLineLength; // Width of image in pixels
    DWORD height = AnsiBdbRecord->VerticalLineLength; // Height of image in pixels

    std::cout << "Image resolution: " << width << " x " << height << "\n";
    std::cout << "Horizontal resolution: " << AnsiBdbHeader->HorizontalImageResolution << " Vertical resolution: " << AnsiBdbHeader->VerticalImageResolution << "\n";
    std::cout << "ansi_381_record.BlockLength: " << AnsiBdbRecord->BlockLength << "\n";

    PBYTE firstPixel = (PBYTE)((PBYTE)AnsiBdbRecord) + sizeof(WINBIO_BDB_ANSI_381_RECORD);

    SBmpImage bmp;
    std::vector<uint8> data(width * height);
    memcpy(&data[0], firstPixel, width * height);

    SYSTEMTIME st;
    GetSystemTime(&st);
    std::stringstream s;
    s << st.wYear << "." << st.wMonth << "." << st.wDay << "." << st.wHour << "." << st.wMinute << "." << st.wSecond << "." << st.wMilliseconds;
    std::string bmpFile = "data/fingerPrint_"+s.str()+".bmp";

    BmpSetImageData(&bmp, data, width, height, AnsiBdbHeader->HorizontalImageResolution, AnsiBdbHeader->VerticalImageResolution);
    BmpSave(&bmp, bmpFile);
    //ShellExecuteA(NULL, NULL, bmpFile.c_str(), NULL, NULL, SW_SHOWNORMAL);

    CFile raw("data/rawData_" + s.str() + ".bin");
    raw.write(&data[0], data.size());
    raw.close();


    PBYTE vendor_firstByte = (PBYTE)((PBYTE)sample) + sample->VendorDataBlock.Offset;
    int vendor_size = sample->VendorDataBlock.Size;

    std::vector<uint8> pkc(vendor_size);
    memcpy(&pkc[0], vendor_firstByte, vendor_size);

    CFile raw_pck("data/template_" + s.str() + ".pkc");
    raw_pck.write(&pkc[0], pkc.size());
    raw_pck.close();

    WinBioFree(sample);
    sample = NULL;
  }

  if(sessionHandle != NULL)
  {
    WinBioCloseSession(sessionHandle);
    sessionHandle = NULL;
  }

  return hr;
}

HRESULT EnumerateSensors()
{
    // Declare variables.
    HRESULT hr = S_OK;
    PWINBIO_UNIT_SCHEMA unitSchema = NULL;
    SIZE_T unitCount = 0;
    SIZE_T index = 0;

    // Enumerate the installed biometric units.
    hr = WinBioEnumBiometricUnits(
        WINBIO_TYPE_FINGERPRINT,        // Type of biometric unit
        &unitSchema,                    // Array of unit schemas
        &unitCount);                   // Count of unit schemas

    if (FAILED(hr))
    {
        wprintf_s(L"\n WinBioEnumBiometricUnits failed. hr = 0x%x\n", hr);
        goto e_Exit;
    }

    // Display information for each installed biometric unit.
    wprintf_s(L"\nSensors: \n");
    for (index = 0; index < unitCount; ++index)
    {
        wprintf_s(L"\n[%d]: \tUnit ID: %d\n",
            index,
            unitSchema[index].UnitId);
        wprintf_s(L"\tDevice instance ID: %s\n",
            unitSchema[index].DeviceInstanceId);
        wprintf_s(L"\tPool type: %d\n",
            unitSchema[index].PoolType);
        wprintf_s(L"\tBiometric factor: %d\n",
            unitSchema[index].BiometricFactor);
        wprintf_s(L"\tSensor subtype: %d\n",
            unitSchema[index].SensorSubType);
        wprintf_s(L"\tSensor capabilities: 0x%08x\n",
            unitSchema[index].Capabilities);
        wprintf_s(L"\tDescription: %s\n",
            unitSchema[index].Description);
        wprintf_s(L"\tManufacturer: %s\n",
            unitSchema[index].Manufacturer);
        wprintf_s(L"\tModel: %s\n",
            unitSchema[index].Model);
        wprintf_s(L"\tSerial no: %s\n",
            unitSchema[index].SerialNumber);
        wprintf_s(L"\tFirmware version: [%d.%d]\n",
            unitSchema[index].FirmwareVersion.MajorVersion,
            unitSchema[index].FirmwareVersion.MinorVersion);
    }


e_Exit:
    if (unitSchema != NULL)
    {
        WinBioFree(unitSchema);
        unitSchema = NULL;
    }

    wprintf_s(L"\nPress any key to exit...");
    _getch();

    return hr;
}

int main()
{
    EnumerateSensors();
  CreateDirectoryA("data", NULL);
  CaptureSample();
  //while(!FAILED(CaptureSample()));
}