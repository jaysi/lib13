#ifndef DB13_H
#define DB13_H

#include "../../sqlite/sqlite3.h"
#include "type13.h"
#include "const13.h"
#include "error13.h"
#include "obj13.h"

/***            SOME DBMS RELATED DEFS          ***/
enum db_drv_id {
    DB_DRV_NULL,
    DB_DRV_SQLITE,
    DB_DRV_INVAL
};

enum db_drv_class_id {
    DB_DRV_CLS_NULL,
    DB_DRV_CLS_PRIV,
    DB_DRV_CLS_SQL,
    DB_DRV_CLS_INVAL
};

/***            SOME DBMS RELATED DEFS          ***/

//struct db_db_s{

//    char* name;
//    char* path;
//    db_table_id start;
//    db_table_id end;

//};

typedef uint16_t db_colid_t;
#define DB_COLID_INVAL ((db_colid_t)-1)

typedef uint32_t db_rowid_t;
#define DB_ROWID_INVAL ((db_rowid_t)-1)
#define DB_ROWID_ZERO   0UL

typedef uint64_t db_recid_t;
#define DB_RECID_INVAL ((db_recid_t)-1)

typedef uint64_t db_fieldid_t;
#define DB_FIELDID_INVAL ((db_fieldid_t)-1)

typedef uint8_t db_logicflag_t;
typedef uint16_t db_colflag_t;
typedef uint16_t db_tableflag_t;
typedef uint32_t db_table_id;
#define DB_TID_INVAL   ((db_table_id)-1)


#define DB_STR_EMPTY  ""

#define DB_DEF_DRIVER   DB_DRV_SQLITE
#ifdef WIN32
#define DB_DEF_HOSTNAME "localhost"
#else
#define DB_DEF_HOSTNAME "localhost.localdomain"
#endif
#define DB_DEF_PORT     "65500"
#define DB_DEF_NAME     "db.13"
#define DB_DEF_USERNAME ""
#define DB_DEF_PASSWORD ""

enum db_colpref_t {
    DB_COLPREF_EMPTY,
    DB_COLPREF_NAME,
    DB_COLPREF_TYPE,
    DB_COLPREF_ALIAS,
    DB_COLPREF_REF,
    DB_COLPREF_FLAGS,
    DB_COLPREF_INVAL
};

#define DB_TABLEF_EMPTY   (0x0001<<0)
#define DB_TABLEF_CORE    (0x0001<<1)//core table, the system designer's backyard!
#define DB_TABLEF_SYS     (0x0001<<2)//system table, tables like wpt ...
#define DB_TABLEF_CTL     (0x0001<<3)//control table, for administration
#define DB_TABLEF_ENCRYPT (0x0001<<4)//table encrypted
#define DB_TABLEF_MALLOC  (0x0001<<5)//load table in memmory, e.g. info
#define DB_TABLEF_VIRTUAL (0x0001<<6)//virtual table, not on disk
#define DB_TABLEF_WRLOCK  (0x0001<<7)
#define DB_TABLEF_RDLOCK  (0x0001<<8)

struct db_table_info{

    db_tableflag_t flags;

    char* name;
    char* alias;

    db_colid_t ncols;

    char** col_name;
    char** col_type;
    char** col_alias;
    char** col_ref;
    db_colflag_t* col_flags;

    char* tmpbuf;

    //struct db_table_info* next;

};

#define DB_FLAG_OPEN (0x01<<0)
#define DB_DEF_FLAGS 0x00

struct db13{

    magic13_t magic;
    uint8_t flags;

    enum db_drv_id driver;
    char* host;
    char* port;
    char* username;
    char* password;

    char* name;

    void* dbms;

    db_table_id ntableslots;
    db_table_id ntables;
    struct db_table_info* table_info;

    struct e13 e;
    
    th13_mutex_t table_lock_mx;

    struct db13* next;

};

//locks
typedef char db_lock_t;
#define DB_LOCK_RD	(0x01<<1)
#define DB_LOCK_WR	(0x01<<2)

#define DB_COLF_AUTO       (0x0001<<0)
#define DB_COLF_HIDE       (0x0001<<1)
#define DB_COLF_TRANSL     (0x0001<<2)//translated
#define DB_COLF_SINGULAR   (0x0001<<3)
#define DB_COLF_ONETIMEFIX (0x0001<<4)//defined once!
#define DB_COLF_LIST       (0x0001<<5)
#define DB_COLF_VIRTUAL    (0x0001<<6)
#define DB_COLF_ACCEPTNULL (0x0001<<7)
#define DB_COLF_ENCRYPT    (0x0001<<8)
#define DB_COLF_END        0

#define DB_STMT_MAGIC 0x1bac

struct db_stmt{
    unsigned short magic;
    void* h;
    struct db13* db;
};

#define DB_T_BOOL "BOOL"
#define DB_T_INT  "INTEGER"
#define DB_T_BIGINT  "BIGINT"
#define DB_T_REAL  "REAL"
#define DB_T_TEXT  "TEXT"
#define DB_T_DATE  "DATE"
#define DB_T_D13STIME  "D13STIME"
#define DB_T_RAW    "BLOB"
#define DB_T_INVAL  "INVAL"

/* moved to obj13.h
enum db_type_id{
    DB_TY_BOOL,
    DB_TY_INT,
    DB_TY_BIGINT,
    DB_TY_REAL,
    DB_TY_TEXT,
    DB_TY_DATE,
    DB_TY_RAW,
    DB_TY_INVAL
};
*/

enum db_sort{

    DB_SO_DONT,
    DB_SO_DEC,
    DB_SO_INC //increase, from small to large!

};

enum db_logic_comb{
    DB_LOGICOMB_NONE,
    DB_LOGICOMB_AND,
    DB_LOGICOMB_OR,
    DB_LOGICOMB_INVAL
};

enum db_logic{

    DB_LOGIC_LIKE,
    DB_LOGIC_NOTLIKE,
    DB_LOGIC_EQ,
    DB_LOGIC_NE,
    DB_LOGIC_LT,
    DB_LOGIC_LE,
    DB_LOGIC_GT,
    DB_LOGIC_GE,
    DB_LOGIC_BETWEEN,
    DB_LOGIC_AND,
    DB_LOGIC_OR,
    DB_LOGIC_NOT,
    DB_LOGIC_XOR,
    DB_LOGIC_OPENP,
    DB_LOGIC_CLOSEP,

    DB_LOGIC_INVAL
};

#define DB_LOGICF_COL_CMP	(0x01<<0) //??? compare columns
#define DB_LOGICF_USE_COLID	(0x01<<2)//
#define DB_LOGICF_DEF		(0x00)

struct db_logic_s{
    char flags;
    //enum db_logic_comb comb;
    db_colid_t colid;
    char* colname;
    enum db_logic logic;
    void* value;//not in use for now
    char* sval;
    int64_t ival;// = size of sval in blob data type
};
/*
struct db_field {
	char* colname;
	db_colid_t colid;
	db_rowid_t rowid;
	db_fieldid_t recid;
	db_type_id type;//this is for faster trans, you can always ask datatype from column
    void* val;
    struct db_record* next;
};

struct db_logic2 {
	db_logicflag_t flags;
	struct db_field* field;
    enum db_logic logic;
    struct db_logic2* next;
};
*/
#ifdef __cplusplus
    extern "C" {
#endif

#define db_isinit(db) ((db)->magic == MAGIC13_DB13?true_:false_)
#define db_isopen(db) (((db)->flags&DB_FLAG_OPEN)?true_:false_)

error13_t db_init(struct db13* db,
                  enum db_drv_id driver);

error13_t db_open(struct db13* db,
                     char* host,
                     char* port,
                     char* username,
                     char* password,
                     char *name);

error13_t db_destroy(struct db13* db);

error13_t db_close(struct db13* db);
db_table_id db_get_tid_byname(struct db13* db, char* name);
db_table_id db_get_tid_byalias(struct db13* db, char* alias);
char* db_get_table_name(struct db13* db, db_table_id tid);
char* db_get_table_alias(struct db13* db, db_table_id tid);
db_colid_t db_get_colid_byname(	struct db13* db, db_table_id tid,
								char* col_name);
db_colid_t db_get_colid_byalias(struct db13* db, db_table_id tid,
								char* col_alias);
char* db_get_col_name(struct db13* db, db_table_id tid, db_colid_t colid);
char* db_get_col_alias(struct db13* db, db_table_id tid, db_colid_t colid);
db_colid_t db_col_count(struct db13* db, db_table_id tid);
void db_set_colflag(struct db13* db, db_table_id tid, db_colid_t colid,
					db_colflag_t flags);
void db_unset_colflag(struct db13* db, db_table_id tid, db_colid_t colid,
					db_colflag_t flags);
db_colflag_t db_get_colflag(struct db13* db, db_table_id tid,
							db_colid_t colid);
db_colid_t db_count_colflag(struct db13* db, db_table_id tid,
							db_colflag_t flags);
enum db_type_id db_coltype(struct db13* db, db_table_id tid,
						db_colid_t colid);
char* db_coltype_name(struct db13* db, db_table_id tid,
					db_colid_t colid);

error13_t db_step(struct db_stmt* st);
error13_t db_finalize(struct db_stmt* st);
error13_t db_reset(struct db_stmt* st);
error13_t db_begin_trans(struct db13* db);
error13_t db_commit_trans(struct db13* db);
error13_t db_rollback_trans(struct db13* db);
error13_t db_create_table(struct db13* db, db_table_id tid);
error13_t db_trunc_table(struct db13* db, db_table_id tid);
error13_t db_rm_table(struct db13* db, db_table_id tid);

error13_t db_column_text(struct db_stmt* st, db_colid_t col,
						size_t* len, unsigned char** text);
error13_t db_column_date(struct db_stmt* st, db_colid_t col, int date[3]);
error13_t db_column_int(struct db_stmt* st, db_colid_t col, int* val);
error13_t db_column_int64(struct db_stmt* st, db_colid_t col, int64_t* val);
error13_t db_column_double(struct db_stmt* st, db_colid_t col, double* val);
error13_t db_column_size(struct db_stmt* st, db_colid_t col, size_t* size);
error13_t db_column_raw(struct db_stmt* st, db_colid_t col,
						size_t* datalen, void** data);

error13_t db_select_all(struct db13* db, db_table_id tid, struct db_stmt* st);
error13_t db_insert(struct db13* db, db_table_id tid,
					uchar **col, size_t* size, struct db_stmt* st);
error13_t db_update_col_byobjid(struct db13* db, db_table_id tid,
								objid13_t objid, db_colid_t col,
								uchar *val, size_t size, struct db_stmt* st);
error13_t db_update(struct db13* db, db_table_id tid,
					struct db_logic_s iflogic,
					db_colid_t ncol, db_colid_t* col,
					uchar** val, size_t* size,
					struct db_stmt* st);
error13_t db_delete(struct db13* db, db_table_id tid,
					int nlogic, struct db_logic_s* logic,
					struct db_stmt* st);

error13_t db_collect(	struct db13* db, db_table_id tid,
						char** cols,
						int nlogic, struct db_logic_s* logic,
						char* sortcol, enum db_sort stype, int nlimit,
						struct db_stmt* st);

error13_t db_count(	struct db13* db, db_table_id tid,
					int nlogic, struct db_logic_s* logic,
					db_rowid_t* nrows);

error13_t db_set_table_slots(struct db13* db, db_table_id ntables);
error13_t db_get_table_slots(struct db13* db, db_table_id* ntables);
error13_t db_get_full_table_slots(struct db13* db, db_table_id* ntables);
/*
 *... is the list of columns followed by their properties, comes in order
 * name(char*), type(char*), alias(char*), ref(char*), flags(_colflag_t)
*/
error13_t db_define_table(struct db13* db, char* name, char* alias,
						db_tableflag_t flags, db_colid_t ncols, ...);
error13_t db_undef_table(struct db13* db, char* name);
error13_t db_istable_physical(struct db13* db, char* name);//check for table existance on disk, returns CONTINUE if table does not exists on disk but there is no other errors

error13_t db_resolve_ref(struct db13* db, char* ref, char* from, char** to);//resolves ref, e.g. uid to username
error13_t db_unresolve_ref(struct db13* db, char* ref, char* from, char** to);//unresolves ref!, e.g. username to uid
error13_t db_create_reflist(struct db13* db, char* ref, char** list);//creates refrence list, e.g. user list

error13_t db_lock_table(struct db13* db, db_table_id tid, db_lock_t lock);
error13_t db_unlock_table(struct db13* db, db_table_id tid, db_lock_t lock);

#ifdef __cplusplus
    }
#endif


#endif // DB_H
