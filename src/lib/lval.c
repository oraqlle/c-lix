#include <lval.h>
#include <builtins.h>
#include <lenv.h>
#include <utilities.h>

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


///////////////////////////
/// `lval` Constructors ///
///////////////////////////

lval* lval_num(long x)
{
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}


lval* lval_err(char* fmt, ...)
{
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    
    va_list va;
    va_start(va, fmt);

    v->err = malloc(512);
    vsnprintf(v->err, 511, fmt, va);

    v->err = realloc(v->err, strlen(v->err) + 1);

    va_end(va);
    return v;
}


lval* lval_sym(char* s)
{
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}


lval* lval_str(char* s)
{
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_STR;
    v->str = malloc(strlen(s) + 1);
    strcpy(v->str, s);
    return v;
}


lval* lval_sexpr(void)
{
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}


lval* lval_qexpr(void)
{
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}


lval* lval_fun(lbuiltin func)
{
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->builtin = func;
    return v;
}


lval* lval_lambda(lval* formals, lval* body)
{
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;

    v->builtin = NULL;

    v->env = lenv_new();

    v->formals = formals;
    v->body = body;
    return v;
}


/////////////////////////
/// `lval` Destructor ///
/////////////////////////

void lval_del(lval* v)
{
    switch (v->type)
    {
        case LVAL_NUM:
            break;
        
        case LVAL_ERR:
            free(v->err);
            break;

        case LVAL_SYM:
            free(v->sym);
            break;

        case LVAL_STR:
            free(v->str);
            break;

        case LVAL_FUN:
            if (!v->builtin)
            {
                lenv_del(v->env);
                lval_del(v->formals);
                lval_del(v->body);
            }
            break;

        case LVAL_QEXPR:
        case LVAL_SEXPR:
            for (int i = 0; i < v->count; i++)
                lval_del(v->cell[i]);

            free(v->cell);
            break;
    }

    free(v);
}


//////////////////////
/// `lval` Methods ///
//////////////////////

lval* lval_add(lval* v, lval* x)
{
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}


lval* lval_copy(lval* v)
{
    lval* x = malloc(sizeof(lval));
    x->type = v->type;

    switch (v->type)
    {
        case LVAL_FUN:
            if (v->builtin)
                x->builtin = v->builtin;
            else
            {
                x->builtin = NULL;
                x->env = lenv_copy(v->env);
                x->formals = lval_copy(v->formals);
                x->body = lval_copy(v->body);
            }
            break;

        case LVAL_NUM:
            x->num = v->num;
            break;

        case LVAL_ERR:
            x->err = malloc(strlen(v->err) + 1);
            strcpy(x->err, v->err);
            break;

        case LVAL_SYM:
            x->sym = malloc(strlen(v->sym) + 1);
            strcpy(x->sym, v->sym);
            break;

        case LVAL_STR:
            x->str = malloc(strlen(v->str) + 1);
            strcpy(x->str, v->str);
            break;

        case LVAL_SEXPR:
        case LVAL_QEXPR:
            x->count = v->count;
            x->cell = malloc(sizeof(lval*) * x->count);
            for (int i = 0; i < x->count; i++)
                x->cell[i] = lval_copy(v->cell[i]);
            break;
    }

    return x;
}


lval* lval_pop(lval* v, int i)
{
    lval* x = v->cell[i];
    memmove(&v->cell[i], &v->cell[i + 1], sizeof(lval*) * (v->count - i - 1));
    v->count--;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);

    return x;
}

lval* lval_take(lval* v, int i)
{
    lval* x = lval_pop(v, i);
    lval_del(v);
    return x;
}


lval* lval_eval(lenv* e, lval* v)
{
    if (v->type == LVAL_SYM)
    {
        lval* x = lenv_get(e, v);
        lval_del(v);
        return x;
    }

    if (v->type == LVAL_SEXPR)
        return lval_eval_sexpr(e, v);

    return v;
}


lval* lval_call(lenv* e, lval* f, lval* a)
{
    if (f->builtin)
        return f->builtin(e, a);

    int given = a->count;
    int total = f->formals->count;

    while (a->count)
    {
        if (f->formals->count == 0)
        {
            lval_del(a);
            return lval_err("Function passed too many arguments. "
                            "Got %i, Expected %i. ", given, total);
        }

        lval* sym = lval_pop(f->formals, 0);

        if (strcmp(sym->sym, "&") == 0)
        {
            if (f->formals->count != 1)
            {
                lval_del(a);

                return lval_err("Function format invalid. "
                                "Symbol '&' not followed by single symbol.");
            }

            lval* nsym = lval_pop(f->formals, 0);
            lenv_put(f->env, nsym, builtin_list(e, a));
            lval_del(sym);
            lval_del(nsym);
            break;
        }

        lval* val = lval_pop(a, 0);
        lenv_put(f->env, sym, val);

        lval_del(sym);
        lval_del(val);
    }

    lval_del(a);

    if (f->formals->count > 0 && strcmp(f->formals->cell[0]->sym, "&") == 0) 
    {
    
        if (f->formals->count != 2) 
        {
            return lval_err("Function format invalid. "
                            "Symbol '&' not followed by single symbol.");
        }
    
        lval_del(lval_pop(f->formals, 0));
        
        lval* sym = lval_pop(f->formals, 0);
        lval* val = lval_qexpr();
        
        lenv_put(f->env, sym, val);
        lval_del(sym); lval_del(val);
    }

    if (f->formals->count == 0)
    {
        f->env->par = e;
        return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
    }
    else
        return lval_copy(f);
}


lval* lval_eval_sexpr(lenv* e, lval* v)
{
    for (int i = 0; i < v->count; i++)
        v->cell[i] = lval_eval(e, v->cell[i]);

    for (int i = 0; i < v->count; i++)
        if (v->cell[i]->type == LVAL_ERR)
            return lval_take(v, i);

    if (v->count == 0)
        return v;

    if (v->count == 1)
        return lval_take(v, 0);

    lval* f = lval_pop(v, 0);

    if (f->type != LVAL_FUN)
    {
        lval* err = lval_err("S-Expression starts with incorrect type. ",
                             "Got %s, Expected %s. ",
                             ltype_name(f->type), ltype_name(LVAL_FUN));

        lval_del(f);
        lval_del(v);
        return err;
    }

    lval* result = lval_call(e, f, v);
    lval_del(f);

    return result;
}


lval* lval_join(lval* x, lval* y)
{
    while (y->count)
        x = lval_add(x, lval_pop(y, 0));

    lval_del(y);
    return x;
}


int lval_eq(lval* x, lval* y)
{
    if (x->type != y->type)
        return 0;

    switch (x->type)
    {
        case LVAL_NUM:
            return (x->num == y->num);

        case LVAL_ERR:
            return (strcmp(x->err, y->err) == 0);

        case LVAL_SYM:
            return (strcmp(x->sym, y->sym) == 0);

        case LVAL_STR:
            return (strcmp(x->str, y->str) == 0);

        case LVAL_FUN:
            if (x->builtin || x->builtin)
                return (x->builtin == y->builtin);
            else
                return (lval_eq(x->formals, y->formals) 
                        && lval_eq(x->body, y->body));

        case LVAL_QEXPR:
        case LVAL_SEXPR:
            if (x->count != y->count)
                return 0;
            
            for (int i = 0; i < x->count; ++i)
                if (!lval_eq(x->cell[i], y->cell[i]))
                    return 0;

            return 1;

        break;
    }

    return 0;
}


////////////////////
/// Prelude Load ///
////////////////////

lval* load_prelude(lenv* e)
{
    #define PRELUDE_PATH_SIZE 100

    char prelude_path[PRELUDE_PATH_SIZE];

    #ifdef _WIN32
        char* envvar = "USERPROFILE";
    #else
        char* envvar = "HOME";
    #endif  /// _WIN32

    if (!getenv(envvar))
    {
        fprintf(stderr, "The environment variable %s was not found.\n", envvar);
        exit(1);
    }

 
    if (snprintf(prelude_path, PRELUDE_PATH_SIZE, "%s/.lix/stdlib/prelude.lx", getenv(envvar)) >= PRELUDE_PATH_SIZE)
    {
        fprintf(stderr, "PRELUDE_PATH_SIZE of %d was too small. Aborting\n", PRELUDE_PATH_SIZE);
        exit(1);
    }    
    
    lval* prelude = lval_add(lval_sexpr(), lval_str(prelude_path));
    lval* p = builtin_load(e, prelude);

    if (p->type == LVAL_ERR)
        lval_println(p);

    return p;
}
