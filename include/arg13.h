#ifndef ARG13_H
#define ARG13_H

#include "type13.h"
#include "error13.h"

#define ARG13_SEP           " "
#define ARG13_LIST_SEP      ";"
#define ARG13_ARG_PACKER1   "\""
#define ARG13_ARG_PACKER2   "'"
#define ARG13_ARG_ESCAPE    '\\'
#define ARG13_SWITCH_SPEC1  "-"
#define ARG13_SWITCH_SPEC2  "--"

typedef uint8_t arg13_aneed_t;
typedef uint8_t arg13_atype_t;

enum arg13_arg_needs {//accepted argument types
    ARG13_AN_NONEED,//no args needed
    ARG13_AN_NEED,//args needed; noneed == need ? maybe: the one that set.
    ARG13_AN_WORD,//word args are accepted
    ARG13_AN_MULTI,//multiple separated args are accepted, not allowed for switches for now
    ARG13_AN_LIST,//list args are ok
    ARG13_AN_OTHER//other types are ok
};

#define ARG13_ANEED(arg_n) (0x01<<(arg_n))
#define ARG13_ANEED_ALL 0xff

enum arg13_arg_types {
    ARG13_T_CMD,//the word could be a command
    ARG13_T_CMDARG,//the word could be a command arg
    ARG13_T_SWITCH,//could be a switch
    ARG13_T_SWITCHARG,//could be a switch arg
    ARG13_T_OTHER//is acceptable
};

#define ARG13_ATYPE(arg_t) (0x01<<(arg_t))
#define ARG13_ATYPE_ALL 0xff

/* an arg13 string: [Command] [CommandArgumentList] [-Switches [SwitchArgumentList]]
 * . the lists are ; separated
 * . you may use separators inside the argument body using " or '
 */

struct arg13_word{
    int id;
    arg13_aneed_t need;//arg
    arg13_atype_t type;//possible types of this word
    char** form;//word appearance forms
    char* desc;
};

struct arg13_word_seq{
    int* seqid;//sequence, ends with -1
    void* call;
    void* callarg;
};

#define ARG13_SENSE_CASE    (0x01<<0)

struct arg13{
    uint8_t flags;

    arg13_atype_t atypes;//supported arg types in this request

    struct arg13_word* word;
    struct arg13_word_dep* worddep;

    int cmdid;

    int ncargs;
    int* cargid;
    char** carg;

    int nswitches;
    int* swid;
    char** swarg;

    struct e13 err;
};

error13_t arg13_init(struct arg13* a13, arg13_atype_t atypes, uint8_t flags,
                     struct arg13_word* word, struct arg13_word_dep* wdep);
error13_t arg13_parse(char* arg, struct arg13* a13);
error13_t arg13_parse_array(int argc, char* argv[], struct arg13* a13);
int arg13_wordseq(struct arg13* a13, struct arg13_word_seq* seqlist);

#endif // ARG13_H
