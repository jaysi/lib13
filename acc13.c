//#define NDEBUG
#include "include/lib13.h"

#define _is_init(ac)    ((ac)->magic == MAGIC13_AC13?true_:false_)

#define ACC_TABLE_INFO              "core"
#define ACC_TABLE_GROUP             "grp"
#define ACC_TABLE_USER              "usr"
#define ACC_TABLE_MEMBERSHIP        "memb"
#define ACC_TABLE_ACL               "acc"
#define ACC_TABLE_OBJ				"obj"
//#define ACC_TABLE_FREEOBJ           "objfree"
#define ACC_TABLE_LOG				"log"

#define _ACC_FREEGIDF_INIT	(0x00)
#define _ACC_FREEGIDF_ANY	(0x01<<0)
#define _ACC_FREEUIDF_INIT	(0x00)
#define _ACC_FREEUIDF_ANY	(0x01<<0)
#define _ACC_FREEOBJIDF_INIT	(0x00)
#define _ACC_FREEOBJIDF_ANY	(0x01<<0)

#define ACC_NTABLES 7

#define _deb_acc_init   _NullMsg
#define _deb_grp_list   _NullMsg
#define _deb_grp_add    _NullMsg
#define _deb_get_free_gid _NullMsg
#define _deb_grp_chk    _NullMsg
#define _deb_grp_rm     _NullMsg
#define _deb_usr_list   _NullMsg
#define _deb_usr_add    _NullMsg
#define _deb_get_free_uid _NullMsg
#define _deb_usr_chk    _NullMsg
#define _deb_usr_rm     _NullMsg
#define _deb_obj_list   _NullMsg
#define _deb_obj_add    _NullMsg
#define _deb_get_free_objid _NullMsg
#define _deb_obj_chk    _NullMsg
#define _deb_obj_rm     _DebugMsg
#define _deb_set_parent _DebugMsg
#define _deb_set_owner _DebugMsg
#define _deb_acc_login	_NullMsg
#define _deb_pchk		_NullMsg

//CONTROL COLUMNS

//copyinfo
#define ACC_TABLE_INFO_COLS    4
//RegID, owner, masterpass(hash), FLAGS(FAKE)

#define ACC_TABLE_OBJ_COLS		9
//RegID, id, name, pid, stat, uid, gid, perm, FLAGS(FAKE)

//group
#define ACC_TABLE_GROUP_COLS    5
//RegID, id, name, stat, FLAGS(FAKE)

//user
#define ACC_TABLE_USER_COLS    8
//RegID, id, name, pass(hashSHA512), lastlogin, lastlogout, stat, FLAGS(FAKE)

//membership
#define ACC_TABLE_MEMBERSHIP_COLS    6
//RegID, nrow, gid, uid, stat, FLAGS(FAKE)

//acl
#define ACC_TABLE_ACL_COLS    7
//RegID, nrow, gid, uid, objid, perm, FLAGS(FAKE)struct db_stmt st;

//freeobj - free'ed object strs
#define ACC_TABLE_FREEOBJ_COLS  4
//RegID, objid, type, FLAGS(FAKE)

static inline error13_t _acc_assert_user(char* name, uid13_t uid){
	if(!name){
		switch(uid){
			//case UID13_INVAL: needed for set_owner
			case UID13_NONE:
			case UID13_ANY:
			case UID13_MAX:
			case UID13_FIRST:
				return e13_error(E13_NOTVALID);
				break;
			default:
				break;
		}
	} else {
		if(	!strcmp(name, ACC_USER_ROOT)	||
			!strcmp(name, ACC_USER_SYSTEM)	||
			!strcmp(name, ACC_USER_MANAGER) ||
			!strcmp(name, ACC_USER_DEBUG)	||
			!strcmp(name, ACC_USER_THIS)	||
			!strcmp(name, ACC_USER_GUEST)	||
			!strcmp(name, ACC_USER_ALL)
		) return e13_error(E13_NOTVALID);
	}
	
	return E13_OK;
}

static inline error13_t _acc_assert_group(char* name, gid13_t gid){
	if(!name){
		switch(gid){
			//case GID13_INVAL: needed for set_owner
			case GID13_NONE:
			case GID13_ANY:
			case GID13_MAX:
			case GID13_FIRST:
				return e13_error(E13_NOTVALID);
				break;
			default:
				break;
		}
	} else {
		if(	!strcmp(name, ACC_GROUP_ROOT)	||
			!strcmp(name, ACC_GROUP_SYSTEM)	||
			!strcmp(name, ACC_GROUP_MANAGER) ||
			!strcmp(name, ACC_GROUP_DEBUG)	||
			!strcmp(name, ACC_GROUP_THIS)	||
			!strcmp(name, ACC_GROUP_GUEST)	||
			!strcmp(name, ACC_GROUP_ALL)
		) return e13_error(E13_NOTVALID);
	}
	
	return E13_OK;
}

static inline error13_t _acc_assert_obj(char* name, objid13_t objid){
	if(!name){
		switch(objid){
			case OBJID13_INVAL:
			case OBJID13_NONE:
			//case OBJID13_ANY:
			case OBJID13_MAX:
			case OBJID13_FIRST:
				return e13_error(E13_NOTVALID);
				break;
			default:
				break;
		}
	} else {
		if(
			!strcmp(name, ACC_OBJ_ALL)
		) return e13_error(E13_NOTVALID);
	}
	
	return E13_OK;
}

error13_t acc_init(struct access13* ac, struct db13* db, regid_t regid){

    error13_t ret;

    if(!db_isinit(db)){
		return e13_error(E13_MISUSE);
    }

    if(!db_isopen(db)){
        return e13_error(E13_MISUSE);
    }

    if((ret = db_set_table_slots(db, ACC_NTABLES)) != E13_OK){
        return ret;
    }

    _deb_acc_init("init %s", ACC_TABLE_INFO);

    if((ret = db_define_table(  db,
                                ACC_TABLE_INFO,
                                ACC_TABLE_INFO,
                                DB_TABLEF_CORE|DB_TABLEF_ENCRYPT|DB_TABLEF_MALLOC,
                                ACC_TABLE_INFO_COLS,

                                "RegID",
                                DB_T_INT,
                                "شماره ثبت",
                                "",
                                DB_COLF_HIDE|DB_COLF_AUTO,

                                "owner",
                                DB_T_TEXT,
                                "مالک",
                                "",
                                0,

                                "masterpass",
                                DB_T_RAW,
                                "کلمه عبور",
                                "",
                                0,

                                "flags",
                                DB_T_INT,
                                "پرچم",
                                "",
                                DB_COLF_VIRTUAL|DB_COLF_HIDE

                    )) != E13_OK){

        return e13_error(E13_NOLFS);

    }
    
    _deb_acc_init("init %s", ACC_TABLE_OBJ);

    if((ret = db_define_table(  db,
                                ACC_TABLE_OBJ,
                                ACC_TABLE_OBJ,
                                DB_TABLEF_CTL|DB_TABLEF_ENCRYPT|DB_TABLEF_MALLOC,
                                ACC_TABLE_OBJ_COLS,

                                "RegID",
                                DB_T_INT,
                                "شماره ثبت",
                                "",
                                DB_COLF_HIDE|DB_COLF_AUTO,

                                "id",
                                DB_T_BIGINT,
                                "شناسه",
                                "",
                                DB_COLF_SINGULAR|DB_COLF_AUTO,

                                "name",
                                DB_T_TEXT,
                                "نام",
                                "",
                                DB_COLF_SINGULAR,                            

                                "pid",//parent objid
                                DB_T_BIGINT,
                                "شناسه والد",
                                "",
                                DB_COLF_AUTO,

                                "stat",
                                DB_T_INT,
                                "وضعیت",
                                "",
                                0,
                                
                                "uid",//owner
                                DB_T_INT,
                                "",
                                "",
                                DB_COLF_AUTO,
                                
                                "gid",//group
                                DB_T_INT,
                                "",
                                "",
                                DB_COLF_AUTO,
                                
                                "perm",//permission bits
                                DB_T_INT,
                                "",
                                "",
                                DB_COLF_AUTO,

                                "flags",
                                DB_T_INT,
                                "پرچم",
                                "",
                                DB_COLF_VIRTUAL|DB_COLF_HIDE

                    )) != E13_OK){

        return e13_error(E13_NOLFS);

    }
    

    _deb_acc_init("init %s", ACC_TABLE_GROUP);

    if((ret = db_define_table(  db,
                                ACC_TABLE_GROUP,
                                ACC_TABLE_GROUP,
                                DB_TABLEF_CTL|DB_TABLEF_ENCRYPT|DB_TABLEF_MALLOC,
                                ACC_TABLE_GROUP_COLS,

                                "RegID",
                                DB_T_INT,
                                "شماره ثبت",
                                "",
                                DB_COLF_HIDE|DB_COLF_AUTO,

                                "id",
                                DB_T_INT,
                                "شناسه",
                                "",
                                DB_COLF_SINGULAR|DB_COLF_AUTO,

                                "name",
                                DB_T_TEXT,
                                "نام",
                                "",
                                DB_COLF_SINGULAR,

                                "stat",
                                DB_T_INT,
                                "وضعیت",
                                "",
                                0,

                                "flags",
                                DB_T_INT,
                                "پرچم",
                                "",
                                DB_COLF_VIRTUAL|DB_COLF_HIDE

                    )) != E13_OK){

        return e13_error(E13_NOLFS);

    }

    _deb_acc_init("init %s", ACC_TABLE_USER);

    if((ret = db_define_table(  db,
                                ACC_TABLE_USER,
                                ACC_TABLE_USER,
                                DB_TABLEF_CTL|DB_TABLEF_ENCRYPT|DB_TABLEF_MALLOC,
                                ACC_TABLE_USER_COLS,

                                "RegID",
                                DB_T_INT,
                                "شماره ثبت",
                                "",
                                DB_COLF_HIDE|DB_COLF_AUTO,

                                "id",
                                DB_T_INT,
                                "شناسه",
                                "",
                                DB_COLF_SINGULAR|DB_COLF_AUTO,

                                "name",
                                DB_T_TEXT,
                                "نام",
                                "",
                                DB_COLF_SINGULAR,

                                "pass",
                                DB_T_RAW,
                                "کلمه عبور",
                                "",
                                0,

                                "lastlogin",
                                DB_T_D13STIME,
                                "زمان آخرین ورود",
                                "",
                                0,

                                "lastlogout",
                                DB_T_D13STIME,
                                "زمان آخرین خروج",
                                "",
                                0,

                                "stat",
                                DB_T_INT,
                                "وضعیت",
                                "",
                                0,

                                "flags",
                                DB_T_INT,
                                "پرچم",
                                "",
                                DB_COLF_VIRTUAL|DB_COLF_HIDE

                    )) != E13_OK){

        return e13_error(E13_NOLFS);

    }

    _deb_acc_init("init %s", ACC_TABLE_MEMBERSHIP);

    if((ret = db_define_table(  db,
                                ACC_TABLE_MEMBERSHIP,
                                ACC_TABLE_MEMBERSHIP,
                                DB_TABLEF_CTL|DB_TABLEF_ENCRYPT|DB_TABLEF_MALLOC,
                                ACC_TABLE_MEMBERSHIP_COLS,

                                "RegID",
                                DB_T_INT,
                                "شماره ثبت",
                                "",
                                DB_COLF_HIDE|DB_COLF_AUTO,

                                "nrow",
                                DB_T_BIGINT,
                                "ردیف",
                                "",
                                DB_COLF_HIDE|DB_COLF_AUTO,

                                "gid",
                                DB_T_INT,
                                "گروه",
                                "@group:id>name",
                                DB_COLF_LIST|DB_COLF_TRANSL,

                                "uid",
                                DB_T_INT,
                                "کاربر",
                                "@user:id>name",
                                DB_COLF_LIST|DB_COLF_TRANSL,

                                "stat",
                                DB_T_INT,
                                "وضعیت",
                                "",
                                0,

                                "flags",
                                DB_T_INT,
                                "پرچم",
                                "",
                                DB_COLF_VIRTUAL|DB_COLF_HIDE

                    )) != E13_OK){

        return e13_error(E13_NOLFS);

    }

    _deb_acc_init("init %s", ACC_TABLE_ACL);

    if((ret = db_define_table(  db,
                                ACC_TABLE_ACL,
                                ACC_TABLE_ACL,
                                DB_TABLEF_CTL|DB_TABLEF_ENCRYPT|DB_TABLEF_MALLOC,
                                ACC_TABLE_ACL_COLS,

                                "RegID",
                                DB_T_INT,
                                "شماره ثبت",
                                "",
                                DB_COLF_HIDE|DB_COLF_AUTO,

                                "nrow",
                                DB_T_BIGINT,
                                "ردیف",
                                "",
                                DB_COLF_HIDE|DB_COLF_AUTO,

                                "gid",
                                DB_T_INT,
                                "گروه",
                                "@group:id>name",
                                DB_COLF_LIST|DB_COLF_TRANSL,

                                "uid",
                                DB_T_INT,
                                "کاربر",
                                "@user:id>name",
                                DB_COLF_LIST|DB_COLF_TRANSL,

                                "objid",
                                DB_T_BIGINT,
                                "objid",
                                "",
                                0,

                                "perm",
                                DB_T_INT,
                                "دسترسی",
                                "",
                                0,

                                "flags",
                                DB_T_INT,
                                "پرچم",
                                "",
                                DB_COLF_VIRTUAL|DB_COLF_HIDE

                    )) != E13_OK){

        return e13_error(E13_NOLFS);

    }

    _deb_acc_init("check db_istable_physical %s", ACC_TABLE_INFO);

    switch((ret = db_istable_physical(db, ACC_TABLE_INFO))){
    case E13_CONTINUE:
        _deb_acc_init("db_istable_physical %s CONTINUE", ACC_TABLE_INFO);
        if((ret = db_create_table(db, db_get_tid_byname(db, ACC_TABLE_INFO))) != E13_OK){
            _deb_acc_init("!db_create_table %s", ACC_TABLE_INFO);
            return ret;
        }

		//create default users and groups, TODO: handle errors better!
		ret = acc_group_add(ac, ACC_GROUP_ALL);
		if(ret == E13_OK) ret = acc_user_add(ac, ACC_USER_ALL, ACC_USER_ALL_PASSWORD);

        ret = E13_CONTINUE;
        break;
    case E13_OK:
        //TODO: the db struct must be ok! check it!
        break;
    default:
        _deb_acc_init("!db_istable_physical %s", ACC_TABLE_INFO);
        return ret;
        break;
    }

    if(ret == E13_OK){//the db struct must be ok! check it!
		
        if((ret = db_istable_physical(db, ACC_TABLE_GROUP)) != E13_OK){ _deb_acc_init("!db_istable_physical %s", ACC_TABLE_GROUP); return e13_error(E13_CORRUPT); }
        if((ret = db_istable_physical(db, ACC_TABLE_USER)) != E13_OK){ _deb_acc_init("!db_istable_physical %s", ACC_TABLE_USER); return e13_error(E13_CORRUPT); }
        if((ret = db_istable_physical(db, ACC_TABLE_MEMBERSHIP)) != E13_OK){ _deb_acc_init("!db_istable_physical %s", ACC_TABLE_MEMBERSHIP); return e13_error(E13_CORRUPT); }
        if((ret = db_istable_physical(db, ACC_TABLE_OBJ)) != E13_OK){ _deb_acc_init("!db_istable_physical %s", ACC_TABLE_OBJ); return e13_error(E13_CORRUPT); }        
        if((ret = db_istable_physical(db, ACC_TABLE_ACL)) != E13_OK){ _deb_acc_init("!db_istable_physical %s", ACC_TABLE_ACL); return e13_error(E13_CORRUPT); }
        //if((ret = db_istable_physical(db, ACC_TABLE_FREEOBJ)) != E13_OK){ _deb_acc_init("!db_istable_physical %s", ACC_TABLE_FREEOBJ); return e13_error(E13_CORRUPT); }

        //TODO: ok, extract regid

    } else {//everything must init.
		
        if((ret = db_create_table(db, db_get_tid_byname(db, ACC_TABLE_GROUP)))){ _deb_acc_init("!db_create_table %s", ACC_TABLE_GROUP); return ret; }
        if((ret = db_create_table(db, db_get_tid_byname(db, ACC_TABLE_USER)))){ _deb_acc_init("!db_create_table %s", ACC_TABLE_USER); return ret; }
        if((ret = db_create_table(db, db_get_tid_byname(db, ACC_TABLE_MEMBERSHIP)))){ _deb_acc_init("!db_create_table %s", ACC_TABLE_MEMBERSHIP); return ret; }
        if((ret = db_create_table(db, db_get_tid_byname(db, ACC_TABLE_OBJ)))){ _deb_acc_init("!db_create_table %s", ACC_TABLE_OBJ); return ret; }	        
        if((ret = db_create_table(db, db_get_tid_byname(db, ACC_TABLE_ACL)))){ _deb_acc_init("!db_create_table %s", ACC_TABLE_ACL); return ret; }
        //if((ret = db_create_table(db, db_get_tid_byname(db, ACC_TABLE_FREEOBJ)))){ _deb_acc_init("!db_create_table %s", ACC_TABLE_FREEOBJ); return ret; }

    }

    ac->db = db;

    ac->hash = &sha256;
    ac->hashlen = SHA256_HASH_LEN;

    ac->magic = MAGIC13_AC13;

    return ret;

}

error13_t acc_set_hash(struct access13* ac, size_t hashlen, void *hashfn){

    ac->hash = hashfn;
    ac->hashlen = hashlen;

    return E13_OK;
}



/*********************************  GROUP FN  *********************************/

//return CONTINUE if an old gid reused
static inline error13_t _acc_get_free_gid(struct access13* ac, gid13_t* id,
										uint8_t flags){

    struct db_stmt st;
    struct db_logic_s logic;
    error13_t ret;
    db_table_id tid;
    db_colid_t cid;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    if(flags & _ACC_FREEGIDF_ANY){
		*id = GID13_ANY;
		return E13_OK;
    }

	*id = GID13_FIRST;

    tid = db_get_tid_byname(ac->db, ACC_TABLE_GROUP);

    _deb_get_free_gid("tid: %u", tid);

    //logic.col = db_get_colid_byname(ac->db, tid, "stat");
    logic.colname = "stat";
    //logic.comb = DB_LOGICOMB_NONE;
    logic.flags = DB_LOGICF_DEF;
    logic.logic = DB_LOGIC_EQ;
    logic.ival = ACC_GRP_STT_REMOVED;

    if((ret = db_collect(ac->db, tid, NULL, 1, &logic, NULL, DB_SO_DONT, 0, &st)) != E13_OK){
        _deb_get_free_gid("collect fails %i", ret);
        return ret;
    }

    _deb_get_free_gid("collect rets %i", ret);

    if((cid = db_get_colid_byname(ac->db, tid, "id")) == DB_COLID_INVAL) return e13_error(E13_NOTVALID);

    _deb_get_free_gid("colid(id): %u", cid);

    switch((ret = db_step(&st))){
        case E13_CONTINUE:
        _deb_get_free_gid("step rets %i, CONTINUE", ret);
        ret = db_column_int(&st, cid, (int*)id);
        if(ret == E13_OK) ret = E13_CONTINUE;
        _deb_get_free_gid("col int ret %i, OK->CONTINUE", ret);
        break;
        case E13_OK:
        _deb_get_free_gid("step rets %i, OK", ret);
        break;
        default:
        _deb_get_free_gid("step rets %i, fin", ret);
        db_finalize(&st);
        return ret;
        break;
    }

    if(ret == E13_CONTINUE) return ret;

    db_finalize(&st);

    _deb_get_free_gid("collecting max id");

    //find max id
    if((ret = db_collect(ac->db, tid, NULL, 0, NULL, "id", DB_SO_DEC, 1, &st)) != E13_OK){
        _deb_get_free_gid("collecting max id fails %i", ret);
        return ret;
    }

    _deb_get_free_gid("collecting max id done");

    switch((ret = db_step(&st))){
        case E13_CONTINUE:
        _deb_get_free_gid("step CONTINUE");
        ret = db_column_int(&st, cid, (int*)id);
        (*id)++;
        if(*id == GID13_INVAL) (*id)++;
        else if(*id >= GID13_MAX) ret = e13_error(E13_FULL);
        _deb_get_free_gid("id %u, fin", *id);
        db_finalize(&st);
        break;
        case E13_OK:
        _deb_get_free_gid("step OK, fin");
        db_finalize(&st);
        break;
        default:
        _deb_get_free_gid("ret %i, fin", ret);
        db_finalize(&st);
        return ret;
        break;
    }

    return ret;
}

error13_t acc_group_set_stat(struct access13* ac, char* name, gid13_t gid, int stt){

    struct db_stmt st;
    db_table_id tid;
    int stat[1];
    struct db_logic_s logic;
    db_colid_t colid;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    if(_acc_assert_group(name, gid) != E13_OK) return e13_error(E13_NOTVALID);

    tid = db_get_tid_byname(ac->db, ACC_TABLE_GROUP);

    if(name){
		logic.colname = "name";
		logic.flags = DB_LOGICF_DEF;
		logic.sval = name;
		logic.logic = DB_LOGIC_LIKE;
    } else {
		logic.colname = "id";
		logic.flags = DB_LOGICF_DEF;
		logic.ival = gid;
		logic.logic = DB_LOGIC_EQ;
    }

    if((colid = db_get_colid_byname(ac->db, tid, "stat")) == DB_COLID_INVAL) return e13_error(E13_NOTVALID);

    stat[0] = stt;

    return db_update(ac->db, tid, logic, 1, &colid, (uchar**)&stat, NULL,&st);

}

error13_t acc_group_add(struct access13 *ac, char *name){

    struct db_stmt st;
    db_table_id tid;
    db_colid_t colids[2];
    uchar* cols[ACC_TABLE_GROUP_COLS];
    size_t size[ACC_TABLE_GROUP_COLS];
    gid13_t gid;
    error13_t ret;
    int stat;
    struct db_logic_s logic;
    struct group13 group;
    uint8_t flags = _ACC_FREEGIDF_INIT;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }
	
	if(_acc_assert_group(name, gid) != E13_OK) return e13_error(E13_NOTVALID);
	
    if(strcmp(name, ACC_GROUP_ALL)){
		if(_acc_assert_group(name, GID13_NONE) != E13_OK)
			return e13_error(E13_NOTVALID);
    } else {
    	flags |= _ACC_FREEGIDF_ANY;
    }

    tid = db_get_tid_byname(ac->db, ACC_TABLE_GROUP);
    _deb_grp_add("tid: %u", tid);
    if(tid == DB_TID_INVAL) return e13_error(E13_CORRUPT);

    //1. check for existence
    if(acc_group_chk(ac, name, GID13_NONE, &group) == E13_OK){
        _deb_grp_add("group exists");
        
        //if exists and stat is ACC_GRP_STT_REMOVED replace it        		
        
        if(group.stt != ACC_GRP_STT_REMOVED) return e13_error(E13_EXISTS);        
        
    }

    _deb_grp_add("group not exists");

    //2. add it

//    cols = (char**)m13_malloc(sizeof(char*)*ACC_TABLE_GROUP_COLS);
//    if(!cols) return e13_error(E13_NOMEM);

    switch((ret = _acc_get_free_gid(ac, &gid, flags))){
    case E13_CONTINUE://an old gid is re-used, update
        _deb_grp_add("reusing old id, id = %u", gid);
        stat = ACC_GRP_STT_ACTIVE;
        //logic.col = db_get_colid_byname(ac->db, tid, "id");
        logic.colname = "id";
//        _deb_grp_add("logic.colid(id): %u", logic.col);
//        logic.comb = DB_LOGICOMB_NONE;
        logic.ival = gid;
        logic.flags = DB_LOGICF_DEF;
        logic.logic = DB_LOGIC_EQ;
        cols[0] = (uchar*)name;
        cols[1] = (uchar*)&stat;
        if((colids[0] = db_get_colid_byname(ac->db, tid, "name")) == DB_COLID_INVAL) return e13_error(E13_NOTVALID);
        if((colids[1] = db_get_colid_byname(ac->db, tid, "stat")) == DB_COLID_INVAL) return e13_error(E13_NOTVALID);
        _deb_grp_add("colids[0](name): %u", colids[0]);
        _deb_grp_add("colids[1](stat): %u", colids[1]);
        ret = db_update(ac->db, tid, logic, 2, colids, cols, NULL, &st);
        _deb_grp_add("db_update ret: %i", ret);
        db_finalize(&st);
        _deb_grp_add("re-used old id");
        return ret;
        break;
    case E13_OK://new id, insert
        _deb_grp_add("need new id");
        break;

    default:
        _deb_grp_add("ret: %i", ret);
        return ret;
        break;

    }

    stat = ACC_GRP_STT_ACTIVE;
    cols[0] = (uchar*)&ac->regid;
    cols[1] = (uchar*)&gid;
    cols[2] = (uchar*)name;
    cols[3] = (uchar*)&stat;
    cols[4] = NULL;/*(uchar*)&flags;*/
    size[0] = sizeof(regid_t);
    size[1] = sizeof(gid13_t);
    size[2] = strlen(name) + 1;
    size[3] = sizeof(int);
//    size[4] = sizeof(int);
    if((ret = db_insert(ac->db, tid, cols, size, &st)) != E13_OK){
        db_finalize(&st);
//        m13_free(cols);
        _deb_grp_add("fails here");
        return ret;
    }

    _deb_grp_add("insert ok, fin;");

    db_finalize(&st);
//    m13_free(cols);
    _deb_grp_add("done");

    return E13_OK;

}

/*
remove all uid record from tid
TODO: THIS COULD BE POSSIBLE!
error13_t _acc_rm_uid_entries(struct access13* ac,
							uid13_t nuid, uid13_t uid[],
							db_table_id ntid, db_table_id tid[]){
*/
static inline error13_t _acc_rm_uid_entries(struct access13* ac,
											uid13_t nuid,
											uid13_t* uid,
											db_table_id ntid,
											db_table_id* tid){
    struct db_stmt st;
    struct db_logic_s logic;
    error13_t ret;
    uid13_t iuid;
    db_table_id itid;

//	not needed
//    if(!_is_init(ac)){
//        return e13_error(E13_MISUSE);
//    }

	for(itid = 0; itid < ntid; itid++){

		//logic.col = db_get_colid_byname(ac->db, tid[itid], "uid");
		logic.colname = "uid";

		for(iuid = 0; iuid < nuid; iuid++){

			//logic.comb = DB_LOGICOMB_NONE;
			logic.flags = DB_LOGICF_DEF;
			logic.logic = DB_LOGIC_EQ;
			logic.ival = uid[iuid];

			_deb_usr_rm("removing...");
			if((ret = db_delete(ac->db, tid[itid], 1, &logic, &st)) != E13_OK){
				_deb_usr_rm("fails %i", ret);
				return ret;
			}
			_deb_usr_rm("remove done");

			switch((ret = db_step(&st))){
				case E13_CONTINUE:
				case E13_OK:
				break;
				default:
				//TODO: what to do on error?
				_deb_usr_rm("step %i", ret);
				return ret;
				break;
			}

            db_reset(&st);

		}//iuid
	}//itid

    return ret;
}

static inline error13_t _acc_mod_uid_entries(	struct access13* ac,
												uid13_t nuid,
												uid13_t uid[],												
												uid13_t newuid[],												
												db_table_id ntid,
												db_table_id tid[]){
    struct db_stmt st;
    struct db_logic_s logic;
    error13_t ret;
    uid13_t iuid;
    db_table_id itid;
	db_colid_t colids[1];
	uchar* cols[1];	

//	not needed
//    if(!_is_init(ac)){
//        return e13_error(E13_MISUSE);
//    }

	for(itid = 0; itid < ntid; itid++){

		//logic.col = db_get_colid_byname(ac->db, tid[itid], "uid");
		logic.colname = "uid";

		for(iuid = 0; iuid < nuid; iuid++){

			//logic.comb = DB_LOGICOMB_NONE;
			logic.flags = DB_LOGICF_DEF;
			logic.logic = DB_LOGIC_EQ;
			logic.ival = uid[iuid];

	
			cols[0] = (uchar*)&newuid[iuid];
			if((colids[0] = db_get_colid_byname(ac->db, tid[itid], "uid")) == DB_COLID_INVAL) return e13_error(E13_NOTVALID);	
				
			ret = db_update(ac->db, tid[itid], logic, 1, colids, cols, NULL, &st);
			
		    _deb_obj_rm("db_update ret: %i", ret);
		    db_finalize(&st);
		    return ret;

			switch((ret = db_step(&st))){
				case E13_CONTINUE:
				case E13_OK:
				break;
				default:
				//TODO: what to do on error?
				_deb_usr_rm("step %i", ret);
				return ret;
				break;
			}

            db_reset(&st);

		}//iuid
	}//itid

    return ret;
}

static inline error13_t _acc_rm_gid_entries(struct access13* ac,
											gid13_t ngid,
											gid13_t gid[],
											db_table_id ntid,
											db_table_id tid[]){
    struct db_stmt st;
    struct db_logic_s logic;
    error13_t ret;
    gid13_t igid;
    db_table_id itid;

//	not needed
//    if(!_is_init(ac)){
//        return e13_error(E13_MISUSE);
//    }

	for(itid = 0; itid < ntid; itid++){

		//logic.col = db_get_colid_byname(ac->db, tid[itid], "gid");
		logic.colname = "gid";

		for(igid = 0; igid < ngid; igid++){

			//logic.comb = DB_LOGICOMB_NONE;
			logic.flags = DB_LOGICF_DEF;
			logic.logic = DB_LOGIC_EQ;
			logic.ival = gid[igid];

			_deb_grp_rm("collecting...");
			if((ret = db_delete(ac->db, tid[itid], 1, &logic, &st)) != E13_OK){
				_deb_grp_rm("fails %i", ret);
				return ret;
			}
			_deb_grp_rm("collecting done");

			switch((ret = db_step(&st))){
				case E13_CONTINUE:
				case E13_OK:
				break;
				default:
				//TODO: what to do on error?
				_deb_grp_rm("step %i", ret);
				return ret;
				break;
			}

            db_reset(&st);

		}//igid
	}//itid

    return ret;
}

error13_t acc_group_rm(struct access13 *ac, char *name, gid13_t gid){

    struct db_stmt st;
    db_table_id tid, tids[2];
    error13_t ret;
    int stt;
    struct db_logic_s logic;
    db_colid_t colids[1];
    uchar* cols[1];
    struct group13 group;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    if(_acc_assert_group(name, gid) != E13_OK) return e13_error(E13_NOTVALID);

    tid = db_get_tid_byname(ac->db, ACC_TABLE_GROUP);
    if(tid == DB_TID_INVAL) return e13_error(E13_CORRUPT);

    //1. check for existence
	if(acc_group_chk(ac, name, gid, &group) != E13_OK){
		return e13_error(E13_NOTFOUND);
	}

	if(group.stt == ACC_GRP_STT_REMOVED) return e13_error(E13_OK);

    tids[0] = db_get_tid_byname(ac->db, ACC_TABLE_MEMBERSHIP);
    tids[1] = db_get_tid_byname(ac->db, ACC_TABLE_ACL);    

    if((ret = _acc_rm_gid_entries(ac, 1, &group.gid, 2, tids)) != E13_OK){
        return ret;
    }

	stt = ACC_GRP_STT_REMOVED;
    //logic.col = db_get_colid_byname(ac->db, tid, "id");
    logic.colname = "id";
    //_deb_grp_rm("logic.colid(id): %u", logic.col);
    //logic.comb = DB_LOGICOMB_NONE;
    logic.ival = group.gid;
    _deb_grp_rm("gid: %u", group.gid);
    logic.flags = DB_LOGICF_DEF;
    logic.logic = DB_LOGIC_EQ;
    cols[0] = (uchar*)&stt;
    if((colids[0] = db_get_colid_byname(ac->db, tid, "stat")) == DB_COLID_INVAL) return e13_error(E13_NOTVALID);
    ret = db_update(ac->db, tid, logic, 1, colids, cols, NULL, &st);
    _deb_grp_rm("db_update ret: %i", ret);
    db_finalize(&st);
    return ret;

}

/*
error13_t acc_group_rm(struct access13 *ac, char *name, gid13_t gid){

    struct db_stmt st;
    db_table_id tid, tids[2];
    error13_t ret;    
    struct db_logic_s logic;
    struct group13 group;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    if(_acc_assert(name, gid, 0) != E13_OK) return e13_error(E13_NOTVALID);

    tid = db_get_tid_byname(ac->db, ACC_TABLE_GROUP);
    if(tid == DB_TID_INVAL) return e13_error(E13_CORRUPT);

    //1. check for existence
	if(acc_group_chk(ac, name, gid, &group) != E13_OK){
		return e13_error(E13_NOTFOUND);
	}

	if(group.stt == ACC_GRP_STT_REMOVED) return e13_error(E13_OK);

    tids[0] = db_get_tid_byname(ac->db, ACC_TABLE_MEMBERSHIP);
    tids[1] = db_get_tid_byname(ac->db, ACC_TABLE_ACL);

    if((ret = _acc_rm_gid_entries(ac, 1, &group.gid, 2, tids)) != E13_OK){
        return ret;
    }

    //logic.col = db_get_colid_byname(ac->db, tid, "id");
    logic.colname = "id";
    //_deb_grp_rm("logic.colid(id): %u", logic.col);
    //logic.comb = DB_LOGICOMB_NONE;
    logic.ival = group.gid;
    _deb_grp_rm("gid: %u", group.gid);
    logic.flags = DB_LOGICF_DEF;
    logic.logic = DB_LOGIC_EQ;
    
	db_delete(ac->db, tid, 1, &logic, &st);    

    db_finalize(&st);
    
    return ret;

}
*/

error13_t acc_group_chk(struct access13 *ac, char *name, gid13_t gid,
						struct group13* group){

    struct db_stmt st;
    struct db_logic_s logic;
    error13_t ret;
    db_table_id tid;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    tid = db_get_tid_byname(ac->db, ACC_TABLE_GROUP);
    _deb_grp_chk("got tid %u", tid);
    if(tid == DB_TID_INVAL) return e13_error(E13_CORRUPT);

    if(name){
		//logic.col = db_get_colid_byname(ac->db, tid, "name");
		logic.colname = "name";
//		_deb_grp_chk("logic.col %u", logic.col);
		//logic.comb = DB_LOGICOMB_NONE;
		logic.flags = DB_LOGICF_DEF;
		logic.logic = DB_LOGIC_LIKE;
		logic.sval = name;
    } else {
		//logic.col = db_get_colid_byname(ac->db, tid, "id");
		logic.colname = "id";
//		_deb_grp_chk("logic.col %u", logic.col);
		//logic.comb = DB_LOGICOMB_NONE;
		logic.flags = DB_LOGICF_DEF;
		logic.logic = DB_LOGIC_EQ;
		logic.ival = gid;
    }

    _deb_grp_chk("collecting...");
    if((ret = db_collect(ac->db, tid, NULL, 1, &logic, NULL, DB_SO_DONT, 0, &st)) != E13_OK){
        _deb_grp_chk("fails %i", ret);
        return ret;
    }
    _deb_grp_chk("collecting done");

    switch((ret = db_step(&st))){
        case E13_CONTINUE:
        _deb_grp_chk("step CONTINUE");
        ret = db_column_int(&st, db_get_colid_byname(ac->db, tid, "id"), (int*)&group->gid);
        if(ret == E13_OK){
            _deb_grp_chk("id %u", group->gid);
            ret = db_column_int(&st, db_get_colid_byname(ac->db, tid, "stat"), &group->stt);
            _deb_grp_chk("stat %i (%s)", group->stt, ret == E13_OK?"OK":"NOK");
        }
        db_finalize(&st);
        break;
        case E13_OK:
        _deb_grp_chk("step OK");
        db_finalize(&st);
        return e13_error(E13_NOTFOUND);
        break;
        default:
        _deb_grp_chk("step %i", ret);
        return ret;
        break;
    }

    return ret;
}

error13_t acc_group_list(struct access13 *ac, gid13_t *n,
						struct group13 **group){

    struct db_stmt st;
    struct db_logic_s logic;
    error13_t ret;
    db_table_id tid;
    int stt;
    gid13_t id;
    uchar* s;
    size_t slen;
    struct group13* next, *last = NULL;
    *n = GID13_NONE;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    tid = db_get_tid_byname(ac->db, ACC_TABLE_GROUP);
    _deb_grp_list("got tid %u", tid);

//    if((logic.col = db_get_colid_byname(ac->db, tid, "stat")) ==DB_COLID_INVAL){
//		return e13_error(E13_ABORT);
//    }
	logic.colname = "stat";
    //logic.comb = DB_LOGICOMB_NONE;
    logic.flags = DB_LOGICF_DEF;
    logic.ival = ACC_GRP_STT_REMOVED;
    logic.logic = DB_LOGIC_NE;

    _deb_grp_list("collecting...");
    if((ret = db_collect(ac->db, tid, NULL, 1, &logic, "name", DB_SO_INC, 0, &st)) != E13_OK){
        _deb_grp_list("fails@here");
        return ret;
    }
    _deb_grp_list("collecting done.");

    again:
    _deb_grp_list("step.");
    switch((ret = db_step(&st))){
        case E13_CONTINUE:
        _deb_grp_list("step CONTINUE");
        ret = db_column_int(&st, db_get_colid_byname(ac->db, tid, "id"), (int*)&id);
        if(ret == E13_OK){
            _deb_grp_list("id ok, %u", id);
            ret = db_column_int(&st, db_get_colid_byname(ac->db, tid, "stat"), &stt);
        }
        if(ret == E13_OK){
            _deb_grp_list("stat ok, %i", stt);
            ret = db_column_text(&st, db_get_colid_byname(ac->db, tid, "name"), &slen, &s);
        }
        if(ret == E13_OK){
            _deb_grp_list("name ok: %s", s);
            (*n)++;
            _deb_grp_list("n: %u", *n);
            next = (struct group13*)m13_malloc(sizeof(struct group13));
            next->name = (char*)s;
            next->gid = id;
            next->stt = stt;
            next->next = NULL;
            if(!last){
                last = next;
                *group = next;
                _deb_grp_list("added as first");
            } else {
                _deb_grp_list("added as last");
                last->next = next;
                last = next;
            }
        }
        if(ret != E13_OK){
            _deb_grp_list("ret = %i, break;", ret);
            break;
        }
        goto again;
        break;
        case E13_OK:
        _deb_grp_list("step OK");
        db_finalize(&st);
        return E13_OK;
        break;
        default:
        _deb_grp_list("step %i", ret);
        return ret;
        break;
    }

    _deb_grp_list("step %i", ret);
    return ret;

}

error13_t acc_group_list_free(struct group13* group){
    struct group13* first = group, *del;

    while(first){
        del = first;
        first = first->next;
        m13_free(del->name);
        m13_free(del);
    }

    return E13_OK;
}

/*********************************  USER FN  **********************************/

//return CONTINUE if an old uid reused
error13_t _acc_get_free_uid(struct access13* ac, uid13_t* id,
										uint8_t flags){

    struct db_stmt st;
    struct db_logic_s logic;
    error13_t ret;
    db_table_id tid;
    db_colid_t cid;

    _deb_get_free_uid("called.");

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    if(flags & _ACC_FREEUIDF_ANY){
		*id = UID13_ANY;
		return E13_OK;
    }

    *id = UID13_FIRST;

    tid = db_get_tid_byname(ac->db, ACC_TABLE_USER);

    _deb_get_free_uid("tid: %u", tid);

    //logic.col = db_get_colid_byname(ac->db, tid, "stat");
    logic.colname = "stat";
    //logic.comb = DB_LOGICOMB_NONE;
    logic.flags = DB_LOGICF_DEF;
    logic.logic = DB_LOGIC_EQ;
    logic.ival = ACC_USR_STT_REMOVED;

    if((ret = db_collect(ac->db, tid, NULL, 1, &logic, NULL, DB_SO_DONT, 0, &st)) != E13_OK){
        _deb_get_free_uid("collect fails %i", ret);
        return ret;
    }

    _deb_get_free_uid("collect rets %i", ret);

    cid = db_get_colid_byname(ac->db, tid, "id");

    _deb_get_free_uid("colid(id): %u", cid);

    switch((ret = db_step(&st))){
        case E13_CONTINUE:
        _deb_get_free_uid("step rets %i, CONTINUE", ret);
        ret = db_column_int(&st, cid, (int*)id);
        if(ret == E13_OK) ret = E13_CONTINUE;
        _deb_get_free_uid("col int ret %i, OK->CONTINUE", ret);
        break;
        case E13_OK:
        _deb_get_free_uid("step rets %i, OK", ret);
        break;
        default:
        _deb_get_free_uid("step rets %i, fin", ret);
        db_finalize(&st);
        return ret;
        break;
    }

    if(ret == E13_CONTINUE) return ret;

    db_finalize(&st);

    _deb_get_free_uid("collecting max id");

    //find max id
    if((ret = db_collect(ac->db, tid, NULL, 0, NULL, "id", DB_SO_DEC, 1, &st)) != E13_OK){
        _deb_get_free_uid("collecting max id fails %i", ret);
        return ret;
    }

    _deb_get_free_uid("collecting max id done");

    switch((ret = db_step(&st))){
        case E13_CONTINUE:
        _deb_get_free_uid("step CONTINUE");
        ret = db_column_int(&st, cid, (int*)id);
        (*id)++;
        if(*id == UID13_INVAL) (*id)++;
        else if(*id >= UID13_MAX) ret = e13_error(E13_FULL);
        _deb_get_free_uid("id %u, fin", *id);
        db_finalize(&st);
        break;
        case E13_OK:
        _deb_get_free_uid("step OK, fin");
        db_finalize(&st);
        break;
        default:
        _deb_get_free_uid("ret %i, fin", ret);
        db_finalize(&st);
        return ret;
        break;
    }

    return ret;
}

error13_t acc_user_set_stat(struct access13* ac, char* name, uid13_t id, int stt){

    struct db_stmt st;
    db_table_id tid;
    int stat[1];
    struct db_logic_s logic;
    db_colid_t colid;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    if(_acc_assert_user(name, id) != E13_OK) return e13_error(E13_NOTVALID);

    tid = db_get_tid_byname(ac->db, ACC_TABLE_USER);

    if(name){
		//logic.col = db_get_colid_byname(ac->db, tid, "name");
		logic.colname = "name";
		//logic.comb = DB_LOGICOMB_NONE;
		logic.flags = 0;
		logic.sval = name;
		logic.logic = DB_LOGIC_LIKE;
    } else {
//		logic.col = db_get_colid_byname(ac->db, tid, "id");
		logic.colname = "id";
		//logic.comb = DB_LOGICOMB_NONE;
		logic.flags = DB_LOGICF_DEF;
		logic.ival = id;
		logic.logic = DB_LOGIC_EQ;
    }

    colid = db_get_colid_byname(ac->db, tid, "stat");

    stat[0] = stt;

    return db_update(ac->db, tid, logic, 1, &colid, (uchar**)&stat, NULL,&st);

}

error13_t acc_user_add(struct access13 *ac, char *name, char* pass){

    struct db_stmt st;
    db_table_id tid;
    db_colid_t colids[ACC_TABLE_USER_COLS];
    uchar* cols[ACC_TABLE_USER_COLS];
    size_t size[ACC_TABLE_USER_COLS];
    uid13_t uid;
    error13_t ret;
    int stat;
    struct db_logic_s logic;
    uchar passhash[ac->hashlen];
    uint32_t lastlogin = 0UL;
    uint32_t lastlogout = 0UL;
    struct user13 user;
    uint8_t flags = _ACC_FREEUIDF_INIT;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }
    
    if(_acc_assert_user(name, UID13_NONE) != E13_OK) return e13_error(E13_NOTVALID);

    if(strcmp(name, ACC_USER_ALL)){
		if(_acc_assert_user(name, UID13_NONE) != E13_OK)
			return e13_error(E13_NOTVALID);
    } else {
    	flags |= _ACC_FREEUIDF_ANY;
    }

/*
    if(strcmp(name, ACC_USER_ALL)){
		if(_acc_assert_user(name, UID13_NONE, 0) != E13_OK)
			return e13_error(E13_NOTVALID);
    }
*/

    tid = db_get_tid_byname(ac->db, ACC_TABLE_USER);
    _deb_usr_add("tid: %u", tid);
    if(tid == DB_TID_INVAL) return e13_error(E13_CORRUPT);

    //1. check for existence
    if(acc_user_chk(ac, name, UID13_NONE, &user) == E13_OK){
        _deb_usr_add("user exists");        
        if(user.stt != ACC_USR_STT_REMOVED) {
        	m13_free(user.passhash);
			return e13_error(E13_EXISTS);
		}
    }

    _deb_usr_add("user not exists");

    //2. add it

//    cols = (char**)m13_malloc(sizeof(char*)*ACC_TABLE_USER_COLS);
//    if(!cols) return e13_error(E13_NOMEM);

    ac->hash((uchar*)pass, s13_strlen(pass, ACC_MAX_PASSLEN), passhash);
    //passhash[0] = 0;

    switch((ret = _acc_get_free_uid(ac, &uid, flags))){
    case E13_CONTINUE://an old uid is re-used, update
        _deb_usr_add("reusing old id, uid = %lu", uid);
        stat = ACC_USR_STT_OUT;
//        logic.col = db_get_colid_byname(ac->db, tid, "id");
		logic.colname = "id";
        //logic.comb = DB_LOGICOMB_NONE;
        logic.ival = uid;
        logic.flags = DB_LOGICF_DEF;
        logic.logic = DB_LOGIC_EQ;
        cols[0] = (uchar*)name;
        cols[1] = (uchar*)passhash;
        cols[2] = (uchar*)&lastlogin;
        cols[3] = (uchar*)&lastlogout;
        cols[4] = (uchar*)&stat;
        if((colids[0] = db_get_colid_byname(ac->db, tid, "name")) == DB_COLID_INVAL){
				_deb_usr_add("invalid name colid");
				return e13_error(E13_NOTVALID);
        }
        if((colids[1] = db_get_colid_byname(ac->db, tid, "pass")) == DB_COLID_INVAL){
				_deb_usr_add("invalid pass colid");
				return e13_error(E13_NOTVALID);
        }
        if((colids[2] = db_get_colid_byname(ac->db, tid, "lastlogin")) == DB_COLID_INVAL){
				_deb_usr_add("invalid lastlogin colid");
				return e13_error(E13_NOTVALID);
        }
        if((colids[3] = db_get_colid_byname(ac->db, tid, "lastlogout")) == DB_COLID_INVAL){
				_deb_usr_add("invalid lastlogout colid");
				return e13_error(E13_NOTVALID);
        }
        if((colids[4] = db_get_colid_byname(ac->db, tid, "stat")) == DB_COLID_INVAL){
				_deb_usr_add("invalid stat colid");
				return e13_error(E13_NOTVALID);
        }
        size[0] = -1;
        size[1] = ac->hashlen;
        size[2] = sizeof(d13s_time_t);
        size[3] = sizeof(d13s_time_t);
        size[4] = sizeof(int);
        _deb_usr_add("colids[0](name): %u", colids[0]);
        _deb_usr_add("colids[2](pass): %u", colids[1]);
        _deb_usr_add("colids[3](lastlogin): %u", colids[2]);
        _deb_usr_add("colids[4](lastlogout): %u", colids[3]);
        _deb_usr_add("colids[1](stat): %u", colids[4]);
        ret = db_update(ac->db, tid, logic, 5, colids, cols, size, &st);
        _deb_usr_add("db_update ret: %i", ret);
        db_finalize(&st);
        _deb_usr_add("re-used old id");
        return ret;
        break;
    case E13_OK://new id, insert
        _deb_usr_add("need new id, uid = %lu", uid);
        break;

    default:
        _deb_usr_add("ret: %i", ret);
        return ret;
        break;

    }

    stat = ACC_USR_STT_OUT;
    cols[0] = (uchar*)&ac->regid;
    cols[1] = (uchar*)&uid;
    cols[2] = (uchar*)name;
    cols[3] = passhash;
    cols[4] = (uchar*)&lastlogin;
    cols[5] = (uchar*)&lastlogout;
    cols[6] = (uchar*)&stat;
    cols[7] = NULL;/*(uchar*)&flags;*/
    size[0] = sizeof(regid_t);
    size[1] = sizeof(uid13_t);
    size[2] = strlen(name) + 1;
    size[3] = ac->hashlen;
    size[4] = sizeof(d13s_time_t);
    size[5] = sizeof(d13s_time_t);
    size[6] = sizeof(int);
//    size[4] = sizeof(int);
	_deb_usr_add("inserting %s (passhash=%s), uid = %lu...", name, passhash, uid);
    if((ret = db_insert(ac->db, tid, cols, size, &st)) != E13_OK){
        db_finalize(&st);
//        m13_free(cols);
        _deb_usr_add("fails here");
        return ret;
    }

    _deb_usr_add("insert ok, fin;");

    db_finalize(&st);
//    m13_free(cols);
    _deb_usr_add("done");

    return E13_OK;

}

error13_t acc_user_rm(struct access13 *ac, char *name, uid13_t id){

    struct db_stmt st;
    db_table_id tid, tids[2];
    error13_t ret;
    int stt;
    struct db_logic_s logic;
    db_colid_t colids[1];
    uchar* cols[1];
    struct user13 user;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    if(_acc_assert_user(name, id) != E13_OK) return e13_error(E13_NOTVALID);

    tid = db_get_tid_byname(ac->db, ACC_TABLE_USER);
    if(tid == DB_TID_INVAL) return e13_error(E13_CORRUPT);

    //1. check for existence
    if(acc_user_chk(ac, name, id, &user) != E13_OK){
        return e13_error(E13_NOTFOUND);
    }

    if(user.stt == ACC_USR_STT_REMOVED) return e13_error(E13_OK);

    tids[0] = db_get_tid_byname(ac->db, ACC_TABLE_MEMBERSHIP);
    tids[1] = db_get_tid_byname(ac->db, ACC_TABLE_ACL);    

    if((ret = _acc_rm_uid_entries(ac, 1, &user.uid, 2, tids)) != E13_OK) return ret;

    stt = ACC_USR_STT_REMOVED;
    //logic.col = db_get_colid_byname(ac->db, tid, "id");
    logic.colname = "id";
//    _deb_usr_rm("logic.colid(id): %u", logic.col);
    //logic.comb = DB_LOGICOMB_NONE;
    logic.ival = user.uid;
    _deb_usr_rm("uid: %u", user.uid);
    logic.flags = DB_LOGICF_DEF;
    logic.logic = DB_LOGIC_EQ;
    cols[0] = (uchar*)&stt;
    colids[0] = db_get_colid_byname(ac->db, tid, "stat");
    ret = db_update(ac->db, tid, logic, 1, colids, cols, NULL, &st);
    _deb_usr_rm("db_update ret: %i", ret);
    db_finalize(&st);
    return ret;

}

/*
error13_t acc_user_rm(struct access13 *ac, char *name, uid13_t id){

    struct db_stmt st;
    db_table_id tid, tids[2];
    error13_t ret;    
    struct db_logic_s logic;
    struct user13 user;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    if(_acc_assert(name, id, 1) != E13_OK) return e13_error(E13_NOTVALID);

    tid = db_get_tid_byname(ac->db, ACC_TABLE_USER);
    if(tid == DB_TID_INVAL) return e13_error(E13_CORRUPT);

    //1. check for existence
    if(acc_user_chk(ac, name, id, &user) != E13_OK){
        return e13_error(E13_NOTFOUND);
    }

    if(user.stt == ACC_USR_STT_REMOVED) return e13_error(E13_OK);

    tids[0] = db_get_tid_byname(ac->db, ACC_TABLE_MEMBERSHIP);
    tids[1] = db_get_tid_byname(ac->db, ACC_TABLE_ACL);

    if((ret = _acc_rm_uid_entries(ac, 1, &user.uid, 2, tids)) != E13_OK) return ret;
    
    //logic.col = db_get_colid_byname(ac->db, tid, "id");
    logic.colname = "id";
//    _deb_usr_rm("logic.colid(id): %u", logic.col);
    //logic.comb = DB_LOGICOMB_NONE;
    logic.ival = user.uid;
    _deb_usr_rm("uid: %u", user.uid);
    logic.flags = DB_LOGICF_DEF;
    logic.logic = DB_LOGIC_EQ;
    
    db_delete(ac->db, tid, 1, &logic, &st);
    
    db_finalize(&st);
    
    return ret;
}
*/

error13_t acc_user_chpass(struct access13* ac, char* name, uid13_t id,
							char* oldpass,
							char* newpass){
	struct db_stmt st;
    db_table_id tid;
    uchar* col[1];
    struct db_logic_s logic;
    db_colid_t colid;
	uchar passhash[ac->hashlen];

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    if(_acc_assert_user(name, id) != E13_OK) return e13_error(E13_NOTVALID);

    tid = db_get_tid_byname(ac->db, ACC_TABLE_USER);

    if(name){
		//logic.col = db_get_colid_byname(ac->db, tid, "name");
		logic.colname = "name";
		//logic.comb = DB_LOGICOMB_NONE;
		logic.flags = DB_LOGICF_DEF;
		logic.sval = name;
		logic.logic = DB_LOGIC_LIKE;
    } else {
//		logic.col = db_get_colid_byname(ac->db, tid, "id");
		logic.colname = "id";
		//logic.comb = DB_LOGICOMB_NONE;
		logic.flags = DB_LOGICF_DEF;
		logic.ival = id;
		logic.logic = DB_LOGIC_EQ;
    }

    colid = db_get_colid_byname(ac->db, tid, "pass");

    ac->hash(newpass, s13_strlen(newpass, ACC_MAX_PASSLEN), passhash);

	col[0] = passhash;

    return db_update(ac->db, tid, logic, 1, &colid, col, NULL, &st);

}

error13_t acc_user_chk(struct access13 *ac, char *name, uid13_t id,
					struct user13* user){

    struct db_stmt st;
    struct db_logic_s logic;
    error13_t ret;
    db_table_id tid;
    size_t passlen;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    tid = db_get_tid_byname(ac->db, ACC_TABLE_USER);
    _deb_usr_chk("got tid %u", tid);

    if(name){
		//logic.col = db_get_colid_byname(ac->db, tid, "name");
		logic.colname = "name";
//		_deb_usr_chk("logic.col %u", logic.col);
		//logic.comb = DB_LOGICOMB_NONE;
		logic.flags = DB_LOGICF_DEF;
		logic.logic = DB_LOGIC_LIKE;
		logic.sval = name;
    } else {
//		logic.col = db_get_colid_byname(ac->db, tid, "id");
//		_deb_usr_chk("logic.col %u", logic.col);
		logic.colname = "id";
		//logic.comb = DB_LOGICOMB_NONE;
		logic.flags = DB_LOGICF_DEF;
		logic.logic = DB_LOGIC_EQ;
		logic.ival = id;
    }

    _deb_usr_chk("collecting...");
    if((ret = db_collect(ac->db, tid, NULL, 1, &logic, NULL, DB_SO_DONT, 0, &st)) != E13_OK){
        _deb_usr_chk("fails %i", ret);
        return ret;
    }
    _deb_usr_chk("collecting done");

    switch((ret = db_step(&st))){
        case E13_CONTINUE:
        _deb_usr_chk("step CONTINUE");
        user->name = NULL;
        ret = db_column_int(&st, db_get_colid_byname(ac->db, tid, "id"), (int*)&user->uid);
        if(ret == E13_OK){
            _deb_usr_chk("id %u", user->uid);
            ret = db_column_int(&st, db_get_colid_byname(ac->db, tid, "stat"), &user->stt);
            _deb_usr_chk("stat %i (%s)", user->stt, ret == E13_OK?"OK":"NOK");
        }
        if(ret == E13_OK){
			_deb_usr_chk("checking getting passhash, colid = %u", db_get_colid_byname(ac->db, tid, "pass"));
            ret = db_column_text(&st, db_get_colid_byname(ac->db, tid, "pass"), &passlen, &user->passhash);
        }
        if(ret == E13_OK){
			_deb_usr_chk("passhash (%s)", user->passhash);
            ret = db_column_int64(&st, db_get_colid_byname(ac->db, tid, "lastlogin"), &user->lastlogin);
            _deb_usr_chk("lastlogin %llu ", user->lastlogin);
        }
        if(ret == E13_OK){
            ret = db_column_int64(&st, db_get_colid_byname(ac->db, tid, "lastlogout"), &user->lastlogout);
            _deb_usr_chk("lastlogout %llu ", user->lastlogout);
        }

        db_finalize(&st);
        ret = E13_OK;
        break;
        case E13_OK:
        _deb_usr_chk("step OK");
        db_finalize(&st);
        return e13_error(E13_NOTFOUND);
        break;
        default:
        _deb_usr_chk("step %i", ret);
        return ret;
        break;
    }

    return ret;
}

//error13_t acc_uid_chk(struct access13 *ac, uid13_t uid, struct user13* user){
//
//    struct db_stmt st;
//    struct db_logic_s logic;
//    error13_t ret;
//    db_table_id tid;
//    size_t passlen;
//
//    if(!_is_init(ac)){
//        return e13_error(E13_MISUSE);
//    }
//
//    tid = db_get_tid_byname(ac->db, ACC_TABLE_USER);
//    _deb_usr_chk("got tid %u", tid);
//
//    logic.col = db_get_colid_byname(ac->db, tid, "id");
//    _deb_usr_chk("logic.col %u", logic.col);
//    logic.comb = DB_LOGICOMB_NONE;
//    logic.flags = DB_LOGICF_COL_CMP;
//    logic.logic = DB_LOGIC_EQ;
//    logic.ival = uid;
//
//    _deb_usr_chk("collecting...");
//    if((ret = db_collect(ac->db, tid, NULL, 1, &logic,NULL,DB_SO_DONT,0,&st)) !=
//		E13_OK){
//        _deb_usr_chk("fails %i", ret);
//        return ret;
//    }
//    _deb_usr_chk("collecting done");
//
//    switch((ret = db_step(&st))){
//        case E13_CONTINUE:
//        _deb_usr_chk("step CONTINUE");
//        ret = db_column_int(&st, db_get_colid_byname(ac->db, tid, "id"),
//							(int*)&user->uid);
//        if(ret == E13_OK){
//            _deb_usr_chk("id %u", user->uid);
//            ret = db_column_int(&st, db_get_colid_byname(ac->db, tid, "stat"),
//								&user->stt);
//            _deb_usr_chk("stat %i (%s)", user->stt, ret == E13_OK?"OK":"NOK");
//        }
//        if(ret == E13_OK){
//			_deb_usr_chk("checking getting passhash, colid = %u",
//							db_get_colid_byname(ac->db, tid, "pass"));
//            ret = db_column_text(&st, db_get_colid_byname(ac->db, tid, "pass"),
//								&passlen, &user->passhash);
//        }
//        if(ret == E13_OK){
//			_deb_usr_chk("passhash (%s)", user->passhash);
//            ret = db_column_date(&st,db_get_colid_byname(ac->db,tid,"lastdate"),
//								user->lastdate);
//            _deb_usr_chk("date(j) (%i/%i/%i)", user->lastdate[0],
//						user->lastdate[1], user->lastdate[2]);
//        }
//        if(ret == E13_OK){
//            ret = db_column_int(&st, db_get_colid_byname(ac->db,tid,"lasttime"),
//								(int*)&user->lasttime);
//            _deb_usr_chk("lasttime %i ", user->lasttime);
//        }
//
//        db_finalize(&st);
//        ret = E13_OK;
//        break;
//        case E13_OK:
//        _deb_usr_chk("step OK");
//        db_finalize(&st);
//        return e13_error(E13_NOTFOUND);
//        break;
//        default:
//        _deb_usr_chk("step %i", ret);
//        return ret;
//        break;
//    }
//
//    return ret;
//}

error13_t acc_user_list(struct access13 *ac, uid13_t *n, struct user13 **user){

    struct db_stmt st;
    struct db_logic_s logic;
    error13_t ret;
    db_table_id tid;
    int stt;
    uid13_t id;
    uchar* s;
    size_t slen;
    struct user13* next, *last = NULL;
    *n = UID13_NONE;
    d13s_time_t lastlogin, lastlogout;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    tid = db_get_tid_byname(ac->db, ACC_TABLE_USER);
    _deb_usr_list("got tid %u", tid);

//    logic.col = db_get_colid_byname(ac->db, tid, "stat");
//    _deb_usr_list("logic.col %u", logic.col);
	logic.colname = "stat";
    //logic.comb = DB_LOGICOMB_NONE;
    logic.flags = DB_LOGICF_DEF;
    logic.ival = ACC_USR_STT_REMOVED;
    logic.logic = DB_LOGIC_NE;

    _deb_usr_list("collecting...");
    if((ret = db_collect(ac->db, tid, NULL,1,&logic,"name",DB_SO_INC,0,&st)) !=
		E13_OK){
        _deb_usr_list("fails@here");
        return ret;
    }
    _deb_usr_list("collecting done.");

    again:
    _deb_usr_list("step.");
    switch((ret = db_step(&st))){
        case E13_CONTINUE:
        _deb_usr_list("step CONTINUE");
        ret = db_column_int(&st, db_get_colid_byname(ac->db, tid, "id"),
							(int*)&id);
        if(ret == E13_OK){
            _deb_usr_list("id ok, %u", id);
            ret = db_column_int(&st, db_get_colid_byname(ac->db, tid, "stat"),
								&stt);
        }
        if(ret == E13_OK){
            _deb_usr_list("stat ok, %i", stt);
            ret = db_column_text(&st, db_get_colid_byname(ac->db, tid, "name"),
								&slen, &s);
        }
        if(ret == E13_OK){
            ret = db_column_int64(&st, db_get_colid_byname(ac->db, tid, "lastlogin"),
								(int64_t*)&lastlogin);
			_deb_usr_list("lastlogin: %llu", lastlogin);
        }
        if(ret == E13_OK){
            ret = db_column_int64(&st, db_get_colid_byname(ac->db, tid, "lastlogout"),
								(int64_t*)&lastlogout);
			_deb_usr_list("lastlogout: %llu", lastlogout);
        }
        if(ret == E13_OK){
            _deb_usr_list("name ok: %s", s);
            (*n)++;
            _deb_usr_list("n: %u", *n);
            next = (struct user13*)m13_malloc(sizeof(struct user13));
            next->name = (char*)s;
            next->uid = id;
            next->stt = stt;
            next->lastlogin = lastlogin;
            next->lastlogout = lastlogout;
            next->next = NULL;
            if(!last){
                last = next;
                *user = last;
                _deb_usr_list("added as first");
            } else {
                _deb_usr_list("added as last");
                last->next = next;
                last = next;
            }
        }
        if(ret != E13_OK){
            _deb_usr_list("ret = %i, break;", ret);
            break;
        }
        goto again;
        break;
        case E13_OK:
        _deb_usr_list("step OK");
        db_finalize(&st);
        return E13_OK;
        break;
        default:
        _deb_usr_list("step %i", ret);
        return ret;
        break;
    }

    _deb_usr_list("step %i", ret);
    return ret;

}

error13_t acc_user_list_free(struct user13* user){
    struct user13* first = user, *del;

    while(first){
        del = first;
        first = first->next;
        m13_free(del->name);
        m13_free(del);
    }

    return E13_OK;
}

error13_t acc_user_login(struct access13* ac, char* username, char* password,
						uid13_t* uid){

    struct db_stmt st;
    db_table_id tid;
    int stt;
    uchar** val;
    struct db_logic_s logic;
    db_colid_t colid[3];
    error13_t ret;
    struct user13 user;
    d13s_time_t stime;
    char now[20];
    uchar passhash[ac->hashlen];

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

	if(_acc_assert_user(username, UID13_INVAL) != E13_OK) return e13_error(E13_NOTVALID);

    if((ret = acc_user_chk(ac, username, UID13_NONE, &user)) != E13_OK){
        return ret;
    }

    if(user.stt == ACC_USR_STT_IN) return e13_error(E13_EXISTS);

    ac->hash(password, s13_strlen(password, ACC_MAX_PASSLEN), passhash);

    if(memcmp(passhash, user.passhash, ac->hashlen)) return e13_error(E13_AUTH);

    _deb_acc_login("login request username %s, password %s", username,password);

    tid = db_get_tid_byname(ac->db, ACC_TABLE_USER);

//    logic.col = db_get_colid_byname(ac->db, tid, "name");
	logic.colname = "name";
    //logic.comb = DB_LOGICOMB_NONE;
    logic.flags = DB_LOGICF_DEF;
    logic.sval = username;
    logic.logic = DB_LOGIC_LIKE;

    colid[0] = db_get_colid_byname(ac->db, tid, "stat");
    colid[1] = db_get_colid_byname(ac->db, tid, "lastlogin");
    //colid[2] = db_get_colid_byname(ac->db, tid, "lastlogout");

    stt = ACC_USR_STT_IN;

    val = m13_malloc(3*sizeof(char*));

    val[0] = (char*)&stt;
    d13s_clock(&stime);
    val[1]=(uchar*)&stime;
    //d13_clock(&time_);
    //val[2]=(uchar*)&time_;

    ret = db_update(ac->db, tid, logic, 2, colid, val, NULL, &st);

    db_finalize(&st);

	m13_free(val);

    return ret==E13_CONTINUE?E13_OK:ret;

}

error13_t acc_user_logout(struct access13* ac, char* username, uid13_t uid){

    struct db_stmt st;
    db_table_id tid;
    int stt;
    uchar* val[2];
    struct db_logic_s logic;
    db_colid_t colid[3];
    error13_t ret;
    struct user13 user;
    d13s_time_t stime;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

	if(_acc_assert_user(username, uid) != E13_OK) return e13_error(E13_NOTVALID);

    if((ret = acc_user_chk(ac, username, UID13_NONE, &user)) != E13_OK){
        return ret;
    }

    if(user.stt != ACC_USR_STT_IN) return e13_error(E13_NOTFOUND);

    _deb_acc_login("logout request username %s, uid %u", username, uid);

    tid = db_get_tid_byname(ac->db, ACC_TABLE_USER);

    if(username){
//        logic.col = db_get_colid_byname(ac->db, tid, "name");
		logic.colname = "name";
        //logic.comb = DB_LOGICOMB_NONE;
        logic.flags = DB_LOGICF_DEF;
        logic.sval = username;
        logic.logic = DB_LOGIC_LIKE;
    } else {
//        logic.col = db_get_colid_byname(ac->db, tid, "id");
		logic.colname = "id";
        //logic.comb = DB_LOGICOMB_NONE;
        logic.flags = DB_LOGICF_DEF;
        logic.ival = uid;
        logic.logic = DB_LOGIC_EQ;
    }

    colid[0] = db_get_colid_byname(ac->db, tid, "stat");
    colid[1] = db_get_colid_byname(ac->db, tid, "lastlogout");

    stt = ACC_USR_STT_OUT;

    //val = m13_malloc(2*sizeof(char*));

    val[0] = (char*)&stt;
    d13s_clock(&stime);
    val[1]=(uchar*)&stime;
//    val[1] = ;
//    val[2] = ;

    ret = db_update(ac->db, tid, logic, 2, colid, val, NULL, &st);

    db_finalize(&st);

	//m13_free(val);

    return ret==E13_CONTINUE?E13_OK:ret;

}
/*********************************  OBJ FN  *********************************/

//return CONTINUE if an old objid reused
static inline error13_t _acc_get_free_objid(struct access13* ac, objid13_t* id,
										uint8_t flags){

    struct db_stmt st;
    struct db_logic_s logic;
    error13_t ret;
    db_table_id tid;
    db_colid_t cid;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    if(flags & _ACC_FREEOBJIDF_ANY){
		*id = OBJID13_INVAL;
		return E13_OK;
    }

	*id = 1;

    tid = db_get_tid_byname(ac->db, ACC_TABLE_OBJ);

    _deb_get_free_objid("tid: %u", tid);

    //logic.col = db_get_colid_byname(ac->db, tid, "stat");
    logic.colname = "stat";
    //logic.comb = DB_LOGICOMB_NONE;
    logic.flags = DB_LOGICF_DEF;
    logic.logic = DB_LOGIC_EQ;
    logic.ival = ACC_OBJ_STT_REMOVED;

    if((ret = db_collect(ac->db, tid, NULL, 1, &logic, NULL, DB_SO_DONT, 0, &st)) != E13_OK){
        _deb_get_free_objid("collect fails %i", ret);
        return ret;
    }

    _deb_get_free_objid("collect rets %i", ret);

    if((cid = db_get_colid_byname(ac->db, tid, "id")) == DB_COLID_INVAL) return e13_error(E13_NOTVALID);

    _deb_get_free_objid("colid(id): %u", cid);

    switch((ret = db_step(&st))){
        case E13_CONTINUE:
        _deb_get_free_objid("step rets %i, CONTINUE", ret);
        ret = db_column_int(&st, cid, (int*)id);
        if(ret == E13_OK) ret = E13_CONTINUE;
        _deb_get_free_objid("col int ret %i, OK->CONTINUE", ret);
        break;
        case E13_OK:
        _deb_get_free_objid("step rets %i, OK", ret);
        break;
        default:
        _deb_get_free_objid("step rets %i, fin", ret);
        db_finalize(&st);
        return ret;
        break;
    }

    if(ret == E13_CONTINUE) return ret;

    db_finalize(&st);

    _deb_get_free_objid("collecting max id");

    //find max id
    if((ret= db_collect(ac->db, tid, NULL, 0, NULL, "id", DB_SO_DEC, 1, &st))!=
		E13_OK){
        _deb_get_free_objid("collecting max id fails %i", ret);
        return ret;
    }

    _deb_get_free_objid("collecting max id done");

    switch((ret = db_step(&st))){
        case E13_CONTINUE:
        _deb_get_free_objid("step CONTINUE");
        ret = db_column_int(&st, cid, (int*)id);
        (*id)++;
        if(*id >= OBJID13_MAX) ret = e13_error(E13_FULL);
        _deb_get_free_objid("id %u, fin", *id);
        db_finalize(&st);
        break;
        case E13_OK:
        _deb_get_free_objid("step OK, fin");
        db_finalize(&st);
        break;
        default:
        _deb_get_free_objid("ret %i, fin", ret);
        db_finalize(&st);
        return ret;
        break;
    }

    return ret;
}

error13_t acc_obj_set_stat(	struct access13* ac,
							char* name, objid13_t objid,
							int stt){

    struct db_stmt st;
    db_table_id tid;
    int stat[1];
    struct db_logic_s logic;
    db_colid_t colid;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    if(_acc_assert_obj(name, objid) != E13_OK) return e13_error(E13_NOTVALID);

    tid = db_get_tid_byname(ac->db, ACC_TABLE_OBJ);

    if(name){
		logic.colname = "name";
		logic.flags = DB_LOGICF_DEF;
		logic.sval = name;
		logic.logic = DB_LOGIC_LIKE;
    } else {
		logic.colname = "id";
		logic.flags = DB_LOGICF_DEF;
		logic.ival = objid;
		logic.logic = DB_LOGIC_EQ;
    }

    if((colid = db_get_colid_byname(ac->db, tid, "stat")) == DB_COLID_INVAL)
		return e13_error(E13_NOTVALID);

    stat[0] = stt;

    return db_update(ac->db, tid, logic, 1, &colid, (uchar**)&stat, NULL,&st);

}

error13_t acc_obj_set_parent(	struct access13* ac,
								char* name, objid13_t objid,
								char* parent, objid13_t pid){

    struct db_stmt st;
    db_table_id tid;     
    struct db_logic_s logic;
    db_colid_t colid;    
    int64_t val[1];    
	struct obj13 pobj;
	error13_t ret;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    if(_acc_assert_obj(name, objid) != E13_OK) return e13_error(E13_NOTVALID);

    tid = db_get_tid_byname(ac->db, ACC_TABLE_OBJ);

    if(name){
		logic.colname = "name";
		logic.flags = DB_LOGICF_DEF;
		logic.sval = name;
		logic.logic = DB_LOGIC_LIKE;
    } else {
		logic.colname = "id";
		logic.flags = DB_LOGICF_DEF;
		logic.ival = objid;
		logic.logic = DB_LOGIC_EQ;
    }
    
    if(parent){    	
    	if((ret = acc_obj_chk(ac, parent, OBJID13_NONE, &pobj)) != E13_OK){    	
			return ret;    				
		}
		_deb_set_parent("set (parent=%s) pid to %llu", parent, pobj.objid);
	} else {		
		pobj.objid = pid;		
		_deb_set_parent("set pid to %llu", pobj.objid);
	}

    if((colid = db_get_colid_byname(ac->db, tid, "pid")) == DB_COLID_INVAL)
		return e13_error(E13_NOTVALID);

    val[0] = pobj.objid;    
    
    _deb_set_parent("pid=%llu, val=%llu",pobj.objid, val[0]);

    ret = db_update(ac->db, tid, logic,
					1, &colid,
					(uchar**)&val, NULL,
					&st);
	
	db_finalize(&st);
	
	return ret;	
}

error13_t acc_obj_add(struct access13 *ac, char *name, char* parent){

    struct db_stmt st;
    db_table_id tid;
    db_colid_t colids[3];
    uchar* cols[ACC_TABLE_OBJ_COLS];
    size_t size[ACC_TABLE_OBJ_COLS];
    objid13_t objid;
    error13_t ret;
    int stat;
    struct db_logic_s logic;
    struct obj13 obj, pobj;
    uint8_t flags = _ACC_FREEOBJIDF_INIT;
	uid13_t uid = UID13_NONE;
	gid13_t gid = GID13_NONE;
	acc_perm_t perm = 0;	
    

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }
    
	if(_acc_assert_obj(name, OBJID13_INVAL) != E13_OK)
		return e13_error(E13_NOTVALID);
	_deb_obj_add("passed assert obj name");

	if(parent && _acc_assert_obj(parent, OBJID13_INVAL) != E13_OK)
		return e13_error(E13_NOTVALID);	
	_deb_obj_add("passed assert obj parent");

    if(strcmp(name, ACC_OBJ_ALL)){
		if(_acc_assert_obj(name, OBJID13_NONE) != E13_OK)
			return e13_error(E13_NOTVALID);
    } else {
    	flags |= _ACC_FREEOBJIDF_ANY;
    }

    tid = db_get_tid_byname(ac->db, ACC_TABLE_OBJ);
    _deb_obj_add("tid: %u", tid);
    if(tid == DB_TID_INVAL) return e13_error(E13_CORRUPT);

    //1. check for existence
    if(acc_obj_chk(ac, name, OBJID13_NONE, &obj) == E13_OK){
        _deb_obj_add("obj exists");
        
        //if exists and stat is ACC_OBJ_STT_REMOVED replace it        		
        
        if(obj.stt != ACC_OBJ_STT_REMOVED) return e13_error(E13_EXISTS);        
        
    }
    
    //1a. check for parent
    if(parent){	
	    if(acc_obj_chk(ac, parent, OBJID13_NONE, &pobj) != E13_OK){
	        _deb_obj_add("parent obj not exists");		
			return e13_error(E13_NOTFOUND);
	        
	    }
	    if(pobj.stt == ACC_OBJ_STT_REMOVED){
	    	_deb_obj_add("parent obj removed already");
	    	return e13_error(E13_NOTFOUND);
		}		
	} else {
		pobj.objid = OBJID13_NONE;
	}
	
	_deb_obj_add("parent objid = %llu", pobj.objid);

    //2. add it

//    cols = (char**)m13_malloc(sizeof(char*)*ACC_TABLE_OBJ_COLS);
//    if(!cols) return e13_error(E13_NOMEM);

    switch((ret = _acc_get_free_objid(ac, &objid, flags))){
    case E13_CONTINUE://an old objid is re-used, update
        _deb_obj_add("reusing old id, id = %u", objid);
        stat = ACC_OBJ_STT_ACTIVE;
        //logic.col = db_get_colid_byname(ac->db, tid, "id");
        logic.colname = "id";
//        _deb_obj_add("logic.colid(id): %u", logic.col);
//        logic.comb = DB_LOGICOMB_NONE;
        logic.ival = objid;
        logic.flags = DB_LOGICF_DEF;
        logic.logic = DB_LOGIC_EQ;
        cols[0] = (uchar*)name;
        cols[1] = (uchar*)&stat;
        cols[2] = (uchar*)&pobj.objid;
        if((colids[0] = db_get_colid_byname(ac->db, tid, "name")) == DB_COLID_INVAL) return e13_error(E13_NOTVALID);
        if((colids[1] = db_get_colid_byname(ac->db, tid, "stat")) == DB_COLID_INVAL) return e13_error(E13_NOTVALID);
        if((colids[2] = db_get_colid_byname(ac->db, tid, "pid")) == DB_COLID_INVAL) return e13_error(E13_NOTVALID);
        _deb_obj_add("colids[0](name): %u", colids[0]);
        _deb_obj_add("colids[1](stat): %u", colids[1]);
        _deb_obj_add("colids[2](pid): %llu", colids[2]);
        ret = db_update(ac->db, tid, logic, 3, colids, cols, NULL, &st);
        _deb_obj_add("db_update ret: %i", ret);
        db_finalize(&st);
        _deb_obj_add("re-used old id");
        return ret;
        break;
    case E13_OK://new id, insert
        _deb_obj_add("need new id");
        break;

    default:
        _deb_obj_add("ret: %i", ret);
        return ret;
        break;

    }

    stat = ACC_OBJ_STT_ACTIVE;
    cols[0] = (uchar*)&ac->regid;
    cols[1] = (uchar*)&objid;
    cols[2] = (uchar*)name;
    cols[3] = (uchar*)&pobj.objid;
    cols[4] = (uchar*)&stat;
    cols[5] = (uchar*)&uid;
    cols[6] = (uchar*)&gid;
    cols[7] = (uchar*)&perm;
    size[0] = sizeof(regid_t);
    size[1] = sizeof(objid13_t);
    size[2] = strlen(name) + 1;
    size[3] = sizeof(objid13_t);
    size[4] = sizeof(int);
    size[5] = sizeof(uid13_t);
    size[6] = sizeof(gid13_t);
    size[7] = sizeof(acc_perm_t);
//    size[4] = sizeof(int);
    if((ret = db_insert(ac->db, tid, cols, size, &st)) != E13_OK){
        db_finalize(&st);
//        m13_free(cols);
        _deb_obj_add("fails here");
        return ret;
    }

    _deb_obj_add("insert ok, fin;");

    db_finalize(&st);
//    m13_free(cols);
    _deb_obj_add("done");

    return E13_OK;

}

static inline error13_t _acc_rm_objid_entries(struct access13* ac,
											objid13_t nobjid,
											objid13_t objid[],
											db_table_id ntid,
											db_table_id tid[]){
    struct db_stmt st;
    struct db_logic_s logic;
    error13_t ret;
    objid13_t iobjid;
    db_table_id itid;

//	not needed
//    if(!_is_init(ac)){
//        return e13_error(E13_MISUSE);
//    }

	for(itid = 0; itid < ntid; itid++){

		//logic.col = db_get_colid_byname(ac->db, tid[itid], "objid");
		logic.colname = "objid";

		for(iobjid = 0; iobjid < nobjid; iobjid++){

			//logic.comb = DB_LOGICOMB_NONE;
			logic.flags = DB_LOGICF_DEF;
			logic.logic = DB_LOGIC_EQ;
			logic.ival = objid[iobjid];

			_deb_obj_rm("collecting...");
			if((ret = db_delete(ac->db, tid[itid], 1, &logic, &st)) != E13_OK){
				_deb_obj_rm("fails %i", ret);
				return ret;
			}
			_deb_obj_rm("collecting done");

			switch((ret = db_step(&st))){
				case E13_CONTINUE:
				case E13_OK:
				break;
				default:
				//TODO: what to do on error?
				_deb_obj_rm("step %i", ret);
				return ret;
				break;
			}

            db_reset(&st);

		}//iobjid
	}//itid

    return ret;
}

static inline error13_t _acc_rm_obj_parent(struct access13* ac, objid13_t pid){
	
	db_table_id tid;
	struct db_logic_s logic;
	db_colid_t colids[1];
	uchar* cols[1];
	objid13_t newpid = OBJID13_NONE;
	error13_t ret;
	struct db_stmt st;

    tid = db_get_tid_byname(ac->db, ACC_TABLE_OBJ);
    if(tid == DB_TID_INVAL) return e13_error(E13_CORRUPT);
    
	logic.colname = "pid";
	logic.flags = DB_LOGICF_DEF;
	logic.logic = DB_LOGIC_EQ;
	logic.ival = pid;
	
	cols[0] = (uchar*)&newpid;	
	if((colids[0] = db_get_colid_byname(ac->db, tid, "pid")) == DB_COLID_INVAL) return e13_error(E13_NOTVALID);	
		
	ret = db_update(ac->db, tid, logic, 1, colids, cols, NULL, &st);
	
    _deb_obj_rm("db_update ret: %i", ret);
    db_finalize(&st);
    return ret;
	
}

error13_t acc_obj_rm(struct access13 *ac, char *name, objid13_t objid){

    struct db_stmt st;
    db_table_id tid, tids[2];
    error13_t ret;
    int stt;
    struct db_logic_s logic;
    db_colid_t colids[1];
    uchar* cols[1];
    struct obj13 obj;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    if(_acc_assert_obj(name, OBJID13_INVAL) != E13_OK)
		return e13_error(E13_NOTVALID);

    tid = db_get_tid_byname(ac->db, ACC_TABLE_OBJ);
    if(tid == DB_TID_INVAL) return e13_error(E13_CORRUPT);

    //1. check for existence
	if(acc_obj_chk(ac, name, objid, &obj) != E13_OK){
		return e13_error(E13_NOTFOUND);
	}

	if(obj.stt == ACC_OBJ_STT_REMOVED) return e13_error(E13_OK);

    //tids[0] = db_get_tid_byname(ac->db, ACC_TABLE_MEMBERSHIP);
    tids[0] = db_get_tid_byname(ac->db, ACC_TABLE_ACL);

	//remove acl entries
    if((ret = _acc_rm_objid_entries(ac, 1, &obj.objid, 1, tids)) != E13_OK){
        return ret;
    }
    
    if((ret = _acc_rm_obj_parent(ac, obj.objid)) != E13_OK){
        return ret;
    }    

	stt = ACC_OBJ_STT_REMOVED;
    //logic.col = db_get_colid_byname(ac->db, tid, "id");
    logic.colname = "id";
    //_deb_obj_rm("logic.colid(id): %u", logic.col);
    //logic.comb = DB_LOGICOMB_NONE;
    logic.ival = obj.objid;
    _deb_obj_rm("objid: %u", obj.objid);
    logic.flags = DB_LOGICF_DEF;
    logic.logic = DB_LOGIC_EQ;
    cols[0] = (uchar*)&stt;
    if((colids[0] = db_get_colid_byname(ac->db, tid, "stat")) == DB_COLID_INVAL) return e13_error(E13_NOTVALID);
    ret = db_update(ac->db, tid, logic, 1, colids, cols, NULL, &st);
    _deb_obj_rm("db_update ret: %i", ret);
    db_finalize(&st);
    return ret;

}

/*
error13_t acc_obj_rm(struct access13 *ac, char *name, objid13_t objid){

    struct db_stmt st;
    db_table_id tid, tids[2];
    error13_t ret;    
    struct db_logic_s logic;
    struct obj13 obj;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    if(_acc_assert(name, objid, 0) != E13_OK) return e13_error(E13_NOTVALID);

    tid = db_get_tid_byname(ac->db, ACC_TABLE_OBJ);
    if(tid == DB_TID_INVAL) return e13_error(E13_CORRUPT);

    //1. check for existence
	if(acc_obj_chk(ac, name, objid, &obj) != E13_OK){
		return e13_error(E13_NOTFOUND);
	}

	if(obj.stt == ACC_OBJ_STT_REMOVED) return e13_error(E13_OK);

    tids[0] = db_get_tid_byname(ac->db, ACC_TABLE_MEMBERSHIP);
    tids[1] = db_get_tid_byname(ac->db, ACC_TABLE_ACL);

    if((ret = _acc_rm_objid_entries(ac, 1, &obj.objid, 2, tids)) != E13_OK){
        return ret;
    }

    //logic.col = db_get_colid_byname(ac->db, tid, "id");
    logic.colname = "id";
    //_deb_obj_rm("logic.colid(id): %u", logic.col);
    //logic.comb = DB_LOGICOMB_NONE;
    logic.ival = obj.objid;
    _deb_obj_rm("objid: %u", obj.objid);
    logic.flags = DB_LOGICF_DEF;
    logic.logic = DB_LOGIC_EQ;
    
	db_delete(ac->db, tid, 1, &logic, &st);    

    db_finalize(&st);
    
    return ret;

}
*/

error13_t acc_obj_chk(struct access13 *ac, char *name, objid13_t objid,
						struct obj13* obj){

    struct db_stmt st;
    struct db_logic_s logic;
    error13_t ret;
    db_table_id tid;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    tid = db_get_tid_byname(ac->db, ACC_TABLE_OBJ);
    _deb_obj_chk("got tid %u", tid);
    if(tid == DB_TID_INVAL) return e13_error(E13_CORRUPT);

    if(name){
		//logic.col = db_get_colid_byname(ac->db, tid, "name");
		logic.colname = "name";
//		_deb_obj_chk("logic.col %u", logic.col);
		//logic.comb = DB_LOGICOMB_NONE;
		logic.flags = DB_LOGICF_DEF;
		logic.logic = DB_LOGIC_LIKE;
		logic.sval = name;
    } else {
		//logic.col = db_get_colid_byname(ac->db, tid, "id");
		logic.colname = "id";
//		_deb_obj_chk("logic.col %u", logic.col);
		//logic.comb = DB_LOGICOMB_NONE;
		logic.flags = DB_LOGICF_DEF;
		logic.logic = DB_LOGIC_EQ;
		logic.ival = objid;
    }

    _deb_obj_chk("collecting...");
    if((ret = db_collect(ac->db, tid, NULL, 1, &logic, NULL, DB_SO_DONT, 0, &st)) != E13_OK){
        _deb_obj_chk("fails %i", ret);
        return ret;
    }
    _deb_obj_chk("collecting done");

    switch((ret = db_step(&st))){
        case E13_CONTINUE:
        _deb_obj_chk("step CONTINUE");
        ret = db_column_int64(&st, db_get_colid_byname(ac->db, tid, "id"), (int64_t*)&obj->objid);
        ret = db_column_int64(&st, db_get_colid_byname(ac->db, tid, "pid"), (int64_t*)&obj->parent);
        ret = db_column_text(&st, db_get_colid_byname(ac->db, tid, "name"), &obj->namelen, (uchar**)&obj->name);
        if(ret == E13_OK){
            _deb_obj_chk("id %llu", obj->objid);
            ret = db_column_int(&st, db_get_colid_byname(ac->db, tid, "stat"), &obj->stt);
            _deb_obj_chk("stat %i (%s)", obj->stt, ret == E13_OK?"OK":"NOK");
        }
        ret = db_column_int(&st, db_get_colid_byname(ac->db, tid, "uid"), (int32_t*)&obj->uid);
        ret = db_column_int(&st, db_get_colid_byname(ac->db, tid, "gid"), (int32_t*)&obj->gid);
        ret = db_column_int(&st, db_get_colid_byname(ac->db, tid, "perm"), (int32_t*)&obj->perm);
		
        db_finalize(&st);
        break;
        case E13_OK:
        _deb_obj_chk("step OK");
        db_finalize(&st);
        return e13_error(E13_NOTFOUND);
        break;
        default:
        _deb_obj_chk("step %i", ret);
        return ret;
        break;
    }

    return ret;
}

error13_t acc_obj_list(struct access13 *ac, objid13_t *n,
						struct obj13 **obj){

    struct db_stmt st;
    struct db_logic_s logic;
    error13_t ret;
    db_table_id tid;
    int stt;
    objid13_t id, pid;
    uchar* s;
    size_t slen;
    struct obj13* next, *last = NULL;
    *n = OBJID13_NONE;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    tid = db_get_tid_byname(ac->db, ACC_TABLE_OBJ);
    _deb_obj_list("got tid %u", tid);

//    if((logic.col = db_get_colid_byname(ac->db, tid, "stat")) ==DB_COLID_INVAL){
//		return e13_error(E13_ABORT);
//    }
	logic.colname = "stat";
    //logic.comb = DB_LOGICOMB_NONE;
    logic.flags = DB_LOGICF_DEF;
    logic.ival = ACC_OBJ_STT_REMOVED;
    logic.logic = DB_LOGIC_NE;

    _deb_obj_list("collecting...");
    if((ret = db_collect(ac->db, tid, NULL, 1, &logic, "name", DB_SO_INC, 0, &st)) != E13_OK){
        _deb_obj_list("fails@here");
        return ret;
    }
    _deb_obj_list("collecting done.");

    again:
    _deb_obj_list("step.");
    switch((ret = db_step(&st))){
        case E13_CONTINUE:
        _deb_obj_list("step CONTINUE");
        ret = db_column_int64(&st, db_get_colid_byname(ac->db, tid, "id"), (int64_t*)&id);		
        _deb_obj_list("id , %llu", id);    
        ret = db_column_text(&st, db_get_colid_byname(ac->db, tid, "name"), &slen, &s);
		_deb_obj_list("name , %s", (char*)s);
        ret = db_column_int64(&st, db_get_colid_byname(ac->db, tid, "pid"), (int64_t*)&pid);
        _deb_obj_list("pid , %llu", pid);
        ret = db_column_int(&st, db_get_colid_byname(ac->db, tid, "stat"), &stt);
        _deb_obj_list("stat , %i", stt);
        
        if(ret == E13_OK){
            _deb_obj_list("name ok: %s", s);
            (*n)++;
            _deb_obj_list("n: %u", *n);
            next = (struct obj13*)m13_malloc(sizeof(struct obj13));
            next->name = (char*)s;
            next->objid = id;
            next->parent = pid;
            next->stt = stt;
            next->next = NULL;
            if(!last){
                last = next;
                *obj = next;
                _deb_obj_list("added as first");
            } else {
                _deb_obj_list("added as last");
                last->next = next;
                last = next;
            }
        }
        if(ret != E13_OK){
            _deb_obj_list("ret = %i, break;", ret);
            break;
        }
        goto again;
        break;
        case E13_OK:
        _deb_obj_list("step OK");
        db_finalize(&st);
        return E13_OK;
        break;
        default:
        _deb_obj_list("step %i", ret);
        return ret;
        break;
    }

    _deb_obj_list("step %i", ret);
    return ret;

}

error13_t acc_obj_list_free(struct obj13* obj){
    struct obj13* first = obj, *del;

    while(first){
        del = first;
        first = first->next;
        m13_free(del->name);
        m13_free(del);
    }

    return E13_OK;
}

error13_t acc_obj_set_owner(	struct access13* ac,
				char* obj, objid13_t objid,
				char* owner, uid13_t uid,
				char* group, gid13_t gid){

    struct db_stmt st;
    db_table_id tid;     
    struct db_logic_s logic;
    db_colid_t colid[2];    
    int32_t val[2];    
	struct obj13 pobj;
	error13_t ret;
	struct user13 usr;
	struct group13 grp;
	db_colid_t ncol;

	if(!_is_init(ac)){
		return e13_error(E13_MISUSE);
	}

	if(_acc_assert_user(owner, uid) != E13_OK) return e13_error(E13_NOTVALID);
	if(_acc_assert_group(group, gid) != E13_OK) return e13_error(E13_NOTVALID);
	if(_acc_assert_obj(obj, objid) != E13_OK) return e13_error(E13_NOTVALID);

	tid = db_get_tid_byname(ac->db, ACC_TABLE_OBJ);

	if(obj){
		logic.colname = "name";
		logic.flags = DB_LOGICF_DEF;
		logic.sval = obj;
		logic.logic = DB_LOGIC_LIKE;
	} else {
		logic.colname = "id";
		logic.flags = DB_LOGICF_DEF;
		logic.ival = objid;
		logic.logic = DB_LOGIC_EQ;
	}
    
	if(owner){    	
		if((ret = acc_user_chk(ac, owner, UID13_NONE, &usr)) != E13_OK){    	
			return ret;    				
		}
		_deb_set_owner("set (owner=%s) uid to %llu", owner, usr.uid);
	} else {		
		usr.uid = uid;	
		_deb_set_owner("set uid to %llu", usr.uid);
	}
	
	if(group){    	
		if((ret = acc_group_chk(ac, group, GID13_NONE, &grp)) != E13_OK){    	
			return ret;    				
		}
		_deb_set_owner("set (group=%s) gid to %llu", group, grp.gid );
	} else {		
		grp.gid = gid;	
		_deb_set_owner("set gid to %llu", grp.gid );
	}
		
	if(usr.uid == UID13_INVAL && grp.gid == GID13_INVAL){
		return e13_error(E13_NOTVALID);
	} else if(usr.uid == UID13_INVAL) {
		
		if((colid[0] = db_get_colid_byname(ac->db, tid, "gid")) == DB_COLID_INVAL)
			return e13_error(E13_NOTVALID);
		ncol = 1;
		val[0] = grp.gid;
		
	} else if(grp.gid == GID13_INVAL) {
		
		if((colid[0] = db_get_colid_byname(ac->db, tid, "uid")) == DB_COLID_INVAL)
			return e13_error(E13_NOTVALID);
		ncol = 1;
		val[0] = usr.uid;
		
	} else {
		
	    if((colid[0] = db_get_colid_byname(ac->db, tid, "uid")) == DB_COLID_INVAL)
			return e13_error(E13_NOTVALID);

	    if((colid[1] = db_get_colid_byname(ac->db, tid, "gid")) == DB_COLID_INVAL)
		return e13_error(E13_NOTVALID);		
		
	    ncol = 2;
		
	    val[0] = usr.uid;
	    val[1] = grp.gid;
	}
    
	//_deb_set_parent("uid=%llu, val=%llu",usr.uid, val[0]);

	ret = db_update(ac->db, tid, logic,
			ncol, colid,
			(uchar**)&val, NULL,
			&st);
	
	db_finalize(&st);
	
	return ret;	
}

error13_t acc_obj_set_perm(	struct access13* ac,								
				char* obj, objid13_t objid,
				acc_perm_t perm){

    struct db_stmt st;
    db_table_id tid;     
    struct db_logic_s logic;
    db_colid_t colid;    
    int32_t val[1];    
	struct obj13 pobj;
	error13_t ret;	

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }
	
    if(_acc_assert_obj(obj, objid) != E13_OK) return e13_error(E13_NOTVALID);

    tid = db_get_tid_byname(ac->db, ACC_TABLE_OBJ);

    if(obj){
		logic.colname = "name";
		logic.flags = DB_LOGICF_DEF;
		logic.sval = obj;
		logic.logic = DB_LOGIC_LIKE;
    } else {
		logic.colname = "id";
		logic.flags = DB_LOGICF_DEF;
		logic.ival = objid;
		logic.logic = DB_LOGIC_EQ;
    }
    
    if((colid = db_get_colid_byname(ac->db, tid, "perm")) == DB_COLID_INVAL)
		return e13_error(E13_NOTVALID);

    val[0] = perm;        

    ret = db_update(ac->db, tid, logic,
					1, &colid,
					(uchar**)&val, NULL,
					&st);
	
	db_finalize(&st);
	
	return ret;	
}

error13_t acc_destroy(struct access13* ac){
	error13_t ret;
	//TODO
	if(!_is_init(ac)) return e13_error(E13_MISUSE);
	ret = db_close(ac->db);
	ret = db_destroy(ac->db);
	m13_free(ac->db);
	return ret;
}

error13_t acc_user_group_list(	struct access13 *ac,
								char *username, uid13_t uid,
								gid13_t* ngrp, struct group13** grouplist,
								int resolve_gid){

    struct db_stmt st;
    struct db_logic_s logic;
    error13_t ret;
    db_table_id tid;
    struct user13 usr;
    struct group13* grp, *gl_last, gtmp;
    size_t passlen;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    if((ret = acc_user_chk(ac, username, uid, &usr)) != E13_OK){
		return ret;
    }

    tid = db_get_tid_byname(ac->db, ACC_TABLE_MEMBERSHIP);
    _deb_usr_chk("got tid %u", tid);

//    logic[0].col = db_get_colid_byname(ac->db, tid, "uid");
	logic.colname = "uid";
    //logic[0].comb = DB_LOGICOMB_NONE;
    logic.flags = DB_LOGICF_DEF;
    logic.logic = DB_LOGIC_EQ;
    logic.ival = usr.uid;

    _deb_usr_chk("collecting...");
    if((ret = db_collect(ac->db, tid, NULL, 1, &logic, NULL, DB_SO_DONT, 0, &st)) != E13_OK){
        _deb_usr_chk("fails %i", ret);
        return ret;
    }
    _deb_usr_chk("collecting done");

    *grouplist = NULL;
    *ngrp = 0UL;

    while((ret = db_step(&st)) == E13_CONTINUE){
        grp = (struct group13*)m13_malloc(sizeof(struct group13));
        grp->next = NULL;

        if(db_column_int(&st, db_get_colid_byname(ac->db, tid, "gid"), &grp->gid) != E13_OK){
            grp->gid = GID13_INVAL;
        }

        if(resolve_gid){
            if(acc_group_chk(ac, NULL, grp->gid, &gtmp) == E13_OK){
                grp->name = s13_malloc_strcpy(gtmp.name, 0);
            }
        } else {
            grp->name = NULL;
        }

        if(!(*grouplist)) {
                *grouplist = grp;
                gl_last = grp;
        } else {
            gl_last->next = grp;
            gl_last = grp;
        }
		(*ngrp)++;
    }

    switch(ret){
        case E13_OK:
        _deb_usr_chk("step OK");
        db_finalize(&st);
        return E13_OK;
        break;
        default:
        _deb_usr_chk("step %i", ret);
        return ret;
        break;
    }

    return ret;
}

error13_t acc_group_user_list(	struct access13 *ac,
								char *groupname, gid13_t gid,
								uid13_t* nusr, struct user13** userlist,
								int resolve_uid){

    struct db_stmt st;
    struct db_logic_s logic;
    error13_t ret;
    db_table_id tid;
    struct group13 grp;
    struct user13* usr, *ul_last, utmp;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    if((ret = acc_group_chk(ac, groupname, gid, &grp)) != E13_OK){
		return ret;
    }

    tid = db_get_tid_byname(ac->db, ACC_TABLE_MEMBERSHIP);
    _deb_usr_chk("got tid %u", tid);

//    logic[0].col = db_get_colid_byname(ac->db, tid, "gid");
	logic.colname = "gid";
    //logic[0].comb = DB_LOGICOMB_NONE;
    logic.flags = DB_LOGICF_DEF;
    logic.logic = DB_LOGIC_EQ;
    logic.ival = grp.gid;

    _deb_usr_chk("collecting...");
    if((ret = db_collect(ac->db, tid, NULL, 1, &logic, NULL,DB_SO_DONT,0,&st)) !=
		E13_OK){
        _deb_usr_chk("fails %i", ret);
        return ret;
    }
    _deb_usr_chk("collecting done");

    *userlist = NULL;
    *nusr = 0UL;
    while((ret = db_step(&st)) == E13_CONTINUE){
        usr = (struct user13*)m13_malloc(sizeof(struct user13));
        usr->next = NULL;

        if(db_column_int(&st,db_get_colid_byname(ac->db,tid,"uid"),&usr->uid) !=
			E13_OK){
            usr->uid = UID13_INVAL;
        }

        if(resolve_uid){
            if(acc_user_chk(ac, NULL, usr->uid, &utmp) == E13_OK){
                usr->name = s13_malloc_strcpy(utmp.name, 0);
            }
        } else {
            usr->name = NULL;
        }

        if(!(*userlist)) {
                *userlist = usr;
                ul_last = usr;
        } else {
            ul_last->next = usr;
            ul_last = usr;
        }
        (*nusr)++;
    }

    switch(ret){
        case E13_OK:
        _deb_usr_chk("step OK");
        db_finalize(&st);
        return E13_OK;
        break;
        default:
        _deb_usr_chk("step %i", ret);
        return ret;
        break;
    }

    return ret;
}

error13_t acc_user_group_chk(struct access13 *ac, char *username, uid13_t uid,
							char* group, gid13_t gid){

    struct db_stmt st;
    struct db_logic_s logic[3];
    error13_t ret;
    db_table_id tid;
    struct user13 usr;
    struct group13 grp;
    size_t passlen;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    if((ret = acc_user_chk(ac, username, uid, &usr)) != E13_OK){
		return ret;
    }
    if((ret = acc_group_chk(ac, group, gid, &grp)) != E13_OK){
		return ret;
    }

    tid = db_get_tid_byname(ac->db, ACC_TABLE_MEMBERSHIP);
    _deb_usr_chk("got tid %u", tid);

//    logic[0].col = db_get_colid_byname(ac->db, tid, "uid");
	logic[0].colname = "uid";
    //logic[0].comb = DB_LOGICOMB_AND;
    logic[0].flags = DB_LOGICF_DEF;
    logic[0].logic = DB_LOGIC_EQ;
    logic[0].ival = usr.uid;

    logic[1].logic = DB_LOGIC_AND;

//    logic[2].col = db_get_colid_byname(ac->db, tid, "gid");
	logic[2].colname = "gid";
    //logic[2].comb = DB_LOGICOMB_NONE;
    logic[2].flags = DB_LOGICF_DEF;
    logic[2].logic = DB_LOGIC_EQ;
    logic[2].ival = grp.gid;

    _deb_usr_chk("collecting...");
    if((ret = db_collect(ac->db, tid, NULL, 2, logic,NULL,DB_SO_DONT,0,&st)) !=
		E13_OK){
        _deb_usr_chk("fails %i", ret);
        return ret;
    }
    _deb_usr_chk("collecting done");

    switch((ret = db_step(&st))){
        case E13_CONTINUE:
        db_finalize(&st);
        ret = E13_OK;
        break;
        case E13_OK:
        _deb_usr_chk("step OK");
        db_finalize(&st);
        return e13_error(E13_NOTFOUND);
        break;
        default:
        _deb_usr_chk("step %i", ret);
        return ret;
        break;
    }

    return ret;
}

error13_t acc_user_join_group(struct access13* ac, char* username, uid13_t uid,
							char* group, gid13_t gid){

    struct db_stmt st;
    struct db_logic_s logic[2];
    error13_t ret;
    db_table_id tid;
    struct user13 usr;
    struct group13 grp;
    size_t passlen;
    char stt;
    uchar* cols[6];
    size_t size[6];

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    if(acc_user_group_chk(ac, username, uid, group, gid) == E13_OK){
		return e13_error(E13_EXISTS);
    }

    if((ret = acc_user_chk(ac, username, uid, &usr)) != E13_OK){
		return ret;
    }
    if((ret = acc_group_chk(ac, group, gid, &grp)) != E13_OK){
		return ret;
    }

    tid = db_get_tid_byname(ac->db, ACC_TABLE_MEMBERSHIP);
    if(tid == DB_TID_INVAL) return e13_error(E13_CORRUPT);
    _deb_usr_chk("got tid %u", tid);

/*	COLUMNS
                                "RegID",
                                DB_T_INT,
                                "شماره ثبت",
                                "",
                                DB_COLF_HIDE|DB_COLF_AUTO,

                                "nrow",
                                DB_T_BIGINT,
                                "ردیف",
                                "",
                                DB_COLF_HIDE|DB_COLF_AUTO,

                                "gid",
                                DB_T_INT,
                                "گروه",
                                "@group:id>name",
                                DB_COLF_LIST|DB_COLF_TRANSL,

                                "uid",
                                DB_T_INT,
                                "کاربر",
                                "@user:id>name",
                                DB_COLF_LIST|DB_COLF_TRANSL,

                                "stat",
                                DB_T_INT,
                                "وضعیت",
                                "",
                                0,

                                "flags",
                                DB_T_INT,
                                "پرچم",
                                "",
                                DB_COLF_VIRTUAL|DB_COLF_HIDE

*/

    stt = ACC_MEMS_STT_ACTIVE;
    cols[0] = (uchar*)&ac->regid;
	cols[1] = NULL;/*(uchar*)&nrow;*/
    cols[2] = (uchar*)&grp.gid;
    cols[3] = (uchar*)&usr.uid;
    cols[4] = (uchar*)&stt;
    cols[5] = NULL;/*(uchar*)&flags;*/
    size[0] = sizeof(regid_t);
//    size[1] = sizeof(int);
    size[2] = sizeof(gid13_t);
    size[3] = sizeof(uid13_t);
    size[4] = sizeof(int);
//    size[5] = sizeof(int);
    if((ret = db_insert(ac->db, tid, cols, size, &st)) != E13_OK){
        db_finalize(&st);
//        m13_free(cols);
        return ret;
    }

    db_finalize(&st);
//    m13_free(cols);
    return E13_OK;
}

error13_t acc_user_leave_group(struct access13* ac, char* username, uid13_t uid,
							char* group, gid13_t gid){

    struct db_stmt st;
    struct db_logic_s logic[3];
    error13_t ret;
    db_table_id tid;
    struct user13 usr;
    struct group13 grp;
    size_t passlen;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    if((ret = acc_user_chk(ac, username, uid, &usr)) != E13_OK){
		return ret;
    }
    if((ret = acc_group_chk(ac, group, gid, &grp)) != E13_OK){
		return ret;
    }

    tid = db_get_tid_byname(ac->db, ACC_TABLE_MEMBERSHIP);
    _deb_usr_chk("got tid %u", tid);

//    logic[0].col = db_get_colid_byname(ac->db, tid, "uid");
	logic[0].colname = "uid";
    //logic[0].comb = DB_LOGICOMB_AND;
    logic[0].flags = DB_LOGICF_DEF;
    logic[0].logic = DB_LOGIC_EQ;
    logic[0].ival = usr.uid;

    logic[1].logic = DB_LOGIC_AND;

//    logic[1].col = db_get_colid_byname(ac->db, tid, "gid");
	logic[2].colname = "gid";
    //logic[2].comb = DB_LOGICOMB_NONE;
    logic[2].flags = DB_LOGICF_DEF;
    logic[2].logic = DB_LOGIC_EQ;
    logic[2].ival = grp.gid;

    _deb_usr_chk("collecting...");
    if((ret = db_delete(ac->db, tid, 2, logic, &st)) != E13_OK){
        _deb_usr_chk("fails %i", ret);
        return ret;
    }
    _deb_usr_chk("collecting done");

    switch((ret = db_step(&st))){
        case E13_CONTINUE:
        db_finalize(&st);
        ret = E13_OK;
        break;
        case E13_OK:
        _deb_usr_chk("step OK");
        db_finalize(&st);
        return e13_error(E13_NOTFOUND);
        break;
        default:
        _deb_usr_chk("step %i", ret);
        return ret;
        break;
    }

    return ret;

}

error13_t _acc_perm_chk(struct access13* ac, objid13_t objid, aclid13_t aclid,
						acc_perm_t* perm, int _idtype){

	//type 1 user, else group

    struct db_stmt st;
    struct db_logic_s logic[3];
    error13_t ret;
    db_table_id tid;
    struct user13 usr;
    struct group13 grp;
    size_t passlen;
    int dbperm;

    tid = db_get_tid_byname(ac->db, ACC_TABLE_ACL);
    _deb_usr_chk("got tid %u", tid);

//    logic[0].col = db_get_colid_byname(ac->db, tid, "objid");
//    logic[0].comb = DB_LOGICOMB_AND;
	logic[0].colname = "objid";
    logic[0].flags = DB_LOGICF_DEF;
    logic[0].logic = DB_LOGIC_EQ;
    logic[0].ival = objid;

    logic[1].logic = DB_LOGIC_AND;

//	logic[1].col = db_get_colid_byname(ac->db, tid, _idtype==1?"uid":"gid");
	logic[2].colname = _idtype==1?"uid":"gid";
//    logic[1].comb = DB_LOGICOMB_NONE;
    logic[2].flags = DB_LOGICF_DEF;
    logic[2].logic = DB_LOGIC_EQ;
    logic[2].ival = aclid;

    _deb_usr_chk("collecting...");
    if((ret = db_collect(ac->db, tid, NULL, 2, logic,NULL,DB_SO_DONT,0,&st)) !=
		E13_OK){
        _deb_usr_chk("fails %i", ret);
        return ret;
    }
    _deb_usr_chk("collecting done");

    switch((ret = db_step(&st))){
        case E13_CONTINUE:
        ret = db_column_int(&st, db_get_colid_byname(ac->db,tid,"perm"), (int*)perm);
        db_finalize(&st);
        break;
        case E13_OK:
        _deb_usr_chk("step OK");
        db_finalize(&st);
        return e13_error(E13_NOTFOUND);
        break;
        default:
        _deb_usr_chk("step %i", ret);
        return ret;
        break;
    }

    return ret;

}

error13_t acc_acl_list_free(struct acc_acl_entry* acllist){
    struct acc_acl_entry* first = acllist, *del;

    while(first){
        del = first;
        first = first->next;
        m13_free(del);
    }

    return E13_OK;
}

//TODO: MAKE THE LIST THING WORK!
error13_t _acc_load_acl(struct access13* ac,
						objid13_t objid,
						uid13_t nuid, uid13_t uid[],
						gid13_t ngid, gid13_t gid[],
						uid13_t* nacl,
						struct acc_acl_entry** acllist){

	struct acc_acl_entry* first = NULL, *last, *entry;
	struct db_stmt st;
    struct db_logic_s logic[nuid+ngid+1];
    error13_t ret;
    db_table_id tid;
    db_colid_t colid_perm, colid_uid, colid_gid, colid_objid;
    acc_perm_t perm;
    uid13_t iuid;
    gid13_t igid;
    int ilogic;

    tid = db_get_tid_byname(ac->db, ACC_TABLE_ACL);
    _deb_usr_chk("got tid %u", tid);

    //logic[0].col = db_get_colid_byname(ac->db, tid, "objid");
    logic[0].colname = "objid";
//    logic[0].comb = DB_LOGICOMB_AND;
    logic[0].flags = DB_LOGICF_DEF;
    logic[0].logic = DB_LOGIC_EQ;
    logic[0].ival = objid;

    ilogic = 1;
    perm = 0;//use as a flag here

    while(nuid){

		if(!perm){
			logic[ilogic++].logic = DB_LOGIC_AND;
			logic[ilogic++].logic = DB_LOGIC_OPENP;
			perm = 1;
		} else {
			logic[ilogic++].logic = DB_LOGIC_OR;
		}

		logic[ilogic].colname = "uid";
		logic[ilogic].logic = DB_LOGIC_EQ;
		logic[ilogic++].ival = uid[--nuid];

        if(!nuid){
			logic[ilogic++].logic = DB_LOGIC_CLOSEP;
			perm = 0;
        }

    }

    while(ngid){

		if(!perm){
			logic[ilogic++].logic = DB_LOGIC_AND;
			logic[ilogic++].logic = DB_LOGIC_OPENP;
			perm = 1;
		} else {
			logic[ilogic++].logic = DB_LOGIC_OR;
		}

		logic[ilogic].colname = "gid";
		logic[ilogic].logic = DB_LOGIC_EQ;
		logic[ilogic++].ival = gid[--ngid];

        if(!ngid){
			logic[ilogic++].logic = DB_LOGIC_CLOSEP;
        }

    }

   _deb_usr_chk("collecting...");
    if((ret = db_collect(ac->db, tid, NULL, nuid+ngid+1, logic,NULL,DB_SO_DONT,0,&st))!=
		E13_OK){
        _deb_usr_chk("fails %i", ret);
        return ret;
    }
    _deb_usr_chk("collecting done");

    colid_perm = db_get_colid_byname(ac->db,tid,"perm");
    colid_uid = db_get_colid_byname(ac->db,tid,"uid");
    colid_gid = db_get_colid_byname(ac->db,tid,"gid");
    colid_objid = db_get_colid_byname(ac->db,tid,"objid");

    *nacl = 0UL;
loop:
    switch((ret = db_step(&st))){
        case E13_CONTINUE:
		(*nacl)++;
        if((ret = db_column_int(&st, colid_perm, (int*)&perm)) != E13_OK) break;
        if((ret = db_column_int(&st, colid_uid, (int*)&iuid)) != E13_OK) break;
        if((ret = db_column_int(&st, colid_gid, (int*)&igid)) != E13_OK) break;
        if((ret = db_column_int64(&st, colid_objid, (int64_t*)&objid)) != E13_OK)
			break;
        if(ret = E13_OK){
				if(!(entry =(struct acc_acl_entry*)m13_malloc(sizeof(struct acc_acl_entry)))) break;
				entry->perm = perm;
				entry->uid = iuid;
				entry->gid = igid;
				entry->objid = objid;
				entry->next = NULL;
			if(!first){
				first = entry;
                last = first;
                *acllist = first;
			} else {
				last->next = entry;
				last = entry;
			}
        }
		goto loop;
        break;
        case E13_OK:
        _deb_usr_chk("step OK");
        db_finalize(&st);
        return e13_error(E13_NOTFOUND);
        break;
        default:
        _deb_usr_chk("step %i", ret);
        return ret;
        break;
    }

    return ret;
}

error13_t acc_pack_gid_list(struct group13* grouplist, gid13_t ngid, gid13_t gidarray[]){
	gid13_t i;
	struct group13* grp;
	grp = grouplist;
	for(i = 0; i < ngid; i++){
		gidarray[i] = grp->gid;
        grp = grp->next;
	}
	return E13_OK;
}

//TODO: NEEDS LOTS OF WORK AND TESTING!
error13_t acc_perm_user_chk(struct access13* ac,
							objid13_t objid,
							char* username, uid13_t uid,
							acc_perm_t* perm){

	error13_t ret;
	struct user13 usr;

    if((ret = acc_user_chk(ac, username, uid, &usr)) != E13_OK){
		return ret;
    }

	if((ret = _acc_perm_chk(ac, objid, uid, perm, 1)) != E13_OK){
		return ret;
	}

	return ret;
}

error13_t acc_perm_user_add(struct access13* ac,
							objid13_t objid,
							char* name, uid13_t uid,
							acc_perm_t perm){

	error13_t ret;
	struct user13 usr;
	uid13_t nacl;
	struct acc_acl_entry* acllist;

	//db vars
    struct db_stmt st;
    struct db_logic_s iflogic;
    db_table_id tid;
    uchar* val[ACC_TABLE_ACL_COLS];
    size_t size[ACC_TABLE_ACL_COLS];
    db_colid_t colid;
    acc_perm_t perm_ptr;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

	/*
		1. check to see if user exists
		2. check to see if there is an entry already exists
		3. add or update if exists
	*/

    if((ret = acc_user_chk(ac, name, uid, &usr)) != E13_OK){
		return ret;
    }

    if((ret = _acc_load_acl(ac, objid, 1, &usr.uid, 0, NULL, &nacl, &acllist))!=
		E13_OK){
		return ret;
    }

    tid = db_get_tid_byname(ac->db, ACC_TABLE_ACL);
    if(nacl){//there is an entry
		if(perm == acllist->perm) {
				return E13_OK;
		} else {//update current perm

			iflogic.colname = "uid";
			iflogic.ival = usr.uid;
			iflogic.logic = DB_LOGIC_EQ;
			iflogic.flags = DB_LOGICF_DEF;

			colid = db_get_colid_byname(ac->db, tid, "perm");

			perm_ptr = perm;
			val[0] = &perm_ptr;

			ret = db_update(ac->db, tid, iflogic, 1, &colid, val, NULL, &st);
			db_finalize(&st);
		}
    } else {//add new entry

/*
                                "RegID",
                                DB_T_INT,
                                "شماره ثبت",
                                "",
                                DB_COLF_HIDE|DB_COLF_AUTO,

                                "nrow",
                                DB_T_BIGINT,
                                "ردیف",
                                "",
                                DB_COLF_HIDE|DB_COLF_AUTO,

                                "gid",
                                DB_T_INT,
                                "گروه",
                                "@group:id>name",
                                DB_COLF_LIST|DB_COLF_TRANSL,

                                "uid",
                                DB_T_INT,
                                "کاربر",
                                "@user:id>name",
                                DB_COLF_LIST|DB_COLF_TRANSL,

                                "objid",
                                DB_T_BIGINT,
                                "objid",
                                "",
                                0,

                                "perm",
                                DB_T_INT,
                                "دسترسی",
                                "",
                                0,

                                "flags",
                                DB_T_INT,
                                "پرچم",
                                "",
                                DB_COLF_VIRTUAL|DB_COLF_HIDE

*/

		val[0] = (uchar*)&ac->regid;//regid
		val[1] = NULL;/*(uchar*)&nrow;*/
		val[2] = NULL;//gid
		val[3] = (uchar*)&usr.uid;//uid
		val[4] = (uchar*)&objid;//objid
		val[5] = NULL;/*(uchar*)&flags;*/
		size[0] = sizeof(regid_t);
	//    size[1] = sizeof(int);
//		size[2] = sizeof(gid13_t);
		size[3] = sizeof(uid13_t);
		size[4] = sizeof(objid13_t);
	//    size[5] = sizeof(int);

        ret = db_insert(ac->db, tid, val, size, &st);
        db_finalize(&st);

    }

    return ret;
}

error13_t acc_perm_user_rm(	struct access13* ac,
							objid13_t objid,
							char* name, uid13_t uid){

	error13_t ret;
	struct user13 usr;

	//db vars
    struct db_stmt st;
    struct db_logic_s logic;
    db_table_id tid;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

	/*
		1. check to see if user exists
	*/

    if((ret = acc_user_chk(ac, name, uid, &usr)) != E13_OK){
		return ret;
    }

    tid = db_get_tid_byname(ac->db, ACC_TABLE_ACL);

    logic.colname = "uid";
    logic.flags = DB_LOGICF_DEF;
    logic.ival = usr.uid;
    logic.logic = DB_LOGIC_EQ;

    ret = db_delete(ac->db, tid, 1, &logic, &st);

    db_finalize(&st);

    return ret;
}

error13_t acc_perm_user_list(	struct access13* ac,
								char* name, uid13_t uid,
								struct acc_acl_entry** acllist, int resolve_id){
	error13_t ret;
	struct user13 usr;

	//db vars
    struct db_stmt st;
    struct db_logic_s logic;
	objid13_t objid;
	acc_perm_t perm;
	struct acc_acl_entry* aclentry, *last;
	db_table_id tid;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

	/*
		1. check to see if user exists
	*/

    if((ret = acc_user_chk(ac, name, uid, &usr)) != E13_OK){
		return ret;
    }

    tid = db_get_tid_byname(ac->db, ACC_TABLE_ACL);

    logic.colname = "uid";
    logic.flags = DB_LOGICF_DEF;
    logic.ival = usr.uid;
    logic.logic = DB_LOGIC_EQ;

    ret = db_collect(ac->db, tid, NULL, 1, &logic, NULL, DB_SO_DONT, -1, &st);
    if(ret != E13_OK){
		db_finalize(&st);
		return ret;
    }

    *acllist = NULL;
loop:
    switch((ret = db_step(&st))){
	case E13_CONTINUE:
		if(db_column_int64(&st, db_get_colid_byname(ac->db, tid, "objid"), (int64_t*)&objid) != E13_OK){
			//TODO
		}
		if(db_column_int(&st, db_get_colid_byname(ac->db, tid, "perm"), (int*)&perm) != E13_OK){
			//TODO
		}
		aclentry = (struct acc_acl_entry*)m13_malloc(sizeof(struct acc_acl_entry));
		aclentry->objid = objid;
		aclentry->perm = perm;
		aclentry->next = NULL;
        if(!(*acllist)){
            *acllist = aclentry;
            last = aclentry;
        } else {
			last->next = aclentry;
			last = last->next;
        }
        goto loop;
		break;
	case E13_OK:
	case E13_DONE:
		db_finalize(&st);
		return E13_OK;
		break;
	default:
		db_finalize(&st);
		return ret;
		break;

    }
}

error13_t acc_perm_group_chk(struct access13* ac,
							objid13_t objid,
							char* groupname, gid13_t gid,
							acc_perm_t* perm){

	error13_t ret;
	struct group13 grp;

    if((ret = acc_group_chk(ac, groupname, gid, &grp)) != E13_OK){
		return ret;
    }

	if((ret = _acc_perm_chk(ac, objid, gid, perm, 0)) != E13_OK){
		return ret;
	}

	return ret;
}

error13_t acc_perm_group_add(	struct access13* ac,
								objid13_t objid,
								char* name, gid13_t gid,
								acc_perm_t perm){

	error13_t ret;
	struct group13 grp;
	gid13_t nacl;
	struct acc_acl_entry* acllist;

	//db vars
    struct db_stmt st;
    struct db_logic_s iflogic;
    db_table_id tid;
    uchar* val[ACC_TABLE_ACL_COLS];
    size_t size[ACC_TABLE_ACL_COLS];
    db_colid_t colid;
    acc_perm_t perm_ptr;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

	/*
		1. check to see if user exists
		2. check to see if there is an entry already exists
		3. add or update if exists
	*/

    if((ret = acc_group_chk(ac, name, gid, &grp)) != E13_OK){
		return ret;
    }

    if((ret = _acc_load_acl(ac, objid, 0, NULL, 1, &grp.gid, &nacl, &acllist))!=
		E13_OK){
		return ret;
    }

    tid = db_get_tid_byname(ac->db, ACC_TABLE_ACL);
    if(nacl){//there is an entry
		if(perm == acllist->perm) {
				return E13_OK;
		} else {//update current perm

			iflogic.colname = "gid";
			iflogic.ival = grp.gid;
			iflogic.logic = DB_LOGIC_EQ;
			iflogic.flags = DB_LOGICF_DEF;

			colid = db_get_colid_byname(ac->db, tid, "perm");

			perm_ptr = perm;
			val[0] = &perm_ptr;

			ret = db_update(ac->db, tid, iflogic, 1, &colid, val, NULL, &st);
			db_finalize(&st);
		}
    } else {//add new entry

/*
                                "RegID",
                                DB_T_INT,
                                "شماره ثبت",
                                "",
                                DB_COLF_HIDE|DB_COLF_AUTO,

                                "nrow",
                                DB_T_BIGINT,
                                "ردیف",
                                "",
                                DB_COLF_HIDE|DB_COLF_AUTO,

                                "gid",
                                DB_T_INT,
                                "گروه",
                                "@group:id>name",
                                DB_COLF_LIST|DB_COLF_TRANSL,

                                "gid",
                                DB_T_INT,
                                "کاربر",
                                "@user:id>name",
                                DB_COLF_LIST|DB_COLF_TRANSL,

                                "objid",
                                DB_T_BIGINT,
                                "objid",
                                "",
                                0,

                                "perm",
                                DB_T_INT,
                                "دسترسی",
                                "",
                                0,

                                "flags",
                                DB_T_INT,
                                "پرچم",
                                "",
                                DB_COLF_VIRTUAL|DB_COLF_HIDE

*/

		val[0] = (uchar*)&ac->regid;//regid
		val[1] = NULL;/*(uchar*)&nrow;*/
		val[2] = (uchar*)&grp.gid;//gid
		val[3] = NULL;//uid
		val[4] = (uchar*)&objid;//objid
		val[5] = NULL;/*(uchar*)&flags;*/
		size[0] = sizeof(regid_t);
	//    size[1] = sizeof(int);
		size[2] = sizeof(gid13_t);
//		size[3] = sizeof(uid13_t);
		size[4] = sizeof(objid13_t);
	//    size[5] = sizeof(int);

        ret = db_insert(ac->db, tid, val, size, &st);
        db_finalize(&st);

    }

    return ret;
}

error13_t acc_perm_group_rm(	struct access13* ac,
							objid13_t objid,
							char* name, gid13_t gid){

	error13_t ret;
	struct group13 grp;

	//db vars
    struct db_stmt st;
    struct db_logic_s logic;
    db_table_id tid;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

	/*
		1. check to see if group exists
	*/

    if((ret = acc_group_chk(ac, name, gid, &grp)) != E13_OK){
		return ret;
    }

    tid = db_get_tid_byname(ac->db, ACC_TABLE_ACL);

    logic.colname = "gid";
    logic.flags = DB_LOGICF_DEF;
    logic.ival = grp.gid;
    logic.logic = DB_LOGIC_EQ;

    ret = db_delete(ac->db, tid, 1, &logic, &st);

    db_finalize(&st);

    return ret;
}

error13_t acc_perm_group_list(	struct access13* ac,
								char* name, gid13_t gid,
								struct acc_acl_entry** acllist, int resolve_id){
	error13_t ret;
	struct group13 grp;

	//db vars
    struct db_stmt st;
    struct db_logic_s logic;
	objid13_t objid;
	acc_perm_t perm;
	struct acc_acl_entry* aclentry, *last;
	db_table_id tid;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

	/*
		1. check to see if group exists
	*/

    if((ret = acc_group_chk(ac, name, gid, &grp)) != E13_OK){
		return ret;
    }

    tid = db_get_tid_byname(ac->db, ACC_TABLE_ACL);

    logic.colname = "gid";
    logic.flags = DB_LOGICF_DEF;
    logic.ival = grp.gid;
    logic.logic = DB_LOGIC_EQ;

    ret = db_collect(ac->db, tid, NULL, 1, &logic, NULL, DB_SO_DONT, -1, &st);
    if(ret != E13_OK){
		db_finalize(&st);
		return ret;
    }

    *acllist = NULL;
loop:
    switch((ret = db_step(&st))){
	case E13_CONTINUE:
		if(db_column_int64(&st, db_get_colid_byname(ac->db, tid, "objid"), (int64_t*)&objid) != E13_OK){
			//TODO
		}
		if(db_column_int(&st, db_get_colid_byname(ac->db, tid, "perm"), (int*)&perm) != E13_OK){
			//TODO
		}
		aclentry = (struct acc_acl_entry*)m13_malloc(sizeof(struct acc_acl_entry));
		aclentry->objid = objid;
		aclentry->perm = perm;
		aclentry->next = NULL;
		if(!(*acllist)){
			*acllist = aclentry;
			last = aclentry;
		} else {
			last->next = aclentry;
			last = last->next;
		}
        goto loop;
		break;
	case E13_OK:
	case E13_DONE:
		db_finalize(&st);
		return E13_OK;
		break;
	default:
		db_finalize(&st);
		return ret;
		break;

    }
}

error13_t acc_perm_obj_list(struct access13* ac, objid13_t objid, struct acc_acl_entry** acllist, int resolve_id){
	error13_t ret;

	//db vars
    struct db_stmt st;
    struct db_logic_s logic;
	uid13_t uid;
	gid13_t gid;
	acc_perm_t perm;
	struct acc_acl_entry* aclentry, *last;
	db_table_id tid;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

	/*
		1. check to see if group exists
	*/

    tid = db_get_tid_byname(ac->db, ACC_TABLE_ACL);

    logic.colname = "objid";
    logic.flags = DB_LOGICF_DEF;
    logic.ival = objid;
    logic.logic = DB_LOGIC_EQ;

    ret = db_collect(ac->db, tid, NULL, 1, &logic, NULL, DB_SO_DONT, -1, &st);
    if(ret != E13_OK){
		db_finalize(&st);
		return ret;
    }

    *acllist = NULL;
loop:
    switch((ret = db_step(&st))){
	case E13_CONTINUE:
		if(db_column_int(&st, db_get_colid_byname(ac->db, tid, "uid"), (int*)&uid) != E13_OK){
			//TODO
		}
		if(db_column_int(&st, db_get_colid_byname(ac->db, tid, "gid"), (int*)&gid) != E13_OK){
			//TODO
		}
		if(db_column_int(&st, db_get_colid_byname(ac->db, tid, "perm"), (int*)&perm) != E13_OK){
			//TODO
		}
		aclentry = (struct acc_acl_entry*)m13_malloc(sizeof(struct acc_acl_entry));
		aclentry->uid = uid;
		aclentry->gid = gid;
		aclentry->perm = perm;
		aclentry->next = NULL;
		if(!(*acllist)){
			*acllist = aclentry;
			last = aclentry;
		} else {
			last->next = aclentry;
			last = last->next;
		}
        goto loop;
		break;
	case E13_OK:
	case E13_DONE:
		db_finalize(&st);
		return E13_OK;
		break;
	default:
		db_finalize(&st);
		return ret;
		break;

    }
}

error13_t acc_user_access(	struct access13* ac,
							objid13_t objid,
							char* username, uid13_t uid,
							acc_perm_t perm){

    struct db_logic_s logic[2];
    error13_t ret;
    struct user13 usr;
    struct group13* grouplist;
    gid13_t ngrp;
    uid13_t nacl;
    struct acc_acl_entry* acllist, *aclentry;
    gid13_t* gidarray;

    if(!_is_init(ac)){
        return e13_error(E13_MISUSE);
    }

    if((ret = acc_user_chk(ac, username, uid, &usr)) != E13_OK){
		return ret;
    }

    //0. last all user groups
    //1. load all acl entries to objid
    //2. the rules are
    //	a. if the user access is denied, fail
	//	b. if the user access is permitted, success
	//	c. if one of the groups in the user membership list is allowed, success
	//	d. fail
	//3. special users/groups: allusers/allgroups

	//0.
	_deb_pchk("getting user(uid=%lu) groups", usr.uid);
	acc_user_group_list(ac, NULL, usr.uid, &ngrp, &grouplist, 0);
	_deb_pchk("got %lu entries", ngrp);

	if(ngrp){
		gidarray = m13_malloc(sizeof(gid13_t)*ngrp);

		acc_pack_gid_list(grouplist, ngrp, gidarray);
		_deb_pchk("gid list packed", ngrp);
	} else {
		gidarray = NULL;
	}

	//1.
	if(_acc_load_acl(	ac, objid,
					1, &usr.uid,
					ngrp, gidarray,
					&nacl,
					&acllist) != E13_OK){
		//TODO
	}

	if(gidarray) m13_free(gidarray);

	_deb_pchk("got %lu acl entries", nacl);

    //1.a. & 1.b.
	for(aclentry = acllist; aclentry; aclentry = aclentry->next){
        if(aclentry->perm & perm){
			_deb_pchk("uid=%lu, gid=%lu, perm OK", aclentry->uid, aclentry->gid);
			ret = E13_OK;
        }
        if(aclentry->uid == usr.uid){
				_deb_pchk("got uid entry %lu", aclentry->uid);
				break;
        }
	}

	if(aclentry){
		_deb_pchk("has acl entry");
		if(perm & aclentry->perm){
				_deb_pchk("OK");
				ret = E13_OK;
		} else {
			_deb_pchk("NOK");
			ret = e13_error(E13_PERM);
		}
	} else {
		ret = e13_error(E13_PERM);
	}

	if(nacl) acc_acl_list_free(acllist);

	//1.c.
	return ret;
}
