//TODO: VERY IMPORTANT! IGNORE FAKE COLS in physical acts!!!

#include <string.h>
#include "include/db13i.h"
#include "include/msg13.h"
#include "include/mem13.h"
#include "include/str13.h"
#include "include/bit13.h"
#include "include/day13.h"

//#define NDEBUG
#include "include/debug13.h"

#define _deb_def_table _NullMsg
#define _deb_create_table _NullMsg
#define _deb_get_tid_byname _NullMsg
#define _deb_insert _NullMsg
#define _deb_collect _NullMsg
#define _deb_delete _NullMsg
#define _deb_update _DebugMsg
#define _deb_close	_NullMsg
#define _deb_collect_final _NullMsg


#define ACC_UP_IF_COL 900

//enum db_id db_id_byname(char* name){

//    enum db_id id;

//    for(id = DB_DB_EMPTY; id < DB_DB_INVAL; id++){
//        if(!strcmp(name, db_db[id].name)) return id;
//    }

//    return DB_DB_INVAL;
//}

error13_t db_init(struct db13* db,
                  enum db_drv_id driver){

	th13_mutexattr_t mxattr;

    if(db_isinit(db)){
        return e13_ierror(&db->e, E13_MISUSE, "s", msg13(M13_ALREADYINIT));
    }

    if(e13_init(&db->e, E13_MAX_WARN_DEF, E13_MAX_ESTR_DEF, LIB13_NMDB) != E13_OK){
        return e13_error(E13_SYSE);
    }

    db->driver = driver;

    db->name = NULL;
    db->host = NULL;
    db->port = NULL;
    db->flags = DB_DEF_FLAGS;
    db->username = NULL;
    db->password = NULL;
    db->dbms = NULL;
    db->ntableslots = 0UL;
    db->ntables = 0UL;
    db->table_info = NULL;
    //db->table_lock_mx = PTHREAD_MUTEX_INITIALIZER;

    th13_mutexattr_init(&mxattr);
    th13_mutex_init(&db->table_lock_mx, &mxattr);
    th13_mutexattr_destroy(&mxattr);


    db->magic = MAGIC13_DB13;

    return E13_OK;

}

error13_t db_destroy(struct db13* db){

    if(!db_isinit(db)) return e13_error(E13_MISUSE);
    if(db_isopen(db)) return e13_error(E13_MISUSE);
    db->magic = MAGIC13_INV;
    //TODO: a very bad work-around
    db->flags = 0;
    th13_mutex_destroy(&db->table_lock_mx);

    return E13_OK;
}

error13_t db_open(struct db13 *db,
                     char *host,
                     char *port,
                     char *username,
                     char *password,
                     char *name){

    sqlite3* sqlite;

    size_t hlen, plen, ulen, slen, nlen;

//    int do_create = access(db_path(name), F_OK)?1:0;

    if(!db_isinit(db)) return e13_error(E13_MISUSE);
    if(db_isopen(db)) return e13_error(E13_MISUSE);

    switch(db->driver){
    case DB_DRV_NULL:
        db->dbms = NULL;
        break;
    case DB_DRV_SQLITE:

        if(sqlite3_open(name, &sqlite) != SQLITE_OK){
            return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(sqlite));
        }

        db->dbms = sqlite;

        break;
    default:
        return e13_ierror(&db->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;
    }

    hlen = strlen(host?host:DB_DEF_HOSTNAME);
    plen = strlen(port?port:DB_DEF_PORT);
    ulen = strlen(username?username:DB_DEF_USERNAME);
    slen = strlen(password?password:DB_DEF_PASSWORD);
    nlen = strlen(name?name:DB_DEF_NAME);

    db->name = (char*)m13_malloc(hlen + plen + ulen + slen + nlen + 5);
    if(!db->name){
        db_close(db);
        return e13_ierror(&db->e, E13_NOMEM, "s", msg13(M13_NOMEM));
    }

    db->host = db->name + nlen + 1;
    db->port = db->host + hlen + 1;
    db->username = db->port + plen + 1;
    db->password = db->username + ulen + 1;

    strcpy(db->name, name?name:DB_DEF_NAME);
    strcpy(db->host, host?host:DB_DEF_HOSTNAME);
    strcpy(db->port, port?port:DB_DEF_PORT);
    strcpy(db->username, username?username:DB_DEF_USERNAME);
    strcpy(db->password, password?password:DB_DEF_PASSWORD);

    db->flags |= DB_FLAG_OPEN;

    return E13_OK;
}

error13_t db_close(struct db13 *db){

    db_table_id tid;

    if(!db_isinit(db)) return e13_error(E13_MISUSE);
	if(!db_isopen(db)) return e13_error(E13_MISUSE);

    switch(db->driver){
    case DB_DRV_NULL:

        break;
    case DB_DRV_SQLITE:
        sqlite3_close(LITE(db));
        break;
    default:
        return e13_ierror(&db->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;
    }

	_deb_close("ntableslots %lu, ntables %lu", db->ntableslots, db->ntables);
    for(tid = 0; tid < db->ntableslots; tid++){
        _deb_close("tid %lu", tid);
        if(!(db->table_info[tid].flags & DB_TABLEF_EMPTY)){
        	_deb_close("name %s", db->table_info[tid].name);
            db_undef_table(db, db->table_info[tid].name);
        }
    }

    if(db->table_info) m13_free(db->table_info);
    db->ntableslots = 0UL;
    db->ntables = 0UL;

    if(db->name) free(db->name);

    db->flags &= ~DB_FLAG_OPEN;

//    db->magic = MAGIC13_INV;

    return E13_OK;

}

static inline enum db_drv_class_id _db_get_drv_cls(enum db_drv_id id){
    return db_drv_class[id].cls;
}

error13_t db_set_table_slots(struct db13* db, db_table_id ntables){
    db_table_id noldtableslots = db->ntableslots;

    if(!db_isinit(db)){
        return e13_error(E13_MISUSE);
    }

    if(db->ntableslots >= ntables) return e13_error(E13_IMPLEMENT);
    if((db->table_info = (struct db_table_info*)m13_realloc(db->table_info, sizeof(struct db_table_info)*ntables))){
        db->ntableslots = ntables;

        for(; noldtableslots < db->ntableslots; noldtableslots++){
            db->table_info[noldtableslots].flags = DB_TABLEF_EMPTY;
        }

        return E13_OK;
    }
    return e13_error(E13_NOMEM);
}

error13_t db_get_table_slots(struct db13* db, db_table_id* ntables){

    if(!db_isinit(db)){
        *ntables = 0;
        return e13_error(E13_MISUSE);
    }

    *ntables = db->ntableslots;

    return E13_OK;
}

error13_t db_get_full_table_slots(struct db13* db, db_table_id* ntables){

    db_table_id noldtableslots;
    *ntables = 0;

    if(!db_isinit(db)){
        return e13_error(E13_MISUSE);
    }

    for(noldtableslots = 0; noldtableslots < db->ntableslots; noldtableslots++){
        if(!(db->table_info[noldtableslots].flags & DB_TABLEF_EMPTY)){
            (*ntables)++;
        }
    }

    return E13_OK;
}


error13_t db_define_table(struct db13 *db, char *name, char *alias, db_tableflag_t flags, db_colid_t ncols, ...){
    va_list ap;
    db_colid_t col;
    enum db_colpref_t colpref;
    db_table_id tid;
    char* s;
    size_t len;
    db_colflag_t col_flag;

    if(!db_isinit(db)) return e13_error(E13_MISUSE);

    _deb_def_table("running define table... name = %s, alias = %s, flags = %i, ncols = %u", name, alias, flags, ncols);


    //define table if possible!

    if(db->ntables == db->ntableslots){
        return e13_error(E13_FULL);
    }

    for(tid = 0; tid < db->ntableslots; tid++){
        if(db->table_info[tid].flags & DB_TABLEF_EMPTY){
            break;
        }
    }

    _deb_def_table("ntableslots = %u, tid = %u", db->ntableslots, tid);

    db->table_info[tid].name = (char*)m13_malloc(strlen(name) + 1);
    db->table_info[tid].alias = (char*)m13_malloc(strlen(alias) + 1);

    strcpy(db->table_info[tid].name, name);
    strcpy(db->table_info[tid].alias, alias);
    db->table_info[tid].ncols = ncols;

    db->table_info[tid].col_name = (char**)m13_malloc(ncols*sizeof(char*));
    db->table_info[tid].col_type = (char**)m13_malloc(ncols*sizeof(char*));
    db->table_info[tid].col_alias = (char**)m13_malloc(ncols*sizeof(char*));
    db->table_info[tid].col_ref = (char**)m13_malloc(ncols*sizeof(char*));
    db->table_info[tid].col_flags = (db_colflag_t*)m13_malloc(ncols*sizeof(db_colflag_t));

    va_start(ap, ncols);

    for(col = 0; col < ncols; col++) {

        _deb_def_table("col = %u", col);

        for(colpref = DB_COLPREF_EMPTY + 1; colpref < DB_COLPREF_INVAL; colpref++){

            _deb_def_table("colpref = %u", colpref);

            switch(colpref){

            case DB_COLPREF_NAME:

                s = va_arg(ap, char*);
                if(!s) s = DB_STR_EMPTY;
                _deb_def_table("name = %s", s);
                len = strlen(s) + 1;
                db->table_info[tid].col_name[col] = (char*)m13_malloc(len);
                s13_strcpy(db->table_info[tid].col_name[col], s, len);
                _deb_def_table("name = %s", db->table_info[tid].col_name[col]);

                break;

            case DB_COLPREF_TYPE:

                s = va_arg(ap, char*);
                if(!s) s = DB_STR_EMPTY;
                _deb_def_table("type = %s", s);
                len = strlen(s) + 1;
                db->table_info[tid].col_type[col] = (char*)m13_malloc(len);
                s13_strcpy(db->table_info[tid].col_type[col], s, len);
                _deb_def_table("type = %s", db->table_info[tid].col_type[col]);

                break;

            case DB_COLPREF_ALIAS:

                s = va_arg(ap, char*);
                if(!s) s = DB_STR_EMPTY;
                _deb_def_table("alias = %s", s);
                len = strlen(s) + 1;
                db->table_info[tid].col_alias[col] = (char*)m13_malloc(len);
                s13_strcpy(db->table_info[tid].col_alias[col], s, len);
                _deb_def_table("alias = %s", db->table_info[tid].col_alias[col]);

                break;

            case DB_COLPREF_REF:

                s = va_arg(ap, char*);
                if(!s) s = DB_STR_EMPTY;
                _deb_def_table("ref = %s", s);
                len = strlen(s) + 1;
                db->table_info[tid].col_ref[col] = (char*)m13_malloc(len);
                s13_strcpy(db->table_info[tid].col_ref[col], s, len);
                _deb_def_table("ref = %s", db->table_info[tid].col_ref[col]);

                break;

            case DB_COLPREF_FLAGS:

                col_flag = va_arg(ap, int);
                _deb_def_table("flags = %i", col_flag);
                db->table_info[tid].col_flags[col] = col_flag;
                _deb_def_table("flags = %i", db->table_info[tid].col_flags[col]);

                break;

            default:
                break;

            }

        }
    }

    va_end(ap);

    //if everything's ok
    db->table_info[tid].flags = flags;
    //TODO:here could be a flags &= ~FLAG_EMPTY
    db->ntables++;

    return E13_OK;
}

error13_t db_undef_table(struct db13 *db, char *name){
    db_table_id tid;
    db_colid_t cid;
    if(!db_isinit(db)) return e13_error(E13_MISUSE);
    for(tid = 0; tid < db->ntableslots; tid++){
        if(!(db->table_info[tid].flags & DB_TABLEF_EMPTY) && !strcmp(db->table_info[tid].name, name)){

            m13_free(db->table_info[tid].name);
            m13_free(db->table_info[tid].alias);

            db->table_info[tid].name = DB_STR_EMPTY;

            for(cid = 0; cid < db->table_info[tid].ncols; cid++){
                if(db->table_info[tid].col_name[cid] && strlen(db->table_info[tid].col_name[cid])) m13_free(db->table_info[tid].col_name[cid]);
                if(db->table_info[tid].col_alias[cid] && strlen(db->table_info[tid].col_alias[cid])) m13_free(db->table_info[tid].col_alias[cid]);
                if(db->table_info[tid].col_ref[cid] && strlen(db->table_info[tid].col_ref[cid])) m13_free(db->table_info[tid].col_ref[cid]);
                if(db->table_info[tid].col_type[cid] && strlen(db->table_info[tid].col_type[cid])) m13_free(db->table_info[tid].col_type[cid]);
                db->table_info[tid].col_flags[cid] = 0;
            }

            m13_free(db->table_info[tid].col_name);
            m13_free(db->table_info[tid].col_alias);
            m13_free(db->table_info[tid].col_ref);
            m13_free(db->table_info[tid].col_type);
            m13_free(db->table_info[tid].col_flags);

        } else {
            return e13_error(E13_MISUSE);
        }
    }

    return E13_OK;
}

db_table_id db_get_tid_byname(struct db13 *db, char* name){
    db_table_id tid;
    _deb_get_tid_byname("name = %s", name);
    for(tid = 0; tid < db->ntables; tid++){
        _deb_get_tid_byname("tid = %u, name = %s", tid, db->table_info[tid].name);
        if(!strcmp(name, db->table_info[tid].name)) return tid;
    }

    return DB_TID_INVAL;
}

db_table_id db_get_tid_byalias(struct db13 *db, char* alias){
    db_table_id tid;
    for(tid = 0; tid < db->ntables; tid++){
        if(!strcmp(alias, db->table_info[tid].alias)) return tid;
    }

    return DB_TID_INVAL;
}

char* db_get_table_name(struct db13 *db, db_table_id tid){
    assert(tid < db->ntableslots);
    return db->table_info[tid].name;
}

char* db_get_table_alias(struct db13 *db, db_table_id tid){
    assert(tid < db->ntableslots);
    return db->table_info[tid].alias;
}

db_colid_t db_get_colid_byname(struct db13 *db, db_table_id tid, char* col_name){
    db_colid_t colid;

    assert(tid < db->ntableslots);

    for(colid = 0; colid < db->table_info[tid].ncols; colid++){
        if(!strcmp(db->table_info[tid].col_name[colid], col_name)) return colid;
    }

    return DB_COLID_INVAL;
}

db_colid_t db_get_colid_byalias(struct db13* db, db_table_id tid, char* col_alias){
    db_colid_t colid;

    assert(tid < db->ntableslots);
    for(colid = 0; colid < db->table_info[tid].ncols; colid++){
        if(!strcmp(db->table_info[tid].col_alias[colid], col_alias)) return colid;
    }

    return DB_COLID_INVAL;
}

char* db_get_col_name(struct db13* db, db_table_id tid, db_colid_t colid){
    assert(tid < db->ntableslots);
    assert(colid < db->table_info[tid].ncols);
    return db->table_info[tid].col_name[colid];
}

char* db_get_col_type(struct db13* db, db_table_id tid, db_colid_t colid){
    assert(tid < db->ntableslots);
    assert(colid < db->table_info[tid].ncols);
    return db->table_info[tid].col_type[colid];
}

char* db_get_col_alias(struct db13* db, db_table_id tid, db_colid_t colid){
    assert(tid < db->ntableslots);
    assert(colid < db->table_info[tid].ncols);
    return db->table_info[tid].col_alias[colid];
}

db_colid_t db_col_count(struct db13* db, db_table_id tid){
    assert(tid < db->ntableslots);
    return db->table_info[tid].ncols;
}

db_colflag_t db_get_colflag(struct db13* db, db_table_id tid, db_colid_t colid){
    assert(tid < db->ntableslots);
    assert(colid < db->table_info[tid].ncols);
    return db->table_info[tid].col_flags[colid];
}

void db_set_colflag(struct db13* db, db_table_id tid, db_colid_t colid, db_colflag_t flags){
    assert(tid < db->ntableslots);
    assert(colid < db->table_info[tid].ncols);
    db->table_info[tid].col_flags[colid] |= flags;
}

void db_unset_colflag(struct db13* db, db_table_id tid, db_colid_t colid, db_colflag_t flags){
    assert(tid < db->ntableslots);
    assert(colid < db->table_info[tid].ncols);
    db->table_info[tid].col_flags[colid] &= ~flags;
}

db_colid_t db_count_colflag(struct db13* db, db_table_id tid, db_colflag_t flags){
    db_colid_t cnt = 0, i;

    assert(tid < db->ntableslots);

    for(i = 0; i < db->table_info[tid].ncols; i++){
        if(db->table_info[tid].col_flags[i] & flags) cnt++;
    }
    return cnt;
}

static inline char* _db_type_str(struct db13 *db, enum db_type_id type){

    switch(type){

    case DB_TY_BOOL:
        return DB_T_BOOL;
        break;
    case DB_TY_INT:
        return DB_T_INT;
        break;
    case DB_TY_BIGINT:
        return DB_T_BIGINT;
        break;
    case DB_TY_REAL:
        return DB_T_REAL;
        break;
    case DB_TY_TEXT:
        return DB_T_TEXT;
        break;
    case DB_TY_DATE:
        return DB_T_DATE;
        break;
	case DB_TY_D13STIME:
		return DB_T_D13STIME;
		break;
    case DB_TY_RAW:
        return DB_T_RAW;
        break;
    default:
        break;

    }

    return DB_T_INVAL;
}

enum db_type_id db_coltype(struct db13 *db, db_table_id tid, db_colid_t colid){

    assert(tid < db->ntableslots);
    assert(colid < db->table_info[tid].ncols);

    if(!strcmp(db->table_info[tid].col_type[colid], DB_T_BOOL)) return DB_TY_BOOL;
    else if(!strcmp(db->table_info[tid].col_type[colid], DB_T_INT)) return DB_TY_INT;
    else if(!strcmp(db->table_info[tid].col_type[colid], DB_T_BIGINT)) return DB_TY_BIGINT;
    else if(!strcmp(db->table_info[tid].col_type[colid], DB_T_REAL)) return DB_TY_REAL;
    else if(!strcmp(db->table_info[tid].col_type[colid], DB_T_TEXT)) return DB_TY_TEXT;
    else if(!strcmp(db->table_info[tid].col_type[colid], DB_T_DATE)) return DB_TY_DATE;
    else if(!strcmp(db->table_info[tid].col_type[colid], DB_T_D13STIME)) return DB_TY_D13STIME;
    else if(!strcmp(db->table_info[tid].col_type[colid], DB_T_RAW)) return DB_TY_RAW;
    else return DB_TY_INVAL;
}

char* db_coltype_name(struct db13 *db, db_table_id tid, db_colid_t colid){
    assert(tid < db->ntableslots);
    assert(colid < db->table_info[tid].ncols);
    return db->table_info[tid].col_type[colid];
}

error13_t db_step(struct db_stmt* st){

    switch(DB_ST(st)->driver){
    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:
        switch(sqlite3_step(LITE_ST(st))){
        case SQLITE_OK:
            return e13_error(E13_EMPTY);
            break;
        case SQLITE_DONE:
            return E13_OK;
            break;

        case SQLITE_ROW:
            return E13_CONTINUE;
            break;

        default:
            return e13_ierror(&st->db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(DB_ST(st))));
            break;
        }

        break;

    default:
        return e13_ierror(&st->db->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;
    }

}

error13_t db_finalize(struct db_stmt* st){

    if(st->magic != DB_STMT_MAGIC){
        return e13_error(E13_MISUSE);
    }

    switch(DB_ST(st)->driver){
    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:
        switch(sqlite3_finalize(LITE_ST(st))){
        case SQLITE_DONE:
        case SQLITE_OK:
        	st->magic = MAGIC13_INV;
            return E13_OK;
            break;

//        case SQLITE_ROW:
//            return E13_CONTINUE;
//            break;

        default:
            return e13_ierror(&st->db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(st->db)));
            break;
        }

        break;

    default:
        return e13_ierror(&st->db->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;
    }

    return E13_OK;

}

error13_t db_reset(struct db_stmt* st){

    if(st->magic != DB_STMT_MAGIC){
        return e13_error(E13_MISUSE);
    }

    switch(DB_ST(st)->driver){
    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:
        switch(sqlite3_reset(LITE_ST(st))){
        case SQLITE_DONE:
        case SQLITE_OK:
            return E13_OK;
            break;

        case SQLITE_ROW:
            return E13_CONTINUE;
            break;

        default:
            return e13_ierror(&st->db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(st->db)));
            break;
        }

        break;

    default:
        return e13_ierror(&st->db->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;
    }
}

error13_t db_begin_trans(struct db13* db){
    switch(db->driver){
    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:
        switch(sqlite3_exec(LITE(db), "BEGIN TRANSACTION;", NULL, NULL, NULL)){
        case SQLITE_DONE:
        case SQLITE_OK:
            return E13_OK;
            break;

        case SQLITE_ROW:
            return E13_CONTINUE;
            break;

        default:
            return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            break;
        }

        break;

    default:
        return e13_ierror(&db->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;
    }
}

error13_t db_commit_trans(struct db13* db){
    switch(db->driver){
    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:
        switch(sqlite3_exec(LITE(db), "COMMIT TRANSACTION;", NULL, NULL, NULL)){
        case SQLITE_DONE:
        case SQLITE_OK:
            return E13_OK;
            break;

        case SQLITE_ROW:
            return E13_CONTINUE;
            break;

        default:
            return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            break;
        }

        break;

    default:
        return e13_ierror(&db->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;
    }
}

error13_t db_rollback_trans(struct db13* db){
    switch(db->driver){
    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:
        switch(sqlite3_exec(LITE(db), "ROLLBACK TRANSACTION;", NULL, NULL, NULL)){
        case SQLITE_DONE:
        case SQLITE_OK:
            return E13_OK;
            break;

        case SQLITE_ROW:
            return E13_CONTINUE;
            break;

        default:
            return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            break;
        }

        break;

    default:
        return e13_ierror(&db->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;
    }
}

error13_t db_column_text(struct db_stmt* st, db_colid_t col, size_t* len, unsigned char** text){

	assert(col != DB_COLID_INVAL); assert(len);

    switch(DB_ST(st)->driver){
    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:

        if(len){
            *len = strlen((char*)sqlite3_column_text(LITE_ST(st), col));
            if(!(*len)) return e13_error(E13_EMPTY);
        }

        *text = (uchar*)m13_malloc(len?(*len)+1:sqlite3_column_bytes(LITE_ST(st), col)+1);
        if(!(*text)) return e13_error(E13_NOMEM);
        strcpy((char*)*text, (char*)sqlite3_column_text(LITE_ST(st), col));

        return E13_OK;

        break;

    default:
        return e13_ierror(&DB_ST(st)->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;
    }

}

error13_t db_column_date(struct db_stmt* st, db_colid_t col, int date[3]){

    int val;

    assert(col != DB_COLID_INVAL); assert(val);

    switch(DB_ST(st)->driver){
    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:

        val = sqlite3_column_int(LITE_ST(st), col);
        return d13_jdayno2jdate(val, date);

        break;

    default:
        return e13_ierror(&DB_ST(st)->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;
    }

}

error13_t db_column_int(struct db_stmt* st, db_colid_t col, int* val){

	assert(col != DB_COLID_INVAL); assert(val);

    switch(DB_ST(st)->driver){
    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:

        *val = sqlite3_column_int(LITE_ST(st), col);

        return E13_OK;

        break;

    default:
        return e13_ierror(&DB_ST(st)->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;
    }

}

error13_t db_column_int64(struct db_stmt* st, db_colid_t col, int64_t* val){

	assert(col != DB_COLID_INVAL); assert(val);

    switch(DB_ST(st)->driver){
    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:

        *val = sqlite3_column_int64(LITE_ST(st), col);

        return E13_OK;

        break;

    default:
        return e13_ierror(&DB_ST(st)->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;
    }
}

error13_t db_column_double(struct db_stmt* st, db_colid_t col, double* val){

	assert(col != DB_COLID_INVAL); assert(val);

    switch(DB_ST(st)->driver){
    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:

        *val = sqlite3_column_double(LITE_ST(st), col);

        return E13_OK;

        break;

    default:
        return e13_ierror(&DB_ST(st)->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;
    }
}

error13_t db_column_size(struct db_stmt* st, db_colid_t col, size_t *size){

	assert(col != DB_COLID_INVAL); assert(size);

    switch(DB_ST(st)->driver){
    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:

        *size = (size_t)sqlite3_column_bytes(LITE_ST(st), col);

        return E13_OK;

        break;

    default:
        return e13_ierror(&DB_ST(st)->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;
    }
}

error13_t db_column_raw(struct db_stmt* st, db_colid_t col, size_t* datalen, void** data){

	assert(col != DB_COLID_INVAL); assert(datalen);

    switch(DB_ST(st)->driver){
    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:

        *datalen = (size_t)sqlite3_column_bytes(LITE_ST(st), col);
        if(!(*datalen)) return e13_error(E13_EMPTY);

        *data = (char*)m13_malloc(*datalen);
        if(!(*data)) return e13_error(E13_NOMEM);
        memcpy(*data, sqlite3_column_blob(LITE_ST(st), col), *datalen);

        return E13_OK;

        break;

    default:
        return e13_ierror(&DB_ST(st)->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;
    }
}

error13_t db_trunc_table(struct db13* db, db_table_id tid){

    char sql[MAXSQL];

    switch(_db_get_drv_cls(db->driver)){
        case DB_DRV_CLS_SQL:

        snprintf(sql, MAXSQL, "DELETE FROM %s;", db_get_table_name(db, tid));

        break;

        default:
        break;
    }

    switch(db->driver){
    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:

        switch(sqlite3_exec(LITE(db), sql, NULL, NULL, NULL)){
        case SQLITE_DONE:
        case SQLITE_OK:
            return E13_OK;
            break;

        default:
            return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            break;
        }

        break;

    default:
        return e13_ierror(&db->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;
    }

}

error13_t db_rm_table(struct db13* db, db_table_id tid){

    char sql[MAXSQL];

    switch(_db_get_drv_cls(db->driver)){
        case DB_DRV_CLS_SQL:

        snprintf(sql, MAXSQL, "DROP TABLE %s;", db_get_table_name(db, tid));

        break;

        default:
        break;
    }

    switch(db->driver){
    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:

        switch(sqlite3_exec(LITE(db), sql, NULL, NULL, NULL)){
        case SQLITE_DONE:
        case SQLITE_OK:
            return E13_OK;
            break;

        default:
            return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            break;
        }

        break;

    default:
        return e13_ierror(&db->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;
    }

}


error13_t db_create_table(struct db13* db, db_table_id tid){

    char sql[MAXSQL];
    size_t len = 0;
    db_colid_t i;
    char* emsg;
    db_colid_t vir = db_count_colflag(db, tid, DB_COLF_VIRTUAL);
    if(vir == db->table_info[tid].ncols) return E13_OK;

    _deb_create_table("create table, tid = %u", tid);

    switch(_db_get_drv_cls(db->driver)){
    case DB_DRV_CLS_SQL:

#define RLEN (MAXSQL - len)

    _deb_create_table("name = %s, ncols = %u", db->table_info[tid].name, db->table_info[tid].ncols);
    snprintf(sql + len, RLEN, "CREATE TABLE IF NOT EXISTS %s(", db->table_info[tid].name);
    len = strlen(sql);

    for(i = 0; i < db->table_info[tid].ncols; i++){
        _deb_create_table("col = %i, colname = %s, coltype = %s", i, db->table_info[tid].col_name[i], db->table_info[tid].col_type[i]);
        if(db_get_colflag(db, tid, i) & DB_COLF_VIRTUAL) continue;
        snprintf(sql + len, RLEN, "%s %s%s",
                 db->table_info[tid].col_name[i],
                 db->table_info[tid].col_type[i],
                 i == db->table_info[tid].ncols - 1 - vir?");":", ");
        len = strlen(sql);
    }

#undef RLEN
        break;
    default:
        break;
    }

    _deb_create_table("sql: %s", sql);

    switch(db->driver){

    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:

    _deb_create_table("exec...");

    switch(sqlite3_exec(LITE(db), sql, NULL, NULL, &emsg)){
        case SQLITE_DONE:
        case SQLITE_OK:
            _deb_create_table("exec: done, ok");
            return E13_OK;
            break;

        default:
            _deb_create_table("exec failed");
            return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            break;
        }

    default:
        _deb_create_table("bad driver");
        return e13_ierror(&db->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;

    }

}

error13_t db_select_all(struct db13* db, db_table_id tid, struct db_stmt* st){
    char sql[MAXSQL];
    sqlite3_stmt* stmt = NULL;

    switch(_db_get_drv_cls(db->driver)){
    case DB_DRV_CLS_SQL:

    snprintf(sql, MAXSQL, "SELECT * FROM %s;", db_get_table_name(db, tid));

        break;
    default:
        break;
    }

    switch(db->driver){

    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:

    switch(sqlite3_prepare_v2(LITE(db), sql, -1, &stmt, NULL)){
        case SQLITE_DONE:
        case SQLITE_OK:

            //set handles
            st->h = stmt;
            st->db = db;
            st->magic = DB_STMT_MAGIC;

            return E13_OK;
            break;

        default:
            return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            break;
        }

    default:
        return e13_ierror(&db->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;

    }

    return true_;
}

error13_t db_update_col_byobjid(struct db13* db, db_table_id tid, objid13_t objid, db_colid_t col, uchar* val, size_t size, struct db_stmt* st){

    char sql[MAXSQL];
    int date[3];
    sqlite3_stmt* stmt = NULL;

    if(st->magic == DB_STMT_MAGIC){
        db_reset(st);
        goto bind_data;
    }

    switch(_db_get_drv_cls(db->driver)){
    case DB_DRV_CLS_SQL:

#define RLEN (MAXSQL - len)

    snprintf(sql, MAXSQL, "UPDATE %s SET %s = ?1 WHERE objid = ?2", db->table_info[tid].name, db_get_col_name(db, tid, col));

#undef RLEN

        break;
    default:
        break;
    }

    switch(db->driver){

    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:

    switch(sqlite3_prepare_v2(LITE(db), sql, -1, &stmt, NULL)){
        case SQLITE_DONE:
        case SQLITE_OK:

            //set handles
            st->h = stmt;
            st->db = db;
            st->magic = DB_STMT_MAGIC;

            break;

        default:
            return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            break;
        }

    break;

    default:
        return e13_ierror(&db->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;

    }

bind_data:

    switch(db->driver){

    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:

    switch(db_coltype(db, tid, col)){

    case DB_TY_BOOL:
    case DB_TY_INT:
        if(sqlite3_bind_int(LITE_ST(st), 1, (int32_t)(*val)) != SQLITE_OK){
            return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
        }
        break;

    case DB_TY_BIGINT:
        if(sqlite3_bind_int64(LITE_ST(st), 1, (int64_t)(*val)) != SQLITE_OK){
            return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
        }
        break;

    case DB_TY_REAL:
        if(sqlite3_bind_double(LITE_ST(st), 1, (double)(*val)) != SQLITE_OK){
            return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
        }
        break;

    case DB_TY_DATE:
        d13_resolve_date((char*)val, date);
        if(sqlite3_bind_int(LITE_ST(st), 1, (int)d13_jdayno(date[0], date[1], date[2])) != SQLITE_OK){
            return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
        }
        break;

    case DB_TY_D13STIME:

        if(sqlite3_bind_int64(LITE_ST(st), 1, (int64_t)(*val)) != SQLITE_OK){
            return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
        }
        break;

    case DB_TY_TEXT:
        if(sqlite3_bind_text(LITE_ST(st), 1, (char*)val, size?size:-1, NULL) != SQLITE_OK){
            return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
        }
        break;

    default:
        if(sqlite3_bind_blob(LITE_ST(st), 1, val, size, NULL) != SQLITE_OK){
            return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
        }
        break;
    }

    if(sqlite3_bind_int64(LITE_ST(st), 2, objid) != SQLITE_OK){
        return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
    }

    switch(sqlite3_step(LITE_ST(st))){

    case SQLITE_DONE:
    case SQLITE_OK:
        //everything's true but no real changes!
        if(!sqlite3_changes(LITE(db))) return E13_CONTINUE;
        break;
    default:
        return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
        break;

    }

    //perr("UPDATE TRUE");

    return E13_OK;

    break;

    default:
        return e13_ierror(&db->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;

    } //switch(db->driver);

    return E13_OK;

}

error13_t db_update(struct db13* db, db_table_id tid,
					struct db_logic_s iflogic,
					db_colid_t ncol, db_colid_t* col,
					uchar** val, size_t* size,
					struct db_stmt* st){

    char sql[MAXSQL];
    int date[3];
    sqlite3_stmt* stmt = NULL;
    db_colid_t i;
    size_t len;

    if(st->magic == DB_STMT_MAGIC){
        db_reset(st);
        goto bind_data;
    }

    switch(_db_get_drv_cls(db->driver)){
    case DB_DRV_CLS_SQL:

#define RLEN (MAXSQL - len)
    //snprintf(sql, MAXSQL, "UPDATE %s SET %s = ?1 WHERE %s = ?2", db->table_info[tid].name, db_get_col_name(db, tid, col), db_get_col_name(db, tid, ifcol));

    snprintf(sql, MAXSQL, "UPDATE %s SET ", db->table_info[tid].name);

    for(i = 0; i < ncol; i++){

        len = strlen(sql);
        snprintf(sql + len, RLEN, "%s = ?%i%s", db_get_col_name(db, tid, col[i]), col[i], i == ncol - 1?" ":", ");

    }

    len = strlen(sql);

    snprintf(sql + len, RLEN, "WHERE %s %s ?%i",
			iflogic.colname,
             logic_sign[iflogic.logic].sign,
             ACC_UP_IF_COL);

#undef RLEN

        break;
    default:
        break;
    }

    _deb_update("sql: %s", sql);

    switch(db->driver){

    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:

    switch(sqlite3_prepare_v2(LITE(db), sql, -1, &stmt, NULL)){
        case SQLITE_DONE:
        case SQLITE_OK:

            //set handles
            st->h = stmt;
            st->db = db;
            st->magic = DB_STMT_MAGIC;

            break;

        default:
            return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            break;
        }

    break;

    default:
        return e13_ierror(&db->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;

    }

	if(!(iflogic.flags & DB_LOGICF_USE_COLID)){
			iflogic.colid = db_get_colid_byname(db, tid, iflogic.colname);
	}

bind_data:

    switch(db->driver){

    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:

    for(i = 0; i < ncol + 1; i++){//bind ifval here too!

        _deb_update("table.ncols = %u, col[%i] = %u", db->table_info[tid].ncols, i, col[i]);

        switch(i==ncol?db_coltype(db, tid, iflogic.colid):db_coltype(db, tid, col[i])){

        case DB_TY_BOOL:
        case DB_TY_INT:
        	_deb_update("bind int %i", (i==ncol?iflogic.ival:*((int32_t*)val[i])));
            if(sqlite3_bind_int(LITE_ST(st), i==ncol?ACC_UP_IF_COL:col[i], (i==ncol?iflogic.ival:(int32_t*)val[i])) != SQLITE_OK){
                return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            }
            break;

        case DB_TY_BIGINT:        	
        	_deb_update("bind int64 %llu", *((int64_t*)val[i]));
            if(sqlite3_bind_int64(LITE_ST(st), i==ncol?ACC_UP_IF_COL:col[i], (i==ncol?iflogic.ival:(int64_t*)val[i])) != SQLITE_OK){
                return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            }
            break;

        case DB_TY_REAL:
        	_deb_update("bind double %d", *((double*)val[i]));
            if(sqlite3_bind_double(LITE_ST(st), i==ncol?ACC_UP_IF_COL:col[i], (i==ncol?iflogic.ival:*((double*)val[i]))) != SQLITE_OK){
                return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            }
            break;

        case DB_TY_DATE:
            d13_resolve_date((char*)(i==ncol?iflogic.sval:val[i]), date);
            if(sqlite3_bind_int(LITE_ST(st), i==ncol?ACC_UP_IF_COL:col[i], (int)d13_jdayno(date[0], date[1], date[2])) != SQLITE_OK){
                return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            }
            break;

        case DB_TY_D13STIME:
        	_deb_update("updating time %llu...", (i==ncol?iflogic.ival:*val[i]));
            if(sqlite3_bind_int64(LITE_ST(st), i==ncol?ACC_UP_IF_COL:col[i], (int64_t)(i==ncol?iflogic.ival:*val[i])) != SQLITE_OK){
                return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            }
            break;

        case DB_TY_TEXT:
        	_deb_update("bind text: %s", (char*)(i==ncol?iflogic.sval:val[i]));
            if(sqlite3_bind_text(LITE_ST(st), i==ncol?ACC_UP_IF_COL:col[i], (char*)(i==ncol?iflogic.sval:val[i]), size?(size[i]?size[i]:-1):-1, NULL) != SQLITE_OK){
                return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            }
            break;

        default:
            if(sqlite3_bind_blob(LITE_ST(st), i==ncol?ACC_UP_IF_COL:col[i], (i==ncol?iflogic.sval:val[i]), size?size[i]:-1, NULL) != SQLITE_OK){
                return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            }
            break;
        }

    }//for

//    if(sqlite3_bind_int64(LITE_ST(st), i+1, objid) != SQLITE_OK){
//        return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
//    }

    switch(sqlite3_step(LITE_ST(st))){

    case SQLITE_DONE:
    case SQLITE_OK:
        //everything's true but no real changes!
        if(!sqlite3_changes(LITE(db))) return E13_CONTINUE;
        break;
    default:
        return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
        break;

    }

    //perr("UPDATE TRUE");

    return E13_OK;

    break;

    default:
        return e13_ierror(&db->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;

    } //switch(db->driver);

    return E13_OK;

}

error13_t db_insert(struct db13* db, db_table_id tid, uchar** val, size_t* size, struct db_stmt* st){

    int i;
    char sql[MAXSQL];
    int date[3];
    size_t len = 0;
    sqlite3_stmt* stmt = NULL;
    db_colid_t ncols = db_col_count(db, tid);
    db_colid_t vir = db_count_colflag(db, tid, DB_COLF_VIRTUAL);

    assert(size);
    assert(st);
    assert(val);

    if(vir == ncols){
        _deb_insert("table virtual %u = %u", vir, db_col_count(db, tid));
        return E13_OK;
    }

    if(!val || !size){
        _deb_insert("!val || !size");
        return E13_CONTINUE;
    }

    if(st->magic == DB_STMT_MAGIC){
        _deb_insert("predone stmt, bind");
        db_reset(st);
        goto bind_data;
    }

    switch(_db_get_drv_cls(db->driver)){
    case DB_DRV_CLS_SQL:

#define RLEN (MAXSQL - len)
    //INSERT INTO table(list...) VALUES(list...);

    snprintf(sql, MAXSQL, "INSERT INTO %s(", db_get_table_name(db, tid));
    len = strlen(sql);

    for(i = 0; i < ncols; i++){

        if(!val[i] || (db_get_colflag(db, tid, i) & DB_COLF_VIRTUAL)) continue;

        if(i == ncols - 1 - vir){
            snprintf(sql+len, RLEN, "%s", db->table_info[tid].col_name[i]);
        } else {
            snprintf(sql+len, RLEN, "%s, ", db->table_info[tid].col_name[i]);
        }
        len = strlen(sql);
    }

    snprintf(sql+len, RLEN, ") VALUES(");
    len = strlen(sql);

    for(i = 0; i < ncols; i++){

        if(!val[i] || (db_get_colflag(db, tid, i) & DB_COLF_VIRTUAL)) continue;

        if((i == ncols - 1 - vir)) {
            snprintf(sql+len, RLEN, "?%i", i+1);
        } else {
            snprintf(sql+len, RLEN, "?%i, ", i+1);
        }
        len = strlen(sql);
    }

    snprintf(sql+len, RLEN, ");");

#undef RLEN

    break;

    default:
        break;
    }//switch(db_drv[db->driver].cls);

    _deb_insert("sql: %s", sql);

    switch(db->driver){

    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:

        switch(sqlite3_prepare_v2(LITE(db), sql, -1, &stmt, NULL)){
            case SQLITE_DONE:
            case SQLITE_OK:

                //set handles
                st->h = stmt;
                st->db = db;
                st->magic = DB_STMT_MAGIC;

                break;

            default:
                return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
                break;
        }

        break;

    default:
        return e13_ierror(&db->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;

    }//switch(db->driver);

bind_data:

    _deb_insert("binding...");

    switch(db->driver){

    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:

    for(i = 0; i < db->table_info[tid].ncols; i++){

        _deb_insert("tid: %u, col: %u (%s), val: %s", tid, i, db->table_info[tid].col_name[i], val[i]?"OK":"NULL");

        if(!val[i] || (db_get_colflag(db, tid, i) & DB_COLF_VIRTUAL)) {_deb_insert("!val || VIRTU"); continue;}

        switch(db_coltype(db, tid, i)){

        case DB_TY_BOOL:
        case DB_TY_INT:
            _deb_insert("type: %s: %i", "BOOL/INT", (int32_t)(*val[i]));
            if(sqlite3_bind_int(LITE_ST(st), i+1, (int32_t)(*val[i])) != SQLITE_OK){
                return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            }
            break;

        case DB_TY_BIGINT:
            _deb_insert("type: %s: %i", "BIGINT", (int64_t)(*val[i]));
            if(sqlite3_bind_int64(LITE_ST(st), i+1, (int64_t)(*val[i])) != SQLITE_OK){
                return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            }
            break;

        case DB_TY_REAL:
            _deb_insert("type: %s: %f", "REAL", (double)(*val[i]));
            if(sqlite3_bind_double(LITE_ST(st), i+1, (double)(*val[i])) != SQLITE_OK){
                return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            }
            break;

        case DB_TY_DATE:
            d13_resolve_date((char*)val[i], date);
            _deb_insert("type: %s: %i/%i/%i", "DATE", date[0], date[1], date[2]);
            if(sqlite3_bind_int(LITE_ST(st), i+1, (int)d13_gdayno(date[0], date[1], date[2])) != SQLITE_OK){
                return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            }
            break;

        case DB_TY_D13STIME:
            _deb_insert("type: %s: %i", "TIME", (int64_t)(*val[i]));
            if(sqlite3_bind_int64(LITE_ST(st), i+1, (int64_t)(*val[i])) != SQLITE_OK){
                return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            }
            break;

        case DB_TY_TEXT:
            _deb_insert("type: %s: %s", "TEXT", (char*)val[i]);
            if(sqlite3_bind_text(LITE_ST(st), i+1, (char*)val[i], size[i]?size[i]:-1, NULL) != SQLITE_OK){
                return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            }
            break;

        default:
            _deb_insert("type: %s: %s", "BLOB", val[i]);
            if(sqlite3_bind_blob(LITE_ST(st), i+1, val[i], size[i], NULL) != SQLITE_OK){
                return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            }
            break;
        }

    }

    switch(sqlite3_step(LITE_ST(st))){

    case SQLITE_DONE:
    case SQLITE_OK:
        _deb_insert("step OK/DONE");
        //everything's true but no real changes!
        if(!sqlite3_changes(LITE(db))) {_deb_insert("no real changes"); return E13_CONTINUE;}
        _deb_insert("changes OK");
        break;
    default:
        _deb_insert("error %s", sqlite3_errmsg(LITE(db)));
        return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
        break;

    }

    //perr("UPDATE TRUE");

    return E13_OK;

    break;

    default:
        return e13_ierror(&db->e, E13_IMPLEMENT, "s",
						msg13(M13_DRIVERNOTSUPPORTED));
        break;

    } //switch(db->driver);

    return E13_OK;

}

error13_t _db_assert_logic(int nlogic, struct db_logic_s* logic){
    return E13_OK;
}

error13_t db_collect(	struct db13* db, db_table_id tid,
						char** cols,
						int nlogic, struct db_logic_s* logic,
						char* sortcol, enum db_sort stype, int nlimit,
						struct db_stmt* st){

    char sql[MAXSQL];
    int i;
    int date[3];
    size_t len = 0;
    sqlite3_stmt* stmt = NULL;

    switch(_db_get_drv_cls(db->driver)){
    case DB_DRV_CLS_SQL:

#define RLEN (MAXSQL - len)

	if(_db_assert_logic(nlogic, logic) != E13_OK) return e13_error(E13_MISUSE);

    snprintf(sql + len, RLEN, "SELECT ");
    _deb_collect(sql);
    len = strlen(sql);

    if(!cols){
        snprintf(sql + len, RLEN, "* ");
        _deb_collect(sql);
        len = strlen(sql);
    } else {
        while(*cols){
            snprintf(sql + len, RLEN, "%s%s", *cols, (cols+1)?",":" ");
            _deb_collect(sql);
            len = strlen(sql);
            cols++;
        }
    }

    snprintf(sql + len, RLEN, "FROM %s", db->table_info[tid].name);
    _deb_collect(sql);
    len = strlen(sql);

    if(nlogic){

        snprintf(sql + len, RLEN, " WHERE ");
        _deb_collect(sql);
        len = strlen(sql);

        for(i = 1; i < nlogic + 1; i++){

//            if(!(logic[i-1].flags & DB_LOGICF_COL_CMP)){

			if(logic[i-1].logic >= DB_LOGIC_AND && logic[i-1].logic < DB_LOGIC_INVAL){

				if(i == nlogic && logic[i-1].logic != DB_LOGIC_CLOSEP)
					return e13_error(E13_SYNTAX);

				snprintf(sql + len, RLEN, " %s ", logic_sign[logic[i-1].logic].sign);
				len = strlen(sql);
				continue;
			} else {

				snprintf(sql + len, RLEN, " %s %s ?%i",
						 logic[i-1].flags & DB_LOGICF_USE_COLID?db_get_col_name(db, tid, logic[i-1].colid):logic[i-1].colname,
						 logic_sign[logic[i-1].logic].sign,
						 i
						);
						_deb_collect(sql);
			}

            len = strlen(sql);

        }

    }

    if(sortcol){
        snprintf(sql + len, RLEN, " ORDER BY %s %s", sortcol, stype == DB_SO_DEC?"DESC":"");
        _deb_collect(sql);
        len = strlen(sql);
    }

    if(nlimit > 0){
        snprintf(sql + len, RLEN, " LIMIT %i", nlimit);
        _deb_collect(sql);
        //len = strlen(sql);
    }

#undef RLEN
    strcat(sql, ";");

    break;

    default:
        break;
    }

    _deb_collect_final("sql: %s", sql);

    switch(db->driver){

    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:

        switch(sqlite3_prepare_v2(LITE(db), sql, -1, &stmt, NULL)){
            case SQLITE_DONE:
            case SQLITE_OK:

                //set handles
                st->h = stmt;
                st->db = db;
                st->magic = DB_STMT_MAGIC;

                break;

            default:
                return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
                break;
        }

    if(nlogic){

        _deb_collect("binding %i logics", nlogic);

        for(i = 1; i < nlogic + 1; i++){

            _deb_collect("logic %i", i);

//            if(!(logic[i-1].flags & DB_LOGICF_COL_CMP)){

			if(!(logic[i-1].flags & DB_LOGICF_USE_COLID))
				logic[i-1].colid = db_get_colid_byname(db, tid, logic[i-1].colname);

			switch(db_coltype(db, tid, logic[i-1].colid)){

			case DB_TY_BOOL:
			case DB_TY_INT:
				_deb_collect("INT/BOOL %i", logic[i-1].ival);
				if(sqlite3_bind_int(stmt, i, logic[i-1].ival) != SQLITE_OK){
					return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
				}
			break;

			case DB_TY_BIGINT:
				_deb_collect("BIGINT %i", logic[i-1].ival);
				if(sqlite3_bind_int64(stmt, i, logic[i-1].ival) != SQLITE_OK){
					return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
				}
			break;

			case DB_TY_REAL:
				_deb_collect("REAL %f", (double)logic[i-1].ival);
				if(sqlite3_bind_double(stmt, i, (double)logic[i-1].ival) != SQLITE_OK){
					return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
				}
			break;

			case DB_TY_DATE:
				_deb_collect("DATE %s", logic[i-1].sval);
				if(d13_resolve_date(logic[i-1].sval, date) != E13_OK){
					continue;
				}

				if(sqlite3_bind_int(stmt, i, (int)d13_jdayno(date[0], date[1], date[2])) != SQLITE_OK){
					return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
				}
				break;

			case DB_TY_D13STIME:
				_deb_collect("D13STIME %i", logic[i-1].ival);
				if(sqlite3_bind_int64(stmt, i, logic[i-1].ival) != SQLITE_OK){
					return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
				}
			break;

			case DB_TY_TEXT:
				_deb_collect("TEXT %s", logic[i-1].sval);
				if(sqlite3_bind_text(stmt, i, logic[i-1].sval, -1, NULL) != SQLITE_OK){
					return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
				}
			break;

			default:
				//set ival as size for blob
				_deb_collect("BLOB %s size %i", logic[i-1].sval, logic[i-1].ival);
				if(sqlite3_bind_blob(stmt, i, logic[i-1].sval, logic[i-1].ival, NULL) != SQLITE_OK){
					return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
				}
				break;

			}

       //     }//if(!(logic[i-1].col_flags & DB_LOGICF_COL_CMP))

        }//for

    }//if(nlogic)

    return E13_OK;

    break;

    default:
         return e13_ierror(&db->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;

    }

    return E13_OK;

}

/*
#include <stdio.h>
#include <sqlite3.h>

static int callback(void *count, int argc, char **argv, char **azColName) {
    int *c = count;
    *c = atoi(argv[0]);
    return 0;
}

int main(int argc, char **argv) {
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    int count = 0;

    rc = sqlite3_open("test.db", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return(1);
    }
    rc = sqlite3_exec(db, "select count(*) from mytable", callback, &count, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        printf("count: %d\n", count);
    }
    sqlite3_close(db);
    return 0;
}
*/

error13_t db_count(	struct db13* db, db_table_id tid,
					int nlogic, struct db_logic_s* logic,
					db_rowid_t* nrows){

    char sql[MAXSQL];
    int i;
    int date[3];
    size_t len = 0;
    sqlite3_stmt* stmt = NULL;

    switch(_db_get_drv_cls(db->driver)){
    case DB_DRV_CLS_SQL:

#define RLEN (MAXSQL - len)

    snprintf(sql + len, RLEN, "COUNT * FROM %s", db->table_info[tid].name);
    len = strlen(sql);

    if(nlogic){

        snprintf(sql + len, RLEN, " WHERE ");
        len = strlen(sql);

        for(i = 1; i < nlogic + 1; i++){

			if(logic[i-1].logic >= DB_LOGIC_AND && logic[i-1].logic < DB_LOGIC_INVAL){

				if(i == nlogic && logic[i-1].logic != DB_LOGIC_CLOSEP)
					return e13_error(E13_SYNTAX);

				snprintf(sql + len, RLEN, " %s ", logic_sign[logic[i-1].logic].sign);
				len = strlen(sql);
				continue;

			} else {

				snprintf(sql + len, RLEN, " %s %s ?%i",
						 logic[i-1].flags & DB_LOGICF_USE_COLID?db_get_col_name(db, tid, logic[i-1].colid):logic[i-1].colname,
						 logic_sign[logic[i-1].logic].sign,
						 i
						);

				len = strlen(sql);

			}

        }

    }

#undef RLEN
    strcat(sql, ";");

    break;

    default:
        break;
    }

    _deb_collect("sql: %s", sql);

    switch(db->driver){

    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:

        switch(sqlite3_prepare_v2(LITE(db), sql, -1, &stmt, NULL)){
            case SQLITE_DONE:
            case SQLITE_OK:

                //set handles

				switch(sqlite3_step(stmt)){
				case SQLITE_ROW:
					i = sqlite3_column_int(stmt, 0);
					if(i > -1) *nrows = (db_rowid_t)i;
					else *nrows = 0;
					sqlite3_finalize(stmt);
					break;
				case SQLITE_DONE:
				case SQLITE_OK:
					//everything's true but no real changes!
					*nrows = DB_ROWID_ZERO;
                    sqlite3_finalize(stmt);
					break;
				default:
					sqlite3_finalize(stmt);
					return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
					break;

				}

                break;

            default:
                return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
                break;
        }
			default:
				return e13_ierror(&db->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
				break;
    }

    return E13_OK;

}

error13_t db_delete(	struct db13* db, db_table_id tid,
						int nlogic, struct db_logic_s* logic,
						struct db_stmt* st){

    char sql[MAXSQL];
    int i;
    int date[3];
    size_t len = 0;
    sqlite3_stmt* stmt = NULL;

    switch(_db_get_drv_cls(db->driver)){
    case DB_DRV_CLS_SQL:

#define RLEN (MAXSQL - len)

    snprintf(sql + len, RLEN, "DELETE FROM %s", db->table_info[tid].name);
    len = strlen(sql);

    if(nlogic){

        snprintf(sql + len, RLEN, " WHERE ");
        len = strlen(sql);

        for(i = 1; i < nlogic + 1; i++){

			if(logic[i-1].logic >= DB_LOGIC_AND && logic[i-1].logic < DB_LOGIC_INVAL){

				if(i == nlogic && logic[i-1].logic != DB_LOGIC_CLOSEP)
					return e13_error(E13_SYNTAX);

				snprintf(sql + len, RLEN, " %s ", logic_sign[logic[i-1].logic].sign);
				len = strlen(sql);
				continue;

			} else {

				snprintf(sql + len, RLEN, " %s %s ?%i",
						 logic[i-1].flags & DB_LOGICF_USE_COLID?db_get_col_name(db, tid, logic[i-1].colid):logic[i-1].colname,
						 logic_sign[logic[i-1].logic].sign,
						 i
						);

				len = strlen(sql);

			}

        }

    }

#undef RLEN
    strcat(sql, ";");

    break;

    default:
        break;
    }

    _deb_delete("sql: %s", sql);

    switch(db->driver){

    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:

        switch(sqlite3_prepare_v2(LITE(db), sql, -1, &stmt, NULL)){
            case SQLITE_DONE:
            case SQLITE_OK:

                //set handles
                st->h = stmt;
                st->db = db;
                st->magic = DB_STMT_MAGIC;
		                
			    if(nlogic){
			
			        _deb_delete("binding %i logics", nlogic);
			
			        for(i = 1; i < nlogic + 1; i++){
			
			            _deb_delete("logic %i", i);
			
			//            if(!(logic[i-1].flags & DB_LOGICF_COL_CMP)){
			
						if(!(logic[i-1].flags & DB_LOGICF_USE_COLID))
							logic[i-1].colid = db_get_colid_byname(db, tid, logic[i-1].colname);
			
						switch(db_coltype(db, tid, logic[i-1].colid)){
			
						case DB_TY_BOOL:
						case DB_TY_INT:
							_deb_delete("INT/BOOL %i", logic[i-1].ival);
							if(sqlite3_bind_int(stmt, i, logic[i-1].ival) != SQLITE_OK){
								return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
							}
						break;
			
						case DB_TY_BIGINT:
							_deb_delete("BIGINT %i", logic[i-1].ival);
							if(sqlite3_bind_int64(stmt, i, logic[i-1].ival) != SQLITE_OK){
								return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
							}
						break;
			
						case DB_TY_REAL:
							_deb_delete("REAL %f", (double)logic[i-1].ival);
							if(sqlite3_bind_double(stmt, i, (double)logic[i-1].ival) != SQLITE_OK){
								return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
							}
						break;
			
						case DB_TY_DATE:
							_deb_delete("DATE %s", logic[i-1].sval);
							if(d13_resolve_date(logic[i-1].sval, date) != E13_OK){
								continue;
							}
			
							if(sqlite3_bind_int(stmt, i, (int)d13_jdayno(date[0], date[1], date[2])) != SQLITE_OK){
								return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
							}
							break;
			
						case DB_TY_D13STIME:
							_deb_delete("D13STIME %i", logic[i-1].ival);
							if(sqlite3_bind_int64(stmt, i, logic[i-1].ival) != SQLITE_OK){
								return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
							}
						break;
			
						case DB_TY_TEXT:
							_deb_delete("TEXT %s", logic[i-1].sval);
							if(sqlite3_bind_text(stmt, i, logic[i-1].sval, -1, NULL) != SQLITE_OK){
								return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
							}
						break;
			
						default:
							//set ival as size for blob
							_deb_delete("BLOB %s size %i", logic[i-1].sval, logic[i-1].ival);
							if(sqlite3_bind_blob(stmt, i, logic[i-1].sval, logic[i-1].ival, NULL) != SQLITE_OK){
								return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
							}
							break;
			
						}
			
			       //     }//if(!(logic[i-1].col_flags & DB_LOGICF_COL_CMP))
			
			        }//for
			        	
			    }//if(nlogic)	                
                
loop:
				switch(sqlite3_step(LITE_ST(st))){

				case SQLITE_ROW:
					goto loop;
					break;
				case SQLITE_DONE:
				case SQLITE_OK:
					_deb_delete("OK");
					//everything's true but no real changes!
					//if(!sqlite3_changes(LITE(db))) return E13_CONTINUE;//won't work in non-exist case
					return E13_OK;
					break;
				default:
					_deb_delete("fails here");
					return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
					break;

				}//switch(step)

                break;

            default:
            	_deb_delete("fails here");
                return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
                break;
        }//switch(prepare)	
				
		break;//case SQLITE
		
	default:
		return e13_ierror(&db->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
		break;		
			
    }//switch(db->driver)

    return E13_OK;

}


/*
int get_last_id(struct db_s* db, enum table_id tid){

    char sql[MAXSQL];
    int row;

    sqlite3_stmt* stmt;

    snprintf(sql, MAXSQL, "SELECT * FROM %s ORDER BY %s DESC;", table_info[tid].name, "id");

    if(sqlite3_prepare_v2(db->h, sql, -1, &stmt, NULL) != SQLITE_OK){
        perrdb(sqlite3_errmsg(db->h));
        return -1;
    }

    switch(sqlite3_step(stmt)){
    case SQLITE_ROW:
        row = sqlite3_column_int(stmt, id_col(tid, "id"));
        sqlite3_finalize(stmt);
        return row;
        break;

    case SQLITE_DONE:
    case SQLITE_OK:
        sqlite3_finalize(stmt);
        //no entries
        return 0;
        break;
    default:
        perrdb(sqlite3_errmsg(db->h));
        sqlite3_finalize(stmt);
        break;

    }

    return -1;

}
*/

error13_t db_istable_physical(struct db13 *db, char *name){
    char sql[MAXSQL];

    //SQLITE: SELECT name FROM sqlite_master WHERE type = 'table' AND name = 'table_name';

    snprintf(sql, MAXSQL, "SELECT name FROM sqlite_master WHERE type = 'table' AND name = '%s';", name);

    switch(db->driver){
    case DB_DRV_NULL:
        return E13_OK;
        break;

    case DB_DRV_SQLITE:
        switch(sqlite3_exec(LITE(db), sql, NULL, NULL, NULL)){
        case SQLITE_DONE:
        case SQLITE_OK:
            return E13_CONTINUE;
            break;

        case SQLITE_ROW:
            return E13_OK;
            break;

        default:
            return e13_ierror(&db->e, E13_SYSE, "s", sqlite3_errmsg(LITE(db)));
            break;
        }

        break;

    default:
        return e13_ierror(&db->e, E13_IMPLEMENT, "s", msg13(M13_DRIVERNOTSUPPORTED));
        break;
    }

	return e13_error(E13_SYSE);
}

