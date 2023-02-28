#include "include/arg13.h"
#include "include/str13.h"
#include "include/mem13.h"

#include <string.h>
#include <assert.h>

static char* list_packer[3] = {ARG13_ARG_PACKER1, ARG13_ARG_PACKER2, NULL};
//static char* switch_spec[3] = {ARG13_SWITCH_SPEC1, ARG13_SWITCH_SPEC2, NULL};

int _arg13_get_wordid(  struct arg13* a13,
                        char* str,
                        int sensecase,
                        arg13_atype_t type,
                        arg13_aneed_t need,
                        struct arg13_word** word){

    int (*cmp)(const char* s1, const char* s2);
    int i;

    if(sensecase) cmp  = &strcmp;
    else cmp = &strcasecmp;

    for(*word = a13->word; (*word)->desc; (*word)++){
        for(i = 0; (*word)->form[i]; i++){
            if(!cmp((*word)->form[i], str) &&
                ((*word)->type & type) &&
                ((*word)->need & need)) break;
        }
    }

    if(!(*word)) return -1;

    return (*word)->id;
}

error13_t arg13_init(   struct arg13* a13, arg13_atype_t atypes, uint8_t flags,
                        struct arg13_word* word, struct arg13_word_dep* wdep){
    a13->atypes = atypes;
    a13->flags = flags;
    a13->word = word;
    a13->worddep = wdep;
    return e13_init(&a13->err, E13_MAX_WARN_DEF, E13_MAX_ESTR_DEF, LIB13_ARG);
}

static inline error13_t _arg13_get_multi_carg(struct arg13* a13,
                                                char* part[], int nparts,
                                                int* swi){
    struct arg13_word* word;
    int i;
    //try to guess number of command args before first *switch
    for(i = *swi; i < nparts; i++){
        if(_arg13_get_wordid(   a13,
                                part[i],
                                a13->flags & ARG13_SENSE_CASE?1:0,
                                ARG13_ATYPE(ARG13_T_SWITCH),
                                ARG13_ANEED_ALL,
                                &word) < 0){
            break;
        }
    }

    i--;
    assert(i > 0);

    if(i == *swi) return e13_error(E13_FORMAT);

    a13->ncargs = *swi - i;
    a13->cargid = (int*)m13_malloc(a13->ncargs*sizeof(int));
    if(!a13->cargid){
        return e13_ierror(&a13->err, E13_NOMEM, "s", "for a13->cargid");
    }

    a13->carg = s13_exmem(a13->ncargs);
    if(!a13->carg){
        m13_free(a13->cargid);
        return e13_ierror(&a13->err, E13_NOMEM, "s", "for a13->carg");
    }

    //assign command args
    for(i = 0; i < a13->ncargs; i++){
        a13->cargid[i] = _arg13_get_wordid(a13,
                                        part[i + (*swi)],
                                        a13->flags & ARG13_SENSE_CASE?1:0,
                                        ARG13_ATYPE(ARG13_T_CMDARG),
                                        ARG13_ANEED_ALL,
                                        &word);
        a13->carg[i] = part[i + *swi];
    }

    *swi = i;

    return E13_OK;
}

static inline error13_t _arg13_get_single_carg(struct arg13* a13,
                                                char* part[],
                                                int* swi){
    struct arg13_word* word;
    a13->ncargs = 1;
    a13->cargid = (int*)m13_malloc(a13->ncargs*sizeof(int));
    if(!a13->cargid){
        return e13_ierror(&a13->err, E13_NOMEM, "s", "for a13->cargid");
    }

    a13->carg = s13_exmem(a13->ncargs);
    if(!a13->carg){
        m13_free(a13->cargid);
        return e13_ierror(&a13->err, E13_NOMEM, "s", "for a13->carg");
    }

    a13->cargid[0] =  _arg13_get_wordid(a13,
                                        part[*swi],
                                        a13->flags & ARG13_SENSE_CASE?1:0,
                                        ARG13_ATYPE(ARG13_T_CMDARG),
                                        ARG13_ANEED_ALL,
                                        &word);
    a13->carg[0] = part[*swi];

    (*swi)++;

    return E13_OK;
}

error13_t arg13_parse(char *arg, struct arg13 *a13){

    int nparts, swi, i;
    char** part;
    struct arg13_word* word;
    error13_t ret;
    int sensecase = a13->flags & ARG13_SENSE_CASE?1:0;

    e13_cleanup(&a13->err);

    //allocate
    nparts = s13_exparts(arg, ARG13_SEP, list_packer, ARG13_ARG_ESCAPE);
    if(!nparts){
        return e13_uerror(&a13->err, E13_EMPTY, "s", "empty string");
    }
    part = s13_exmem(nparts);
    nparts = s13_explode(arg, ARG13_SEP, list_packer, ARG13_ARG_ESCAPE, part);

    //init
    swi = 0;

    if(a13->atypes & ARG13_ATYPE(ARG13_T_CMD)){

        //extract command
        a13->cmdid = _arg13_get_wordid( a13,
                                        part[0],
                                        sensecase,
                                        ARG13_ATYPE(ARG13_T_CMD),
                                        ARG13_ANEED_ALL,
                                        &word);
        if(a13->cmdid < 0){
            return e13_uerror(&a13->err, E13_NOTFOUND, "sss", "command [ ", part[0] ," ] not found");
        }
        swi++;

        //extract command arguments
        if(a13->atypes & ARG13_ATYPE(ARG13_T_CMDARG)){//de command may have args
            if( (word->need & ARG13_ANEED(ARG13_AN_NEED)) &&
                !(word->need & ARG13_ANEED(ARG13_AN_NONEED))){//really need arg

                if(word->need & ARG13_AN_MULTI){
                    ret = _arg13_get_multi_carg(a13, part, nparts, &swi);
                } else {//the command needs an argument other than multiword
                    ret = _arg13_get_single_carg(a13, part, &swi);
                }

                if(ret != E13_OK) return ret;

            } else if(  !(word->need & ARG13_ANEED(ARG13_AN_NEED)) &&
                        (word->need & ARG13_ANEED(ARG13_AN_NONEED))){//no args
                  a13->ncargs = 0;
            }
        } else {//no arguments needed SECAM!
            a13->ncargs = 0;
        }
    }//if(command_needed)

    if(a13->atypes & ARG13_ATYPE(ARG13_T_SWITCH)){//may have switches

        //guess number of switches to allocate memmory
        i = nparts - swi;

        /*this guess was more accurate... and much more time consuming!
        for(i = swi; i < nparts; i++)
            if( _arg13_get_wordid(  a13,
                                    part[i],
                                    sensecase,
                                    ARG13_ATYPE(ARG13_T_SWITCH),
                                    ARG13_ANEED_ALL,
                                    &word) > 0) a13->nswitches++;
        */

        //allocate memmory for switches
        a13->swid = (int*)m13_malloc(i*sizeof(int));
        if(!a13->swid){
            return e13_ierror(&a13->err, E13_NOMEM, "s", "a13->swid");
        }
        a13->swarg = s13_exmem(i);
        if(!a13->swarg){
            m13_free(a13->swid);
            return e13_ierror(&a13->err, E13_NOMEM, "s", "a13->swarg");
        }

        word = NULL;
        a13->nswitches = 0;
        while(swi < nparts){
            if(!word){

                if( _arg13_get_wordid(  a13,
                                        part[swi],
                                        sensecase,
                                        ARG13_ATYPE(ARG13_T_SWITCH),
                                        ARG13_ANEED_ALL,
                                        &word) < 0){
                    return e13_uerror(&a13->err, E13_NOTFOUND, "sss", "switch [ ", part[swi] ," ] not found");
                }

                a13->swid[a13->nswitches++] = word->id;
                swi++;

                /*resolve switch arg*/
            } else if(a13->atypes & ARG13_ATYPE(ARG13_T_SWITCHARG)){
                if( word->need & ARG13_ANEED(ARG13_AN_NEED) &&
                    !(word->need & ARG13_ANEED(ARG13_AN_NONEED))){//need args

                    if(swi == nparts-1){
                        return e13_uerror(&a13->err, E13_NOTVALID, "sss", "switch [ ", part[swi-1] ," ] needs an argument");
                    }

                    a13->swarg[a13->nswitches-1] = part[swi++];

                } else if(  word->need & ARG13_ANEED(ARG13_AN_NONEED) &&
                            !(word->need & ARG13_ANEED(ARG13_AN_NEED))){//noargs

                    a13->swarg[a13->nswitches-1] = NULL;

                } else if(  (word->need & ARG13_ANEED(ARG13_AN_NONEED) &&
                            (word->need & ARG13_ANEED(ARG13_AN_NEED))) ||
                            !(word->need & ARG13_ANEED(ARG13_AN_NONEED) &&
                            !(word->need & ARG13_ANEED(ARG13_AN_NEED)))
                            ){//could have

                    if(swi > nparts-1){
                        if( _arg13_get_wordid(  a13,
                                                part[swi],
                                                sensecase,
                                                ARG13_ATYPE(ARG13_T_SWITCH),
                                                ARG13_ANEED_ALL,
                                                &word) < 0){//my best guess again
                            a13->swarg[a13->nswitches-1] = part[swi++];
                        } else {//not an argument
                            a13->swarg[a13->nswitches-1] = NULL;
                        }
                    } else {//no more parts
                        a13->swarg[a13->nswitches-1] = NULL;
                    }
                }

                word = NULL;
            }//if(word)
        }//while
    }//if(atype_switch)

    return E13_OK;
}

int arg13_wordseq(struct arg13* a13, struct arg13_word_seq* seqlist){
    int i;
    int j;
    int wrong;
    struct arg13_word_seq* seq;

    j = 0;
    for(seq = seqlist; seq->seqid; seq++, j++){

        if(seq->seqid[0] != a13->cmdid) continue;

        for(i = 0; i < a13->ncargs; i++){
            if(seq->seqid[i+1] < 0 || a13->cargid[i] != seq->seqid[i+1]){
                wrong = 1;
                break;
            }
        }

        if(!wrong) return j;
        else wrong = 0;
    }

    return -1;
}

error13_t arg13_parse_array(int argc, char *argv[], struct arg13 *a13){
    return e13_error(E13_IMPLEMENT);
}

