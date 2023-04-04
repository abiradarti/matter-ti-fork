/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <app/clusters/ota-requestor/OTADownloader.h>
#include <app/clusters/ota-requestor/OTARequestorInterface.h>
#include <lib/support/DefaultStorageKeyAllocator.h>

#include "OTAImageProcessorImpl.h"

#include <ti/net/ota/ota.h>

#include <ti/net/ota/source/OtaArchive.h>

#include <ti/drivers/power/PowerCC32XX.h>

#include <ti/drivers/net/wifi/fs.h>

#include "ota_settings.h"

#include <ti_drivers_config.h>


using namespace chip::DeviceLayer;
using namespace chip::DeviceLayer::PersistedStorage;

#define OTA_VERSION_LEN 14
#define OTA_BUFF_SIZE 1024

// static struct
// {
//     int                 state;
//     uint32_t            flags;
//     ota_eventCallback_f fAppCallback;
//     uint32_t            localDevType;
//     uint8_t             localMacAddress[SL_MAC_ADDR_LEN];
//     char                localNetworkSSID[32];
//     SlNetCfgIpV4Args_t  localIpInfo;

//     workQ_t             hWQ;
//     char                *pVersion;
//     char                newVersion[VERSION_STR_SIZE+1];
//     uint32_t            commitWatchdogTimeout;

//     void *              hSession;
//     OtaArchive_t        hOtaArchive;

//     OtaCounters_t       counters;
//     OtaIfType_e         type;
//     unsigned int        nImageLen;
//     uint32_t            nTotalRead;

//     OtaIfStatus         status;
//     uint8_t             progressPercent;

//     union
//     {
//         FileServerParams_t  fileServerParams;
//         TarFileParams_t     tarFileParams;
//         HTTPSRV_IF_params_t httpServerParams;
//         uint8_t             buff[OTA_BUFF_SIZE];
//     };
// } m_ota = { 0 };

OtaArchive_t        hOtaArchive;
uint32_t totalFileBytes;


extern "C" {
int FILE_write(char * pFilename, uint16_t length, uint8_t * pValue, uint32_t * pToken, uint32_t flags);
int FILE_read(int8_t * pFilename, uint16_t length, uint8_t * pValue, uint32_t token);
};

extern "C" void cc32xxLog(const char * aFormat, ...);

namespace chip {

CHIP_ERROR OTAImageProcessorImpl::PrepareDownload()
{
    PlatformMgr().ScheduleWork(HandlePrepareDownload, reinterpret_cast<intptr_t>(this));
    return CHIP_NO_ERROR;
}

CHIP_ERROR OTAImageProcessorImpl::Finalize()
{
    PlatformMgr().ScheduleWork(HandleFinalize, reinterpret_cast<intptr_t>(this));
    return CHIP_NO_ERROR;
}

CHIP_ERROR OTAImageProcessorImpl::Apply()
{
    PlatformMgr().ScheduleWork(HandleApply, reinterpret_cast<intptr_t>(this));
    return CHIP_NO_ERROR;
}

CHIP_ERROR OTAImageProcessorImpl::Abort()
{
    PlatformMgr().ScheduleWork(HandleAbort, reinterpret_cast<intptr_t>(this));
    return CHIP_NO_ERROR;
}

CHIP_ERROR OTAImageProcessorImpl::ProcessBlock(ByteSpan & block)
{
     if ((nullptr == block.data()) || block.empty())
    {
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    // Store block data for HandleProcessBlock to access
    CHIP_ERROR err = SetBlock(block);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(SoftwareUpdate, "Cannot set block data: %" CHIP_ERROR_FORMAT, err.Format());
    }
    PlatformMgr().ScheduleWork(HandleProcessBlock, reinterpret_cast<intptr_t>(this));
    return CHIP_NO_ERROR;
}

bool OTAImageProcessorImpl::IsFirstImageRun()
{
    OTARequestorInterface * requestor;
    uint32_t runningSwVer;

    if (CHIP_NO_ERROR != ConfigurationMgr().GetSoftwareVersion(runningSwVer))
    {
        return false;
    }

    requestor = GetRequestorInstance();

    return (requestor->GetTargetVersion() == runningSwVer) &&
        (requestor->GetCurrentUpdateState() == chip::app::Clusters::OtaSoftwareUpdateRequestor::OTAUpdateStateEnum::kApplying);
}


CHIP_ERROR OTAImageProcessorImpl::ConfirmCurrentImage(){

    int rc = OtaArchive_commit();
    if(rc == 0)
    {
        PowerCC32XX_reset(PowerCC32XX_PERIPH_WDT);
        return CHIP_NO_ERROR;
    }
    return CHIP_NO_ERROR;
}

void OTAImageProcessorImpl::HandlePrepareDownload(intptr_t context)
{
    auto * imageProcessor = reinterpret_cast<OTAImageProcessorImpl *>(context);
    if (imageProcessor == nullptr)
    {
        ChipLogError(SoftwareUpdate, "ImageProcessor context is null");
        return;
    }
    else if (imageProcessor->mDownloader == nullptr)
    {
        ChipLogError(SoftwareUpdate, "mDownloader is null");
        return;
    }

    imageProcessor->mHeaderParser.Init();

    OtaArchive_init(&hOtaArchive);

}

void OTAImageProcessorImpl::HandleFinalize(intptr_t context)
{
    auto * imageProcessor = reinterpret_cast<OTAImageProcessorImpl *>(context);
    if (imageProcessor == nullptr)
    {
        return;
    }
    //cc32xxLog("OTAImageProcessorImpl::HandleFinalize");
    imageProcessor->ReleaseBlock();
}

void OTAImageProcessorImpl::HandleApply(intptr_t context)
{
    auto * imageProcessor = reinterpret_cast<OTAImageProcessorImpl *>(context);
    if (imageProcessor == nullptr)
    {
        return;
    }

    int rc = OtaArchive_commit();
    if(rc == 0)
    {
        PowerCC32XX_reset(PowerCC32XX_PERIPH_WDT);
    }
}

void OTAImageProcessorImpl::HandleAbort(intptr_t context)
{
    auto * imageProcessor = reinterpret_cast<OTAImageProcessorImpl *>(context);
    if (imageProcessor == nullptr)
    {
        return;
    }

    OtaArchive_abort(&hOtaArchive);

    imageProcessor->ReleaseBlock();
}

void OTAImageProcessorImpl::HandleProcessBlock(intptr_t context)
{
    auto * imageProcessor = reinterpret_cast<OTAImageProcessorImpl *>(context);
    if (imageProcessor == nullptr)
    {
        ChipLogError(SoftwareUpdate, "ImageProcessor context is null");
        return;
    }
    else if (imageProcessor->mDownloader == nullptr)
    {
        ChipLogError(SoftwareUpdate, "mDownloader is null");
        return;
    }

    int         rc = 0;
    uint32_t    nTotalProcessed = 0;
    uint16_t    nProcessed = 0;
    uint16_t    nUnprocessed = imageProcessor->mBlock.size();
    // uint8_t     nProgressBarPercentStep;
    // uint8_t     nProgressBarPercentCount;
    // uint32_t    nProgressBarStep;
    // uint32_t    nProgressBarNext;
    int         otaArchiveStatus = ARCHIVE_STATUS_CONTINUE;
    // bool        bEndOfInput = false;

    ByteSpan block        = imageProcessor->mBlock;
    CHIP_ERROR chip_error = imageProcessor->ProcessHeader(block);

    if (chip_error != CHIP_NO_ERROR)
    {
        ChipLogError(SoftwareUpdate, "Matter image header parser error %s", chip::ErrorStr(chip_error));
        imageProcessor->mDownloader->EndDownload(CHIP_ERROR_INVALID_FILE_IDENTIFIER);
        return;
    }

    
    // /*** Initiate progress-bar params ***/
    // if(m_ota.nImageLen)
    // {
    //     nProgressBarPercentStep = (OTA_BUFF_SIZE * 100 / m_ota.nImageLen);
    //     if(nProgressBarPercentStep < 4)
    //         nProgressBarPercentStep = 4;
    //     nProgressBarStep = m_ota.nImageLen * nProgressBarPercentStep / 100;
    //     nProgressBarNext = nProgressBarStep;
    //     nProgressBarPercentCount = 0;
    // }
    
    while( rc >= 0 && nUnprocessed &&
            (totalFileBytes == 0 || nTotalProcessed < totalFileBytes) )
    {
        /*** Call OtaArchive with unprocessed bytes ***/
        cc32xxLog("ProcessOta:: Ready to process (%d)", nUnprocessed);
        uint16_t byteProcessed;
        otaArchiveStatus = OtaArchive_process(&hOtaArchive, imageProcessor->mBlock.data(), (int16_t)nUnprocessed, (int16_t*)&byteProcessed);

        /*** Update processing counters ***/
        nProcessed += byteProcessed;
        nUnprocessed -= byteProcessed;
        nTotalProcessed += byteProcessed;
        cc32xxLog("ProcessOta:: OtaArchive status=%d, processed=%d (%d), unprocessed=%d", otaArchiveStatus, byteProcessed, nTotalProcessed, nUnprocessed);

        // /*** Update (LOG) progress bar ***/
       
        // if(nTotalProcessed >= nProgressBarNext)
        // {
        //     nProgressBarNext += nProgressBarStep;
        //     nProgressBarPercentCount += nProgressBarPercentStep;
        //     m_ota.progressPercent = nProgressBarPercentCount;
        //     cc32xxLog("ProcessOta: ---- Download file in progress (%02d%%) %d/%d ----", nProgressBarPercentCount, nTotalProcessed, m_ota.nImageLen);
        // }
        

        if(otaArchiveStatus == ARCHIVE_STATUS_DOWNLOAD_DONE)
        {
            /*** Terminate session upon successful download completion ***/
            nUnprocessed = 0;
            cc32xxLog("ProcessOta: ---- Download file completed");
            // rc = StopLoad(0);
            //return 0;
        }
        else if(otaArchiveStatus < 0)
        {
            // int status;
            rc = otaArchiveStatus;
            /*** Terminate session upon OTA failure ***/
            cc32xxLog("ProcessOta: ---- OTA failure (%d)", rc);

            nUnprocessed = 0;
            imageProcessor->mDownloader->EndDownload(CHIP_ERROR_WRITE_FAILED);
            // status = StopLoad(rc);
            // if (status != 0)
            //     while(1);

            //return rc;
        }


        // /*** Load next chunk - if  OtaArchive require more bytes,
        //      or unprocessed count is less then half the buffer (and TotalRead < ImageLen) ***/
        // if(otaArchiveStatus == ARCHIVE_STATUS_CONTINUE )
        // {
        //     /*** Verify that MCU image fits the device type ***/
        //     if(OtaArchive_getStatus(&hOtaArchive) == ARCHIVE_STATE_OPEN_FILE)
        //     {
        //         rc = CheckMcuImage((char *)hOtaArchive.CurrTarObj.pFileName);
        //         if(rc < 0)
        //             return;
        //         else
        //             cc32xxLog("ProcessOta: ---- Writing file: %s", (char *)hOtaArchive.CurrTarObj.pFileName);
        //     }
        // }
    }
    imageProcessor->mDownloader->FetchNextData();
}

CHIP_ERROR OTAImageProcessorImpl::ProcessHeader(ByteSpan & block)
{
    if (mHeaderParser.IsInitialized())
    {
        OTAImageHeader header;
        CHIP_ERROR error = mHeaderParser.AccumulateAndDecode(block, header);

        // Needs more data to decode the header
        ReturnErrorCodeIf(error == CHIP_ERROR_BUFFER_TOO_SMALL, CHIP_NO_ERROR);
        ReturnErrorOnFailure(error);

        // SL TODO -- store version somewhere
        ChipLogProgress(SoftwareUpdate, "Image Header software version: %ld payload size: %lu", header.mSoftwareVersion,
                        (long unsigned int) header.mPayloadSize);
        mParams.totalFileBytes = header.mPayloadSize;
        totalFileBytes = header.mPayloadSize;
        mHeaderParser.Clear();
    }
    return CHIP_NO_ERROR;
}

CHIP_ERROR OTAImageProcessorImpl::SetBlock(ByteSpan & block)
{
    if (!IsSpanUsable(block))
    {
        ReleaseBlock();
        return CHIP_NO_ERROR;
    }
    if (mBlock.size() < block.size())
    {
        if (!mBlock.empty())
        {
            ReleaseBlock();
        }
        uint8_t * mBlock_ptr = static_cast<uint8_t *>(chip::Platform::MemoryAlloc(block.size()));
        if (mBlock_ptr == nullptr)
        {
            return CHIP_ERROR_NO_MEMORY;
        }
        mBlock = MutableByteSpan(mBlock_ptr, block.size());
    }
    CHIP_ERROR err = CopySpanToMutableSpan(block, mBlock);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(SoftwareUpdate, "Cannot copy block data: %" CHIP_ERROR_FORMAT, err.Format());
        return err;
    }
    return CHIP_NO_ERROR;
}

CHIP_ERROR OTAImageProcessorImpl::ReleaseBlock()
{
    if (mBlock.data() != nullptr)
    {
        chip::Platform::MemoryFree(mBlock.data());
    }

    mBlock = MutableByteSpan();
    return CHIP_NO_ERROR;
}

// int16_t StopLoad(int status)
// {
//     int16_t rc = m_sessionCBs[m_ota.type].fEndSession(m_ota.hSession, status);
//     return rc;
// }

int OTA_IF_getCurrentVersion(uint8_t *pVersion)
{
    int16_t rc = SL_ERROR_BSD_EINVAL;

    if(pVersion)
    {
        rc = FILE_read((int8_t*)"ota.dat", OTA_VERSION_LEN, pVersion, 0);
        if(rc < 0)
        {
            memset(pVersion, '0', OTA_VERSION_LEN);
            char fileName[] = "ota.dat";
            rc = FILE_write(fileName, OTA_VERSION_LEN, (uint8_t*)pVersion, NULL, SL_FS_CREATE_FAILSAFE);
            if (rc < 0)
                while(1)
                    ;
        }
    }
    if(rc < 0){
        cc32xxLog("FILE_read failed");
    } 
    return rc;
}

bool OTA_IF_isNewVersion(uint8_t *pCandidateVersion)
{
    bool bIsNewer;
    uint8_t currVersion[OTA_VERSION_LEN];
    int cmpResult = 1;

    OTA_IF_getCurrentVersion(currVersion);
#if !OTA_IF_FLAG_DISABLE_DOWNGRADE_PROTECTION
    cmpResult = memcmp(pCandidateVersion, currVersion, OTA_VERSION_LEN);
#endif

    if(cmpResult > 0)
    {
        cc32xxLog("candidate version: (%.*s) is newer than current version: (%.*s)", OTA_VERSION_LEN, pCandidateVersion, OTA_VERSION_LEN, currVersion);
        bIsNewer = true;
    }
    else
    {
        cc32xxLog("candidate version: (%.*s) is older than current version: (%.*s)", OTA_VERSION_LEN, pCandidateVersion, OTA_VERSION_LEN, currVersion);
        bIsNewer = false;
    }
    return bIsNewer;
}



} // namespace chip
