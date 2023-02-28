#ifndef OBJ13_H
#define OBJ13_H

#include <assert.h>

#include "error13.h"
#include "type13.h"
#include "const13.h"
#include "thread13.h"

typedef uint16_t obj13_flag_t;
typedef uint64_t objid13_t;
typedef uint8_t obj13_link_t;

#define OBJID13_ZERO    (0ULL)
#define OBJID13_NONE	OBJID13_ZERO
#define OBJID13_INVAL   ((objid13_t)-1)
//#define OBJID13_ANY		(0xffffffffffffffff)
#define OBJID13_MAX     (OBJID13_INVAL-1)
#define OBJID13_FIRST   ((objid13_t)1)

#define OBJ13_LINK_CONTAIN  (0x01<<0)//parent(x)=>child(x);child(x)=>parent(ok)
#define OBJ13_LINK_PROPERTY (0x01<<1)//parent(x)=>child(ok);child(x)=>parent(ok)
#define OBJ13_LINK_DEPENDED (0x01<<2)//parent(x)=>child(x);child(x)=>parent(x)
#define OBJ13_LINK_BY       (0x01<<7)/* this is the last bit!
                                        enables diff between contain by,
                                        contain to, friend by, friend to, etc.*/

#define OBJ13_LINK_DEF      OBJ13_LINK_EMPTY

struct obj13_link {
    objid13_t dst;
    obj13_link_t link_t;
    struct obj13_link* left, *right;
};

// cut from db13.h for compat
enum db_type_id{
    DB_TY_BOOL,
    DB_TY_INT,
    DB_TY_BIGINT,
    DB_TY_REAL,
    DB_TY_TEXT,
    DB_TY_DATE,//deprecated
    DB_TY_D13STIME,//day13 serialize-able time
    DB_TY_RAW,
    DB_TY_INVAL
};

struct obj13_region_field {
    enum db_type_id type;
    datalen13_t len;
    char* name;
};

typedef uint32_t obj13_regionid_t;
typedef uint8_t obj13_region_lock_t;
typedef uint8_t obj13_region_flag_t;
typedef uint8_t obj13_ctl_flag_t;
typedef uint32_t obj13_region_ref_t;
#define OBJ13_REGION_LOCK_WR    (0x01<<0)
#define OBJ13_REGION_LOCK_RD    (0x01<<1)
#define OBJ13_REGION_LOCK_RDWR  (OBJ13_REGION_LOCK_RD|OBJ13_REGION_LOCK_WR)

#define OBJ13_REGION_FLAG_NOREF (0x01<<0)
#define OBJ13_REGION_FLAG_NOREFRD (0x01<<1)
#define OBJ13_REGION_FLAG_NOREFWR (0x01<<2)
#define OBJ13_REGION_FLAG_NOREFRDWR (OBJ13_REGION_FLAG_NOREFWR|OBJ13_REGION_FLAG_NOREFRD)

struct obj13_region {
    char* name;
    obj13_region_flag_t flags;
    //obj13_region_lock_t lock;
    obj13_region_ref_t ref_rd, ref_wr;
    objid13_t start, end;
    uint32_t nfields;
    struct obj13_region_field* field;
    struct obj13_region* next;
};

struct obj13_ctl {
    obj13_ctl_flag_t flags;
    th13_mutex_t mx;
    th13_cond_t cond;
    obj13_regionid_t nregions;
    struct obj13_region* region_first, *region_last;
};

#define OBJ13_FLAG_WR       (0x0001<<0)//writing!
#define OBJ13_FLAG_NOREQ    (0x0001<<1)//no more requests are accepted
#define OBJ13_FLAG_FREEPTR  (0x0001<<2)//free objptr at deletion
#define OBJ13_FLAG_VIRTUAL  (0x0001<<3)//virtual object, like object invoice
#define OBJ13_FLAG_RHEAD    (0x0001<<4)//object region header, e.g. users' regi
#define OBJ13_FLAG_MTHREAD  (0x0001<<5)//on root only, multi threaded
#define OBJ13_FLAG_DEF      (0x0000)

//ac PERMISSIONS
#define ACC_PERM_ZERO	0x0000
#define ACC_PERM_ONE	0x0001
#define ACC_PERM_N		11
#define ACC_PERM_OWN_R 	(ACC_PERM_ONE<<0)
#define ACC_PERM_OWN_W 	(ACC_PERM_ONE<<1)
#define ACC_PERM_OWN_X 	(ACC_PERM_ONE<<2)
#define ACC_PERM_GRP_R  (ACC_PERM_ONE<<3)
#define ACC_PERM_GRP_W  (ACC_PERM_ONE<<4)
#define ACC_PERM_GRP_X  (ACC_PERM_ONE<<5)
#define ACC_PERM_OTH_R  (ACC_PERM_ONE<<6)
#define ACC_PERM_OTH_W  (ACC_PERM_ONE<<7)
#define ACC_PERM_OTH_X  (ACC_PERM_ONE<<8)
#define ACC_PERM_OWN_S  (ACC_PERM_ONE<<9)
#define ACC_PERM_OWN_T  (ACC_PERM_ONE<<10)

//this header controls object!
struct obj13 {
    obj13_flag_t flags;
    objid13_t objid, parent;
    uint16_t ref;//refrence counter
    size_t namelen;
    char* name;

    objid13_t nlinks;
    struct obj13_link* link;
    
    int stt;//for db reasons!
    
    //classic permissions
    char* owner;
    uid13_t uid;
    char* group;
    gid13_t gid;	
	acc_perm_t perm;        

    void* objptr;
    struct obj13* left, *right, *next;//may work as tree or list
};

#define obj13_use(obj)      MACRO( (obj)->ref++; )
#define obj13_unuse(obj)    MACRO( assert((obj)->ref); (obj)->ref--; )

#define obj13_init(obj) MACRO( (obj)->flags = OBJ13_FLAG_DEF; (obj)->objid = OBJID13_INVAL; (obj)->ref = 0; )

#ifdef __cplusplus
    extern "C" {
#endif

    //ctl regions must be, see region-type-field and region structs
    error13_t obj13_ctl_init(struct obj13_ctl* ctl);
    error13_t obj13_ctl_region_init(struct obj13_region* reg);
    error13_t obj13_ctl_region_lock(struct obj13_ctl* ctl,
                                    objid13_t objid,
                                    obj13_region_lock_t lock);
    error13_t obj13_ctl_region_unlock(struct obj13_ctl* ctl,
                                      objid13_t objid,
                                      obj13_region_lock_t lock);
    error13_t obj13_ctl_region_setflag(struct obj13_ctl* ctl,
                                       objid13_t objid,
                                       obj13_region_flag_t flags);
    error13_t obj13_ctl_region_init(struct obj13_region* reg);
    error13_t obj13_ctl_region_field_init(struct obj13_region_field* field);

    error13_t obj13_bst_create_node(struct obj13 **node,
                                    objid13_t objid,
                                    void *objptr,
                                    obj13_flag_t flags);
    error13_t obj13_bst_insert_node(struct obj13 *root, struct obj13 *node);
    error13_t obj13_bst_find_node(struct obj13 *root, objid13_t objid, struct obj13 **node);
    void obj13_bst_free_node(struct obj13* node);
    error13_t obj13_bst_delete_node(struct obj13 *root, objid13_t objid);
    void obj13_bst_inorder_traversal(struct obj13 *root, void (*cb)(struct obj13*));

#define obj13_bst_free_objptr(node) ((node)->flags & OBJ13_FLAG_FREEPTR?m13_free((node)->objptr):E13_OK)

#ifdef __cplusplus
    }
#endif

//error13_t obj13_init(struct obj13* obj);

#endif // OBJ13_H
