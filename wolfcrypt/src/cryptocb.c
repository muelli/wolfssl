/* cryptocb.c
 *
 * Copyright (C) 2006-2018 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* This framework provides a central place for crypto hardware integration
   using the devId scheme. If not supported return `NOT_COMPILED_IN`. */

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <wolfssl/wolfcrypt/settings.h>

#ifdef WOLF_CRYPTO_CB

#include <wolfssl/wolfcrypt/cryptocb.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/logging.h>


/* TODO: Consider linked list with mutex */
#ifndef MAX_CRYPTO_DEVID_CALLBACKS
#define MAX_CRYPTO_DEVID_CALLBACKS 8
#endif

typedef struct CryptoCb {
    int devId;
    CryptoDevCallbackFunc cb;
    void* ctx;
} CryptoCb;
static CryptoCb gCryptoDev[MAX_CRYPTO_DEVID_CALLBACKS];

static CryptoCb* wc_CryptoCb_FindDevice(int devId)
{
    int i;
    for (i=0; i<MAX_CRYPTO_DEVID_CALLBACKS; i++) {
        if (gCryptoDev[i].devId == devId)
            return &gCryptoDev[i];
    }
    return NULL;
}

void wc_CryptoCb_Init(void)
{
    int i;
    for (i=0; i<MAX_CRYPTO_DEVID_CALLBACKS; i++) {
        gCryptoDev[i].devId = INVALID_DEVID;
    }
}

int wc_CryptoCb_RegisterDevice(int devId, CryptoDevCallbackFunc cb, void* ctx)
{
    /* find existing or new */
    CryptoCb* dev = wc_CryptoCb_FindDevice(devId);
    if (dev == NULL)
        dev = wc_CryptoCb_FindDevice(INVALID_DEVID);

    if (dev == NULL)
        return BUFFER_E; /* out of devices */

    dev->devId = devId;
    dev->cb = cb;
    dev->ctx = ctx;

    return 0;
}

void wc_CryptoCb_UnRegisterDevice(int devId)
{
    CryptoCb* dev = wc_CryptoCb_FindDevice(devId);
    if (dev) {
        XMEMSET(dev, 0, sizeof(*dev));
        dev->devId = INVALID_DEVID;
    }
}

#ifndef NO_RSA
int wc_CryptoCb_Rsa(const byte* in, word32 inLen, byte* out,
    word32* outLen, int type, RsaKey* key, WC_RNG* rng)
{
    int ret = NOT_COMPILED_IN;
    CryptoCb* dev;

    /* locate registered callback */
    dev = wc_CryptoCb_FindDevice(key->devId);
    if (dev) {
        if (dev->cb) {
            wc_CryptoInfo cryptoInfo;
            XMEMSET(&cryptoInfo, 0, sizeof(cryptoInfo));
            cryptoInfo.algo_type = WC_ALGO_TYPE_PK;
            cryptoInfo.pk.type = WC_PK_TYPE_RSA;
            cryptoInfo.pk.rsa.in = in;
            cryptoInfo.pk.rsa.inLen = inLen;
            cryptoInfo.pk.rsa.out = out;
            cryptoInfo.pk.rsa.outLen = outLen;
            cryptoInfo.pk.rsa.type = type;
            cryptoInfo.pk.rsa.key = key;
            cryptoInfo.pk.rsa.rng = rng;

            ret = dev->cb(key->devId, &cryptoInfo, dev->ctx);
        }
    }

    return ret;
}

#ifdef WOLFSSL_KEY_GEN
int wc_CryptoCb_MakeRsaKey(RsaKey* key, int size, long e, WC_RNG* rng)
{
    int ret = NOT_COMPILED_IN;
    CryptoCb* dev;

    /* locate registered callback */
    dev = wc_CryptoCb_FindDevice(key->devId);
    if (dev) {
        if (dev->cb) {
            wc_CryptoInfo cryptoInfo;
            XMEMSET(&cryptoInfo, 0, sizeof(cryptoInfo));
            cryptoInfo.algo_type = WC_ALGO_TYPE_PK;
            cryptoInfo.pk.type = WC_PK_TYPE_RSA_KEYGEN;
            cryptoInfo.pk.rsakg.key = key;
            cryptoInfo.pk.rsakg.size = size;
            cryptoInfo.pk.rsakg.e = e;
            cryptoInfo.pk.rsakg.rng = rng;

            ret = dev->cb(key->devId, &cryptoInfo, dev->ctx);
        }
    }

    return ret;
}
#endif
#endif /* !NO_RSA */

#ifdef HAVE_ECC
int wc_CryptoCb_MakeEccKey(WC_RNG* rng, int keySize, ecc_key* key, int curveId)
{
    int ret = NOT_COMPILED_IN;
    CryptoCb* dev;

    /* locate registered callback */
    dev = wc_CryptoCb_FindDevice(key->devId);
    if (dev) {
        if (dev->cb) {
            wc_CryptoInfo cryptoInfo;
            XMEMSET(&cryptoInfo, 0, sizeof(cryptoInfo));
            cryptoInfo.algo_type = WC_ALGO_TYPE_PK;
            cryptoInfo.pk.type = WC_PK_TYPE_EC_KEYGEN;
            cryptoInfo.pk.eckg.rng = rng;
            cryptoInfo.pk.eckg.size = keySize;
            cryptoInfo.pk.eckg.key = key;
            cryptoInfo.pk.eckg.curveId = curveId;

            ret = dev->cb(key->devId, &cryptoInfo, dev->ctx);
        }
    }

    return ret;
}

int wc_CryptoCb_Ecdh(ecc_key* private_key, ecc_key* public_key,
    byte* out, word32* outlen)
{
    int ret = NOT_COMPILED_IN;
    CryptoCb* dev;

    /* locate registered callback */
    dev = wc_CryptoCb_FindDevice(private_key->devId);
    if (dev) {
        if (dev->cb) {
            wc_CryptoInfo cryptoInfo;
            XMEMSET(&cryptoInfo, 0, sizeof(cryptoInfo));
            cryptoInfo.algo_type = WC_ALGO_TYPE_PK;
            cryptoInfo.pk.type = WC_PK_TYPE_ECDH;
            cryptoInfo.pk.ecdh.private_key = private_key;
            cryptoInfo.pk.ecdh.public_key = public_key;
            cryptoInfo.pk.ecdh.out = out;
            cryptoInfo.pk.ecdh.outlen = outlen;

            ret = dev->cb(private_key->devId, &cryptoInfo, dev->ctx);
        }
    }

    return ret;
}

int wc_CryptoCb_EccSign(const byte* in, word32 inlen, byte* out,
    word32 *outlen, WC_RNG* rng, ecc_key* key)
{
    int ret = NOT_COMPILED_IN;
    CryptoCb* dev;

    /* locate registered callback */
    dev = wc_CryptoCb_FindDevice(key->devId);
    if (dev) {
        if (dev->cb) {
            wc_CryptoInfo cryptoInfo;
            XMEMSET(&cryptoInfo, 0, sizeof(cryptoInfo));
            cryptoInfo.algo_type = WC_ALGO_TYPE_PK;
            cryptoInfo.pk.type = WC_PK_TYPE_ECDSA_SIGN;
            cryptoInfo.pk.eccsign.in = in;
            cryptoInfo.pk.eccsign.inlen = inlen;
            cryptoInfo.pk.eccsign.out = out;
            cryptoInfo.pk.eccsign.outlen = outlen;
            cryptoInfo.pk.eccsign.rng = rng;
            cryptoInfo.pk.eccsign.key = key;

            ret = dev->cb(key->devId, &cryptoInfo, dev->ctx);
        }
    }

    return ret;
}

int wc_CryptoCb_EccVerify(const byte* sig, word32 siglen,
    const byte* hash, word32 hashlen, int* res, ecc_key* key)
{
    int ret = NOT_COMPILED_IN;
    CryptoCb* dev;

    /* locate registered callback */
    dev = wc_CryptoCb_FindDevice(key->devId);
    if (dev) {
        if (dev->cb) {
            wc_CryptoInfo cryptoInfo;
            XMEMSET(&cryptoInfo, 0, sizeof(cryptoInfo));
            cryptoInfo.algo_type = WC_ALGO_TYPE_PK;
            cryptoInfo.pk.type = WC_PK_TYPE_ECDSA_VERIFY;
            cryptoInfo.pk.eccverify.sig = sig;
            cryptoInfo.pk.eccverify.siglen = siglen;
            cryptoInfo.pk.eccverify.hash = hash;
            cryptoInfo.pk.eccverify.hashlen = hashlen;
            cryptoInfo.pk.eccverify.res = res;
            cryptoInfo.pk.eccverify.key = key;

            ret = dev->cb(key->devId, &cryptoInfo, dev->ctx);
        }
    }

    return ret;
}
#endif /* HAVE_ECC */

#ifndef NO_AES
#ifdef HAVE_AESGCM
int wc_CryptoCb_AesGcmEncrypt(Aes* aes, byte* out,
                               const byte* in, word32 sz,
                               const byte* iv, word32 ivSz,
                               byte* authTag, word32 authTagSz,
                               const byte* authIn, word32 authInSz)
{
    int ret = NOT_COMPILED_IN;
    CryptoCb* dev;

    /* locate registered callback */
    dev = wc_CryptoCb_FindDevice(aes->devId);
    if (dev) {
        if (dev->cb) {
            wc_CryptoInfo cryptoInfo;
            XMEMSET(&cryptoInfo, 0, sizeof(cryptoInfo));
            cryptoInfo.algo_type = WC_ALGO_TYPE_CIPHER;
            cryptoInfo.cipher.type = WC_CIPHER_AES_GCM;
            cryptoInfo.cipher.enc = 1;
            cryptoInfo.cipher.aesgcm_enc.aes       = aes;
            cryptoInfo.cipher.aesgcm_enc.out       = out;
            cryptoInfo.cipher.aesgcm_enc.in        = in;
            cryptoInfo.cipher.aesgcm_enc.sz        = sz;
            cryptoInfo.cipher.aesgcm_enc.iv        = iv;
            cryptoInfo.cipher.aesgcm_enc.ivSz      = ivSz;
            cryptoInfo.cipher.aesgcm_enc.authTag   = authTag;
            cryptoInfo.cipher.aesgcm_enc.authTagSz = authTagSz;
            cryptoInfo.cipher.aesgcm_enc.authIn    = authIn;
            cryptoInfo.cipher.aesgcm_enc.authInSz  = authInSz;

            ret = dev->cb(aes->devId, &cryptoInfo, dev->ctx);
        }
    }

    return ret;
}

int wc_CryptoCb_AesGcmDecrypt(Aes* aes, byte* out,
                               const byte* in, word32 sz,
                               const byte* iv, word32 ivSz,
                               const byte* authTag, word32 authTagSz,
                               const byte* authIn, word32 authInSz)
{
    int ret = NOT_COMPILED_IN;
    CryptoCb* dev;

    /* locate registered callback */
    dev = wc_CryptoCb_FindDevice(aes->devId);
    if (dev) {
        if (dev->cb) {
            wc_CryptoInfo cryptoInfo;
            XMEMSET(&cryptoInfo, 0, sizeof(cryptoInfo));
            cryptoInfo.algo_type = WC_ALGO_TYPE_CIPHER;
            cryptoInfo.cipher.type = WC_CIPHER_AES_GCM;
            cryptoInfo.cipher.enc = 0;
            cryptoInfo.cipher.aesgcm_dec.aes       = aes;
            cryptoInfo.cipher.aesgcm_dec.out       = out;
            cryptoInfo.cipher.aesgcm_dec.in        = in;
            cryptoInfo.cipher.aesgcm_dec.sz        = sz;
            cryptoInfo.cipher.aesgcm_dec.iv        = iv;
            cryptoInfo.cipher.aesgcm_dec.ivSz      = ivSz;
            cryptoInfo.cipher.aesgcm_dec.authTag   = authTag;
            cryptoInfo.cipher.aesgcm_dec.authTagSz = authTagSz;
            cryptoInfo.cipher.aesgcm_dec.authIn    = authIn;
            cryptoInfo.cipher.aesgcm_dec.authInSz  = authInSz;

            ret = dev->cb(aes->devId, &cryptoInfo, dev->ctx);
        }
    }

    return ret;
}
#endif /* HAVE_AESGCM */

#ifdef HAVE_AES_CBC
int wc_CryptoCb_AesCbcEncrypt(Aes* aes, byte* out,
                               const byte* in, word32 sz)
{
    int ret = NOT_COMPILED_IN;
    CryptoCb* dev;

    /* locate registered callback */
    dev = wc_CryptoCb_FindDevice(aes->devId);
    if (dev) {
        if (dev->cb) {
            wc_CryptoInfo cryptoInfo;
            XMEMSET(&cryptoInfo, 0, sizeof(cryptoInfo));
            cryptoInfo.algo_type = WC_ALGO_TYPE_CIPHER;
            cryptoInfo.cipher.type = WC_CIPHER_AES_CBC;
            cryptoInfo.cipher.enc = 1;
            cryptoInfo.cipher.aescbc.aes       = aes;
            cryptoInfo.cipher.aescbc.out       = out;
            cryptoInfo.cipher.aescbc.in        = in;
            cryptoInfo.cipher.aescbc.sz        = sz;

            ret = dev->cb(aes->devId, &cryptoInfo, dev->ctx);
        }
    }

    return ret;
}

int wc_CryptoCb_AesCbcDecrypt(Aes* aes, byte* out,
                               const byte* in, word32 sz)
{
    int ret = NOT_COMPILED_IN;
    CryptoCb* dev;

    /* locate registered callback */
    dev = wc_CryptoCb_FindDevice(aes->devId);
    if (dev) {
        if (dev->cb) {
            wc_CryptoInfo cryptoInfo;
            XMEMSET(&cryptoInfo, 0, sizeof(cryptoInfo));
            cryptoInfo.algo_type = WC_ALGO_TYPE_CIPHER;
            cryptoInfo.cipher.type = WC_CIPHER_AES_CBC;
            cryptoInfo.cipher.enc = 0;
            cryptoInfo.cipher.aescbc.aes       = aes;
            cryptoInfo.cipher.aescbc.out       = out;
            cryptoInfo.cipher.aescbc.in        = in;
            cryptoInfo.cipher.aescbc.sz        = sz;

            ret = dev->cb(aes->devId, &cryptoInfo, dev->ctx);
        }
    }

    return ret;
}
#endif /* HAVE_AES_CBC */
#endif /* !NO_AES */

#ifndef NO_SHA
int wc_CryptoCb_ShaHash(wc_Sha* sha, const byte* in,
    word32 inSz, byte* digest)
{
    int ret = NOT_COMPILED_IN;
    CryptoCb* dev;

    /* locate registered callback */
    dev = wc_CryptoCb_FindDevice(sha->devId);
    if (dev) {
        if (dev->cb) {
            wc_CryptoInfo cryptoInfo;
            XMEMSET(&cryptoInfo, 0, sizeof(cryptoInfo));
            cryptoInfo.algo_type = WC_ALGO_TYPE_HASH;
            cryptoInfo.hash.type = WC_HASH_TYPE_SHA;
            cryptoInfo.hash.sha1 = sha;
            cryptoInfo.hash.in = in;
            cryptoInfo.hash.inSz = inSz;
            cryptoInfo.hash.digest = digest;

            ret = dev->cb(sha->devId, &cryptoInfo, dev->ctx);
        }
    }

    return ret;
}
#endif /* !NO_SHA */

#ifndef NO_SHA256
int wc_CryptoCb_Sha256Hash(wc_Sha256* sha256, const byte* in,
    word32 inSz, byte* digest)
{
    int ret = NOT_COMPILED_IN;
    CryptoCb* dev;

    /* locate registered callback */
    dev = wc_CryptoCb_FindDevice(sha256->devId);
    if (dev) {
        if (dev->cb) {
            wc_CryptoInfo cryptoInfo;
            XMEMSET(&cryptoInfo, 0, sizeof(cryptoInfo));
            cryptoInfo.algo_type = WC_ALGO_TYPE_HASH;
            cryptoInfo.hash.type = WC_HASH_TYPE_SHA256;
            cryptoInfo.hash.sha256 = sha256;
            cryptoInfo.hash.in = in;
            cryptoInfo.hash.inSz = inSz;
            cryptoInfo.hash.digest = digest;

            ret = dev->cb(sha256->devId, &cryptoInfo, dev->ctx);
        }
    }

    return ret;
}
#endif /* !NO_SHA256 */

#ifndef WC_NO_RNG
int wc_CryptoCb_RandomBlock(WC_RNG* rng, byte* out, word32 sz)
{
    int ret = NOT_COMPILED_IN;
    CryptoCb* dev;

    /* locate registered callback */
    dev = wc_CryptoCb_FindDevice(rng->devId);
    if (dev) {
        if (dev->cb) {
            wc_CryptoInfo cryptoInfo;
            XMEMSET(&cryptoInfo, 0, sizeof(cryptoInfo));
            cryptoInfo.algo_type = WC_ALGO_TYPE_RNG;
            cryptoInfo.rng.rng = rng;
            cryptoInfo.rng.out = out;
            cryptoInfo.rng.sz = sz;

            ret = dev->cb(rng->devId, &cryptoInfo, dev->ctx);
        }
    }

    return ret;
}
#endif /* !WC_NO_RNG */

#endif /* WOLF_CRYPTO_CB */
