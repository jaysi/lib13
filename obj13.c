#include "include/obj13.h"
#include "include/mem13.h"

#include <assert.h>
#include <stdlib.h>

error13_t obj13_ctl_init(struct obj13_ctl* ctl) {

    th13_cond_init(&ctl->cond, NULL);
    th13_mutex_init(&ctl->mx, NULL);

    ctl->flags = 0;
    ctl->region_first = NULL;
    ctl->nregions = 0UL;

    return E13_OK;

}

error13_t obj13_ctl_region_init(struct obj13_region* reg){

    reg->name = NULL;
    reg->flags = 0;
    reg->ref_rd = 0;
    reg->ref_wr = 0;
    reg->start = 0;
    reg->end = 0;
    reg->nfields = 0;
    reg->field = NULL;
    reg->next = NULL;

    return E13_OK;

}

error13_t obj13_ctl_region_field_init(struct obj13_region_field* field){

    field->len = 0;
    field->name = NULL;
    field->type = DB_TY_INVAL;

    return E13_OK;

}

//always lock mx before calling this in mthread mode
error13_t obj13_ctl_find_type_array(struct obj13_ctl* ctl, objid13_t objid, struct obj13_region** reg){

    for(*reg = ctl->region_first; *reg; *reg = (*reg)->next) {
        if( (*reg)->start >= objid &&
            (*reg)->end <= objid
          ){

            return E13_OK;

        }
    }

    return e13_error(E13_NOTFOUND);
}

error13_t obj13_ctl_region_setflag(struct obj13_ctl* ctl,
                                   objid13_t objid,
                                   obj13_region_flag_t flags){

    struct obj13_region* reg;
    error13_t ret;

    th13_mutex_lock(&ctl->mx);

    if((ret = obj13_ctl_find_type_array(ctl, objid, &reg)) != E13_OK){
        th13_mutex_unlock(&ctl->mx);
        return ret;
    }

    reg->flags = flags;

    th13_mutex_unlock(&ctl->mx);

    return E13_OK;

}

error13_t obj13_ctl_region_lock(struct obj13_ctl* ctl,
                                objid13_t objid,
                                obj13_region_lock_t lock){

    error13_t ret;
    struct obj13_region* reg;

    th13_mutex_lock(&ctl->mx);

    if((ret = obj13_ctl_find_type_array(ctl, objid, &reg)) != E13_OK){
        th13_mutex_unlock(&ctl->mx);
        return ret;
    }

    if(lock & OBJ13_REGION_LOCK_RD){
        if(reg->flags & OBJ13_REGION_FLAG_NOREFRD){
            th13_mutex_unlock(&ctl->mx);
            return e13_error(E13_PERM);
        }
    }

    if(lock & OBJ13_REGION_LOCK_WR){
        if(reg->flags & OBJ13_REGION_FLAG_NOREFWR){
            th13_mutex_unlock(&ctl->mx);
            return e13_error(E13_PERM);
        }
    }

    if(lock & OBJ13_REGION_LOCK_WR){

        while(reg->ref_rd && reg->ref_wr){
            th13_cond_wait(&ctl->cond, &ctl->mx);
        }

        reg->ref_wr++;

    } else if(lock & OBJ13_REGION_LOCK_RD){

        while(reg->ref_wr){
            th13_cond_wait(&ctl->cond, &ctl->mx);
        }

        reg->ref_rd++;

    }

    th13_mutex_unlock(&ctl->mx);

    return E13_OK;

}

error13_t obj13_ctl_region_unlock(struct obj13_ctl* ctl,
                                  objid13_t objid,
                                  obj13_region_lock_t lock){

    error13_t ret;
    struct obj13_region* reg;
    int sig;

    th13_mutex_lock(&ctl->mx);

    if((ret = obj13_ctl_find_type_array(ctl, objid, &reg)) != E13_OK){
        th13_mutex_unlock(&ctl->mx);
        return ret;
    }

    sig = 0;

    if(lock & OBJ13_REGION_LOCK_WR){

        if(reg->ref_wr) {reg->ref_wr--; sig+=1;}

    } else if(lock & OBJ13_REGION_LOCK_RD){

        if(reg->ref_rd) {reg->ref_rd--; sig+=2;}

    }

    if(sig) th13_cond_signal(&ctl->cond);

    th13_mutex_unlock(&ctl->mx);

    return E13_OK;

}

error13_t obj13_bst_create_node(struct obj13 **node,
                                objid13_t objid,
                                void *objptr,
                                obj13_flag_t flags){

    *node = (struct obj13*)m13_malloc(sizeof(struct obj13));
    if(!(*node)) return e13_error(E13_NOMEM);

    (*node)->objid = objid;
    (*node)->objptr = objptr;
    (*node)->flags = flags;

    (*node)->right = NULL;
    (*node)->left = NULL;

    //(*node)->ctl = NULL;

    return E13_OK;

}

void obj13_bst_free_node(struct obj13* node){
	//nothing!
}

error13_t obj13_bst_insert_node(struct obj13 *root, struct obj13 *node){

    assert(root);
    assert(node);

    if(node->objid < root->objid)
    {
        if(root->left == NULL)
            root->left = node;
        else
            obj13_bst_insert_node(root->left, node);
    }
    else
    {
        if(root->right == NULL)
            root->right = node;
        else
            obj13_bst_insert_node(root->right, node);
    }

    return E13_OK;
}

error13_t obj13_bst_find_node(struct obj13 *root, objid13_t objid, struct obj13 **node){
    *node = root;
    while(*node){
        if((*node)->objid == objid){
            return E13_OK;
        } else if(objid < (*node)->objid){
            *node = (*node)->left;
        } else {
            *node = (*node)->right;
        }
    }
    return e13_error(E13_NOTFOUND);
}

error13_t obj13_bst_delete_node(struct obj13 *root, objid13_t objid){

    struct obj13* temp, *parent;

    if (!root)
        return e13_error(E13_NOTFOUND);   // item not in BST

    if (objid < root->objid)
            obj13_bst_delete_node(root->left, objid);
    else if (objid > root->objid)
            obj13_bst_delete_node(root->right, objid);
    else
    {
        if (!root->left)
        {
            temp = root->right;
            obj13_bst_free_node(root);
            m13_free(root);
            root = temp;
        }
        else if (!root->right)
        {
                temp = root->left;
                obj13_bst_free_node(root);
                m13_free(root);
                root = temp;
        }
        else    //2 children
        {
            temp = root->right;
            parent = NULL;

            while(!temp->left)
            {
                parent = temp;
                temp = temp->left;
            }
            root->objptr = temp->objptr;
            if (parent)
                obj13_bst_delete_node(parent->left, parent->left->objid);
            else
                obj13_bst_delete_node(root->right, root->right->objid);
        }
    }

    return E13_OK;

}

void obj13_bst_inorder_traversal(struct obj13 *root, void (*cb)(struct obj13*))
{
    struct obj13 *cur, *pre;

    assert(cb);

    if(!root)
        return;

    cur = root;

    while(cur) {
        if(!cur->left) {
            cb(cur);
            cur= cur->right;
        } else {
            pre = cur->left;

            while(pre->right && pre->right != cur)
                pre = pre->right;

            if (!pre->right) {
                pre->right = cur;
                cur = cur->left;
            } else {
                pre->right = NULL;
                cb(cur);
                cur = cur->right;
            }
        }
    }
}
