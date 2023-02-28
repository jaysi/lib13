#include "include/crypt13i.h"
#include "include/mem13.h"
#include "include/debug13.h"
#include "include/sha256.h"

#define _deb_copys _NullMsg
#define _deb_intern _NullMsg
#define _deb_bound _NullMsg
#define _deb_dec    _NullMsg
#define _deb_info   _NullMsg

char** crypt13_list_alg(crypt13_alg_t alg,
						crypt13_mode_t mode,
						crypt13_type_t type){

    char** buf;
    int i, j;

    buf = (char**)m13_malloc_2d(C13I_NTOTAL+1, C13I_LONGEST_NAME, 1);

    j = 0;
    for(i = 0; crypt13_alg[i].alg != CRYPT13_ALG_NONE; i++){

        if(alg != CRYPT13_ALG_ALL && crypt13_alg[i].alg != alg) continue;
        if(mode != CRYPT13_MODE_ALL && crypt13_alg[i].mode != mode) continue;
        if(type != CRYPT13_TYPE_ALL && crypt13_alg[i].type != type) continue;

        strcpy(buf[j++], crypt13_alg[i].name);

    }

    buf[j][0] = '\0';

    return buf;

}

error13_t crypt13_alg_info(crypt13_alg_t alg, struct crypt13* info){

    int i;

    for(i = 0; crypt13_alg[i].alg != CRYPT13_ALG_NONE; i++){

        if(crypt13_alg[i].alg == alg){
            _deb_copys("this copy tends to work right! [check name field]");
            _crypt13_copys(info, &crypt13_alg[i]);
            return E13_OK;
        }
    }

    return e13_error(E13_NOTFOUND);

}

crypt13_alg_t crypt13_alg_id(char* name){
	int i;
    for(i = 0; crypt13_alg[i].alg != CRYPT13_ALG_NONE; i++){

        if(!strcasecmp(name, crypt13_alg[i].name)){
            return crypt13_alg[i].alg;
        }
    }

    return CRYPT13_ALG_NONE;

}

error13_t crypt13_init(struct crypt13* s, crypt13_alg_t alg, uint8_t* key,
                        size_t keysize){

    int i;
    uint8_t sha_key[C13I_KEYSIZE/8];
    aes_context* aes;
    des3_context* des3;
    NtruEncKeyPair* kp;
    struct NtruEncParams* params, p = EES1087EP2_FAST;

    if(_crypt13_is_init(s)) {
		_deb_intern("already init!");
		return e13_error(E13_MISUSE);
	}

    for(i = 0; i < C13I_NTOTAL; i++){

        if(crypt13_alg[i].alg == alg){

            switch(alg){

                case CRYPT13_ALG_NONE:
                    _crypt13_copys(s, &crypt13_alg[i]);
                    s->ctx = NULL;
                    s->params = NULL;
                break;

                case CRYPT13_ALG_AES:

                    aes = (aes_context*)m13_malloc(sizeof(aes_context));
                    if(!aes) return e13_error(E13_NOMEM);
                    sha256(key, keysize, sha_key);
                    if(aes_set_key(aes, sha_key, C13I_AES_KEYSIZE)){
                        m13_free(aes);
                        _deb_intern("aes subsystem cannot set key!");
                        return e13_error(E13_INTERNAL);
                    }

                    _crypt13_copys(s, &crypt13_alg[i]);

                    s->ctx = aes;
                    s->params = NULL;

                break;

                case CRYPT13_ALG_DES3:

                des3 = (des3_context*)m13_malloc(sizeof(des3_context));
                if(!des3) return e13_error(E13_NOMEM);
                sha256(key, keysize, sha_key);
                if(des3_set_3keys(	des3, sha_key, sha_key + DES3_KSIZE,
									sha_key + ( 2 * DES3_KSIZE )))
				{
                    m13_free(des3);
                    _deb_intern("des3 subsystem cannot set key!");
                    return e13_error(E13_INTERNAL);
                }

                _crypt13_copys(s, &crypt13_alg[i]);

                s->ctx = des3;
                s->params = NULL;

                break;

            case CRYPT13_ALG_NTRU:

                params = (struct NtruEncParams*)m13_malloc(sizeof(struct NtruEncParams));
                if(!params) return e13_error(E13_NOMEM);

                kp = (NtruEncKeyPair*)m13_malloc(sizeof(NtruEncKeyPair));
                if(!kp){
                    m13_free(params);
                    return e13_error(E13_NOMEM);
                }

                memcpy(params, &p, sizeof(struct NtruEncParams));

                if (!ntru_gen_key_pair(params, kp, dev_urandom)){
                    m13_free(params);
                    m13_free(kp);
                    return e13_error(E13_INTERNAL);
                }


                _crypt13_copys(s, &crypt13_alg[i]);

                s->ctx = kp;
                s->params = params;

                break;
                
            case CRYPT13_ALG_BASE64:
            	//nothing to do really!
            	break;

            default:
                    return e13_error(E13_IMPLEMENT);
                break;
            }

            s->magic = MAGIC13_CR13;
            return E13_OK;
        }
    }

    return e13_error(E13_NOTFOUND);

}

error13_t crypt13_destroy(struct crypt13* s){
    if(!_crypt13_is_init(s)) return e13_error(E13_MISUSE);
    if(s->ctx) m13_free(s->ctx);
    if(s->params) m13_free(s->params);
    s->magic = MAGIC13_INV;
    return E13_OK;
}

error13_t crypt13_encrypt(  struct crypt13* s, uint8_t* input, size_t insize,
                            uint8_t* out, size_t* outsize){

    size_t i;
    size_t total;

    if(!_crypt13_is_init(s)) {
		_deb_intern("not init!");
		return e13_error(E13_MISUSE);
	}

    _deb_info("alg = %u", s->alg);

    switch(s->alg){

        case CRYPT13_ALG_NONE:

            if(out != input) memcpy(out, input, insize);
            *outsize = insize;

        break;

        case CRYPT13_ALG_AES:

            total = s->blocksize*_crypt13_cryptblocks(s, insize);

            _deb_bound("_crypt13_cryptblocks() = %u",
						_crypt13_cryptblocks(s, insize));

            for(i = 0; i < total; i += s->blocksize){
                _deb_bound("i = %u", i);
                aes_encrypt((aes_context*)s->ctx, input + i, out + i);
            }

            *outsize = i;

            _deb_bound("*outsize = %u", *outsize);

        break;

        case CRYPT13_ALG_DES3:

            total = s->blocksize*_crypt13_cryptblocks(s, insize);

            for(i = 0; i < total; i += s->blocksize){
                des3_encrypt((des3_context*)s->ctx, input + i, out + i);
            }

            *outsize = i;

            _deb_bound("*outsize = %u", *outsize);

        break;

        case CRYPT13_ALG_NTRU:

            *outsize = ntru_enc_len(((struct NtruEncParams*)s->params)->N,
									((struct NtruEncParams*)s->params)->q);

            if (ntru_encrypt((char*)input, insize,
							&((NtruEncKeyPair*)s->ctx)->pub,
							(struct NtruEncParams*)s->params,
							dev_urandom, (char*)out) != 0){
                return e13_error(E13_INTERNAL);
            }

        break;
        
        case CRYPT13_ALG_BASE64:
        	if(base64encode(input, insize, out, outsize)){
				return e13_error(E13_TOOBIG);
			}
        	break;

        default:
            return e13_error(E13_IMPLEMENT);
        break;

    }

    return E13_OK;

}

error13_t crypt13_decrypt(  struct crypt13* s, uint8_t* input, size_t insize,
                            uint8_t* out, size_t* outsize){

    size_t i;//, total;

    if(!_crypt13_is_init(s)) {
		_deb_intern("not init!");
		return e13_error(E13_MISUSE);
	}

    switch(s->alg){

        case CRYPT13_ALG_NONE:

            if(out != input) memcpy(out, input, insize);
            *outsize = insize;

        break;

        case CRYPT13_ALG_AES:

            //total = s->blocksize*_crypt13_cryptblocks(s, insize);

            _deb_dec("insize = %u, s->blocksize", insize, s->blocksize);
            for(i = 0; i < insize; i += s->blocksize){
                _deb_dec("i = %u", i);
                aes_decrypt((aes_context*)s->ctx, input + i, out + i);
            }

            *outsize = i;

            _deb_bound("*outsize = %u", *outsize);

        break;

        case CRYPT13_ALG_DES3:

            //total = s->blocksize*_crypt13_cryptblocks(s, insize);

            for(i = 0; i < insize; i += s->blocksize){
                des3_decrypt((des3_context*)s->ctx, input + i, out + i);
            }

            _deb_bound("*outsize = %u", *outsize);

            *outsize = i;

        break;

        case CRYPT13_ALG_NTRU:

        //*outsize = ntru_max_msg_len((struct NtruEncParams*)s->params);
        ntru_decrypt((char*)input, (NtruEncKeyPair*)s->ctx,
					(struct NtruEncParams*)s->params, out, (int*)outsize);

        break;
        
        case CRYPT13_ALG_BASE64:
        	switch(base64decode(input, insize, out, outsize)){
        		case 1:
					return e13_error(E13_TOOBIG);
        		break;
				case 2:
					return e13_error(E13_CORRUPT);
				break;
				default:
				break;
			}
        break;
        	

        default:
            return e13_error(E13_IMPLEMENT);
        break;

    }

    return E13_OK;

}

//the old crypt13_bufsize
size_t crypt13_enc_size(struct crypt13* s, uint8_t* buf, size_t initsize){
	
    if(!_crypt13_is_init(s)) return e13_error(E13_MISUSE);

    switch(s->alg){
        case CRYPT13_ALG_AES:
        case CRYPT13_ALG_DES:
        case CRYPT13_ALG_DES3:
        return s?_crypt13_cryptblocks(s, initsize)*s->blocksize:((size_t)-1);
        break;

    case CRYPT13_ALG_NTRU:
        return ntru_enc_len(((struct NtruEncParams*)s->params)->N,
							((struct NtruEncParams*)s->params)->q);
        break;

	case CRYPT13_ALG_BASE64:
		return base64encode_size(buf, initsize);
		break;

    case CRYPT13_ALG_NONE:
        return initsize;
        break;

    default:
        break;
    }

    return ((size_t)-1);
}

//this just gives you a hint!
size_t crypt13_dec_size(struct crypt13* s, uint8_t* buf, size_t initsize){
	
    if(!_crypt13_is_init(s)) return e13_error(E13_MISUSE);

    switch(s->alg){
    case CRYPT13_ALG_NTRU:
        return ntru_max_msg_len((struct NtruEncParams*)s->params);
        break;
        
    case CRYPT13_ALG_BASE64:
    	return base64decode_size(buf, initsize);
    	break;

    default:
        break;
    }

    return ((size_t)-1);

}

size_t crypt13_export_pub_size(struct crypt13* s){
    if(!_crypt13_is_init(s)) return e13_error(E13_MISUSE);

    switch(s->alg){
    case CRYPT13_ALG_NTRU:
        return ntru_enc_len(((struct NtruEncParams*)s->params)->N,
							((struct NtruEncParams*)s->params)->q);
        break;

    default:
        break;
    }

    return ((size_t)-1);

}

error13_t crypt13_export_pub(struct crypt13* s, char* buf){
    if(!_crypt13_is_init(s)) return e13_error(E13_MISUSE);

    switch(s->alg){
    case CRYPT13_ALG_NTRU:
        ntru_export_pub(&(((NtruEncKeyPair*)s->ctx)->pub), buf);
        return E13_OK;
        break;

    default:
        break;
    }

    return e13_error(E13_IMPLEMENT);
}

error13_t crypt13_import_pub(struct crypt13* s, char* buf, size_t* size){
    if(!_crypt13_is_init(s)) return e13_error(E13_MISUSE);

    switch(s->alg){
    case CRYPT13_ALG_NTRU:
        *size = ntru_import_pub(buf, &(((NtruEncKeyPair*)s->ctx)->pub));
        return E13_OK;
        break;

    default:
        break;
    }

    return e13_error(E13_IMPLEMENT);
}

