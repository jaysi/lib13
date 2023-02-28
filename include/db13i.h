#ifndef DB13I_H
#define DB13I_H

#include "db13.h"
#include "error13.h"

#define MAXTABLENAME 128
#define MAXSQL      1024
#define MAXCOLNAME  32
#define MAXCOLS     15
#define MAXCOLALIAS 40
#define MAXCOLREF   (1 + MAXTABLENAME + 1 + MAXCOLNAME + 1 + MAXCOLNAME)//@Table:KeyCol>ResolveCol

//SQLITE handle and stmt
#define LITE(db13)   ((sqlite3*)((db13))->dbms)
#define LITE_ST(st13) ((sqlite3_stmt*)((st13)->h))
#define DB_ST(st13) ((st13)->db)

struct db_driver {

    char* name;
    enum db_drv_class_id cls;
    char* ext;//extension

} db_drv_class[] = {
    {"null", DB_DRV_CLS_NULL, ""},
    {"sqlite", DB_DRV_CLS_SQL, ".l83"},
    {NULL, DB_DRV_CLS_INVAL, NULL}
};

struct db_logic_sign_s{
    char* sign;
} logic_sign[] = {
    {"LIKE"},
    {"NOT LIKE"},
    {"="},
    {"<>"},
    {"<"},
    {"<="},
    {">"},
    {">="},
    {"BETWEEN"},
    {"AND"},
    {"OR"},
    {"NOT"},
    {"XOR"},
    {"("},
    {")"},
    {NULL}
};

struct db_logic_comb_s{
    char* sign;
} logic_comb[] = {
    {" "},
    {"AND"},
    {"OR"},
    {NULL}
};

#ifdef __cplusplus
    extern "C" {
#endif

error13_t _db_create_table_range(struct db13* db, db_table_id start, db_table_id end);
error13_t _db_assert_logic(int nlogic, struct db_logic_s* logic);

#ifdef __cplusplus
    }
#endif

#endif // DB13I_H
