/*
 *
 *    Copyright (c) 2023 Project CHIP Authors
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

#include "FactoryDataProvider.h"
#include "CC13XX_26XXConfig.h"
#include <crypto/CHIPCryptoPAL.h>
#include <lib/core/CHIPError.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/Span.h>

using namespace ::chip::DeviceLayer::Internal;

namespace chip {
namespace DeviceLayer {

typedef struct
{
    const uint32_t len;
    uint8_t const * data;
} data_ptr;

typedef struct
{
    data_ptr serial_number;
    data_ptr vendor_id;
    data_ptr product_id;
    data_ptr vendor_name;
    data_ptr product_name;
    data_ptr manufacturing_date;
    data_ptr hw_ver;
    data_ptr hw_ver_str;
    data_ptr dac_cert;
    data_ptr pai_cert;
    data_ptr rd_uniqueid;
    data_ptr spake2_it;
    data_ptr spake2_salt;
    data_ptr spake2_verifier;
    data_ptr discriminator;
} factoryData;

#if defined(__GNUC__)
const factoryData __attribute__((section( ".factory_data_struct"))) __attribute__((used)) gFactoryData =
#else

#error "compiler currently not supported"

#endif
{
    .dac_priv_key = {
        .len = sizeof(gDacPrivKey),
        .data = gDacPrivKey,
    },
    .dac_pub_key = {
        .len = sizeof(gDacPubKey),
        .data = gDacPubKey,
    },
    .dac_cert = {
        .len = sizeof(gDacCert),
        .data = gDacCert,
    },
    .pai_cert = {
        .len = sizeof(gPaiCert),
        .data = gPaiCert,
    },
};

CHIP_ERROR FactoryDataProvider::Init()
{

}

CHIP_ERROR FactoryDataProvider::GetCertificationDeclaration(MutableByteSpan & outBuffer)
{

}
CHIP_ERROR FactoryDataProvider::GetFirmwareInformation(MutableByteSpan & out_firmware_info_buffer)
{

}
CHIP_ERROR FactoryDataProvider::GetDeviceAttestationCert(MutableByteSpan & outBuffer)
{
    ReturnErrorCodeIf(outBuffer.size() < mFactoryData->dac_cert.len, CHIP_ERROR_BUFFER_TOO_SMALL);
    ReturnErrorCodeIf(!mFactoryData->dac_cert.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);
    return CopySpanToMutableSpan(ByteSpan{ mFactoryData->dac_cert.data, mFactoryData->dac_cert.len }, outBuffer);
}
CHIP_ERROR FactoryDataProvider::GetProductAttestationIntermediateCert(MutableByteSpan & outBuffer)
{
    ReturnErrorCodeIf(outBuffer.size() < mFactoryData->pai_cert.len, CHIP_ERROR_BUFFER_TOO_SMALL);
    ReturnErrorCodeIf(!mFactoryData->pai_cert.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);
    return CopySpanToMutableSpan(ByteSpan{ mFactoryData->pai_cert.data, mFactoryData->pai_cert.len }, outBuffer);
}
CHIP_ERROR FactoryDataProvider::SignWithDeviceAttestationKey(const ByteSpan & messageToSign, MutableByteSpan & outSignBuffer)
{

}
CHIP_ERROR FactoryDataProvider::GetSetupDiscriminator(uint16_t & setupDiscriminator)
{
    ReturnErrorCodeIf(!mFactoryData->discriminator.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);
    memcpy(setupDiscriminator, mFactoryData->discriminator.data, mFactoryData->discriminator.len);
    return CHIP_NO_ERROR;
}
CHIP_ERROR FactoryDataProvider::SetSetupDiscriminator(uint16_t setupDiscriminator)
{
    return CHIP_ERROR_NOT_IMPLEMENTED;
}
CHIP_ERROR FactoryDataProvider::GetSpake2pIterationCount(uint32_t & iterationCount)
{
    ReturnErrorCodeIf(!mFactoryData->spake2_it.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);
    memcpy(iterationCount, mFactoryData->spake2p_it.data, mFactoryData->spake2p_it.len);
    return CHIP_NO_ERROR;
}
CHIP_ERROR FactoryDataProvider::GetSpake2pSalt(MutableByteSpan & saltBuf)
{
    ReturnErrorCodeIf(saltBuf.size() < mFactoryData->spake2_salt.len, CHIP_ERROR_BUFFER_TOO_SMALL);
    ReturnErrorCodeIf(!mFactoryData->spake2_salt.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);

    return CopySpanToMutableSpan(ByteSpan{ mFactoryData->spake2_salt.data, mFactoryData->spake2_salt.len }, saltBuf);
}
CHIP_ERROR FactoryDataProvider::GetSpake2pVerifier(MutableByteSpan & verifierBuf, size_t & verifierLen)
{
    ReturnErrorCodeIf(verifierLen < mFactoryData->spake2_verifier.len, CHIP_ERROR_BUFFER_TOO_SMALL);
    ReturnErrorCodeIf(!mFactoryData->spake2_verifier.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);

    return CopySpanToMutableSpan(ByteSpan{ mFactoryData->spake2_verifier.data, mFactoryData->spake2_verifier.len }, verifierBuf);
}
CHIP_ERROR FactoryDataProvider::GetSetupPasscode(uint32_t & setupPasscode)
{
    return CHIP_ERROR_NOT_IMPLEMENTED;
}
CHIP_ERROR FactoryDataProvider::SetSetupPasscode(uint32_t setupPasscode)
{
    return CHIP_ERROR_NOT_IMPLEMENTED;
}
CHIP_ERROR FactoryDataProvider::GetVendorName(char * buf, size_t bufSize)
{
    //FROM FACTORY DATA
    ReturnErrorCodeIf(bufSize < mFactoryData->vendor_name.len, CHIP_ERROR_BUFFER_TOO_SMALL);
    ReturnErrorCodeIf(!mFactoryData->vendor_name.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);
    memcpy(buf, mFactoryData->vendor_name.data, bufSize);
    return CHIP_NO_ERROR;
}
CHIP_ERROR FactoryDataProvider::GetVendorId(uint16_t & vendorId)
{
    //FROM FACTORY DATA
    ReturnErrorCodeIf(!mFactoryData->vendor_id.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);
    memcpy(vendorId, mFactoryData->vendor_id.data, mFactoryData->vendor_id.len);
    return CHIP_NO_ERROR;
}
CHIP_ERROR FactoryDataProvider::GetProductName(char * buf, size_t bufSize)
{
    //FROM FACTORY DATA
    ReturnErrorCodeIf(bufSize < mFactoryData->product_name.len, CHIP_ERROR_BUFFER_TOO_SMALL);
    ReturnErrorCodeIf(!mFactoryData->product_name.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);
    memcpy(buf, mFactoryData->product_name.data, bufSize);
    return CHIP_NO_ERROR;
}
CHIP_ERROR FactoryDataProvider::GetProductId(uint16_t & productId)
{
    //FROM FACTORY DATA
    ReturnErrorCodeIf(!mFactoryData->vendor_id.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);
    memcpy(productId, mFactoryData->vendor_id.data, mFactoryData->product_id.len);
    return CHIP_NO_ERROR;
}
CHIP_ERROR FactoryDataProvider::GetPartNumber(char * buf, size_t bufSize)
{
    return CHIP_ERROR_NOT_IMPLEMENTED;
}
CHIP_ERROR FactoryDataProvider::GetProductURL(char * buf, size_t bufSize)
{
    return CHIP_ERROR_NOT_IMPLEMENTED;
}
CHIP_ERROR FactoryDataProvider::GetProductLabel(char * buf, size_t bufSize)
{
    return CHIP_ERROR_NOT_IMPLEMENTED;
}
CHIP_ERROR FactoryDataProvider::GetSerialNumber(char * buf, size_t bufSize)
{
    ReturnErrorCodeIf(bufSize < mFactoryData->serial_number.len, CHIP_ERROR_BUFFER_TOO_SMALL);
    ReturnErrorCodeIf(!mFactoryData->serial_number.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);
    memcpy(buf, mFactoryData->serial_number.data, bufSize);
    return CHIP_NO_ERROR;
}
CHIP_ERROR FactoryDataProvider::GetManufacturingDate(uint16_t & year, uint8_t & month, uint8_t & day)
{
    return CHIP_ERROR_NOT_IMPLEMENTED;
}
CHIP_ERROR FactoryDataProvider::GetHardwareVersion(uint16_t & hardwareVersion)
{
    ReturnErrorCodeIf(!mFactoryData->hw_ver.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);
    memcpy(productId, mFactoryData->hw_ver.data, mFactoryData->hw_ver.len);
    return CHIP_NO_ERROR;
}
CHIP_ERROR FactoryDataProvider::GetHardwareVersionString(char * buf, size_t bufSize)
{
    //FROM FACTORY DATA
    ReturnErrorCodeIf(bufSize < mFactoryData->hw_ver_str.len, CHIP_ERROR_BUFFER_TOO_SMALL);
    ReturnErrorCodeIf(!mFactoryData->hw_ver_str.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);
    memcpy(buf, mFactoryData->hw_ver_str.data, bufSize);
    return CHIP_NO_ERROR;
}
CHIP_ERROR FactoryDataProvider::GetRotatingDeviceIdUniqueId(MutableByteSpan & uniqueIdSpan)
{
    ReturnErrorCodeIf(uniqueIdSpan.size() < mFactoryData->rd_uniqueid.len, CHIP_ERROR_BUFFER_TOO_SMALL);
    ReturnErrorCodeIf(!mFactoryData->rd_uniqueid.data, CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);
    return CopySpanToMutableSpan(ByteSpan{ mFactoryData->rd_uniqueid.data, mFactoryData->rd_uniqueid.len }, uniqueIdSpan);
}
} // namespace DeviceLayer
} // namespace chip
