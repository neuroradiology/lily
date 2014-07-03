#include <string.h>

#include "lily_impl.h"
#include "lily_symtab.h"

static void bind_strlist(lily_symtab *symtab, int strlist_size, char **strlist, int *ok)
{
    *ok = 0;

    const int ids[] = {SYM_CLASS_LIST, SYM_CLASS_STR};

    lily_sig *list_str_sig = lily_try_sig_from_ids(symtab, ids);
    if (list_str_sig == NULL)
        return;

    lily_sig *str_sig = list_str_sig->siglist[0];
    lily_var *bound_var = lily_try_new_var(symtab, list_str_sig, "argv", 0);
    if (bound_var == NULL)
        return;

    lily_list_val *lv = lily_malloc(sizeof(lily_list_val));
    lily_value **values = lily_malloc(strlist_size * sizeof(lily_value));
    if (lv == NULL || values == NULL) {
        lily_free(lv);
        lily_free(values);
        return;
    }

    lv->gc_entry = NULL;
    lv->elems = values;
    lv->num_values = strlist_size;
    lv->refcount = 1;
    lv->elems = values;
    lv->visited = 0;
    bound_var->value.list = lv;
    bound_var->flags &= ~VAL_IS_NIL;

    int i, err;
    err = 0;
    for (i = 0;i < strlist_size;i++) {
        values[i] = lily_malloc(sizeof(lily_value));
        if (values[i] == NULL) {
            lv->num_values = i;
            err = 1;
            break;
        }
        values[i]->sig = str_sig;
        values[i]->flags = VAL_IS_NIL;
    }

    if (err == 0) {
        for (i = 0;i < strlist_size;i++) {
            lily_str_val *sv = lily_malloc(sizeof(lily_str_val));
            char *raw_str = lily_malloc(strlen(strlist[i]) + 1);

            if (sv == NULL || raw_str == NULL) {
                lily_free(sv);
                lily_free(raw_str);
                err = 1;
                break;
            }
            strcpy(raw_str, strlist[i]);
            sv->size = strlen(strlist[i]);
            sv->refcount = 1;
            sv->str = raw_str;
            values[i]->flags = 0;
	        values[i]->value.str = sv;
        }
    }

    *ok = !err;
}

int lily_pkg_sys_init(lily_symtab *symtab, int argc, char **argv)
{
    if (symtab == NULL)
        return 0;

    int ok = 0;
    lily_class *package_cls = lily_class_by_id(symtab, SYM_CLASS_PACKAGE);
    lily_sig *package_sig = lily_try_sig_for_class(symtab, package_cls);
    lily_var *bound_var = lily_try_new_var(symtab, package_sig, "sys", 0);

    if (bound_var) {
        lily_var *save_top = symtab->var_top;
        int save_spot = symtab->next_register_spot;

        bind_strlist(symtab, argc, argv, &ok);

        if (ok) {
            lily_package_val *pval = lily_malloc(sizeof(lily_package_val));
            lily_var **package_vars = lily_malloc(1 * sizeof(lily_var *));
            if (pval == NULL || package_vars == NULL) {
                lily_free(pval);
                lily_free(package_vars);
                ok = 0;
            }
            else {
                int i = 0;
                lily_var *var_iter = save_top->next;
                while (var_iter) {
                    package_vars[i] = var_iter;
                    i++;
                    var_iter = var_iter->next;
                }
                symtab->var_top = save_top;
                save_top->next = NULL;
                symtab->next_register_spot = save_spot;

                pval->refcount = 1;
                pval->name = bound_var->name;
                pval->gc_entry = NULL;
                pval->var_count = i;
                pval->vars = package_vars;
                bound_var->flags &= ~VAL_IS_NIL;
                bound_var->value.package = pval;
            }
        }
    }

    return ok;
}