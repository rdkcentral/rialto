#include "open_cdm_ext.h"
extern "C"
{
    OpenCDMError opencdm_system_ext_get_ldl_session_limit(struct OpenCDMSystem *system, uint32_t *ldlLimit)
    {
        return ERROR_NONE;
    }

    OpenCDMError opencdm_delete_key_store(struct OpenCDMSystem *system) { return ERROR_NONE; }

    OpenCDMError opencdm_delete_secure_store(struct OpenCDMSystem *system) { return ERROR_NONE; }

    OpenCDMError opencdm_get_key_store_hash_ext(struct OpenCDMSystem *system, uint8_t keyStoreHash[],
                                                uint32_t keyStoreHashLength)
    {
        return ERROR_NONE;
    }

    OpenCDMError opencdm_get_secure_store_hash_ext(struct OpenCDMSystem *system, uint8_t secureStoreHash[],
                                                   uint32_t secureStoreHashLength)
    {
        return ERROR_NONE;
    }

    OpenCDMError opencdm_session_get_challenge_data(struct OpenCDMSession *mOpenCDMSession, uint8_t *challenge,
                                                    uint32_t *challengeSize, uint32_t isLDL)
    {
        return ERROR_NONE;
    }

    OpenCDMError opencdm_session_store_license_data(struct OpenCDMSession *mOpenCDMSession, const uint8_t licenseData[],
                                                    uint32_t licenseDataSize, uint8_t *secureStopId)
    {
        return ERROR_NONE;
    }

    OpenCDMError opencdm_session_cancel_challenge_data(struct OpenCDMSession *mOpenCDMSession) { return ERROR_NONE; }

    OpenCDMError opencdm_session_clean_decrypt_context(struct OpenCDMSession *mOpenCDMSession) { return ERROR_NONE; }

    OpenCDMError opencdm_session_select_key_id(struct OpenCDMSession *mOpenCDMSession, uint8_t keyLenght,
                                               const uint8_t keyId[])
    {
        return ERROR_NONE;
    }
}