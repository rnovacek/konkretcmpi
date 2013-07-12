/* ex: set tabstop=4 expandtab: */
/*
**==============================================================================
**
** Copyright (c) 2008, Michael E. Brasher
** 
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and associated documentation files (the "Software"),
** to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense,
** and/or sell copies of the Software, and to permit persons to whom the
** Software is furnished to do so, subject to the following conditions:
** 
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
**
**==============================================================================
*/

#include "konkret/konkret.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "mof/MOF_Parser.h"
#include "mof/MOF_Options.h"
#include <vector>
#include <string>
#include <cctype>
#include <cassert>
#include <cstdarg>
#include <map>
#include <set>
#include <cassert>
#include <fstream>
#include <unistd.h>
#include <memory>

using namespace std;

static const char* arg0;
static vector<string> skeletons;
vector<string> classnames;
vector<string> aliases;

vector<char> torder;
vector<pair<string,string> > rall;
vector<pair<string,map<string,string> > > pall;
vector<map<string,string> > mall;

string eta, eti, etn;
string ofile;

bool around = false;

static void substitute(
    string& text, 
    const string& pattern, 
    const string& replacement)
{
    size_t pos;

    while ((pos = text.find(pattern)) != size_t(-1))
        text = text.substr(0, pos) + string(replacement) + text.substr(pos + strlen(pattern.c_str()));
}

static void err(const char* format, ...)
{
    fputc('\n', stderr);
    va_list ap;
    fprintf(stderr, "%s: ", arg0);
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fputc('\n', stderr);
    fputc('\n', stderr);
    exit(1);
}

static const char LINE39[] = "=======================================";

static int put(FILE* os, const char* format, ...)
{
    const char* args[10];
    size_t count = 0;
    const char* arg;

    /* Gather arguments */

    va_list ap;
    va_start(ap, format);

    while ((arg = va_arg(ap, char*)))
    {
        assert(count < 10);
        args[count++] = arg;
    }

    va_end(ap);

    /* Print */

    for (const char* p = format; *p; )
    {
        if (p[0] == '$' && isdigit(p[1]))
        {
            size_t i = p[1] - '0';
            assert(i < count);

            fprintf(os, "%s", args[i]);
            p += 2;
        }
        else
        {
            fputc(*p, os);
            p++;
        }
    }

    return 0;
}

static const MOF_Class_Decl* _find_class(const char* name)
{
    for (const MOF_Class_Decl* p = MOF_Class_Decl::list; 
        p; 
        p = (const MOF_Class_Decl*)(p->next))
    {
        if (strcasecmp(p->name, name) == 0)
            return p;
    }

    return NULL;
}

static size_t find(vector<string>& v, const string& cn)
{
    for (size_t i = 0; i < v.size(); i++)
    {
        if (strcasecmp(v[i].c_str(), cn.c_str()) == 0)
            return i;
    }

    return (size_t)-1;
}

static bool contains(vector<string>& v, const string& cn)
{
    return find(v, cn) != (size_t)-1;
}

static const char* alias(const char* cn)
{
    size_t pos = find(classnames, cn);

    if (pos == (size_t)-1)
        return cn;

    return aliases[pos].c_str();
}

static void direct_dependencies(const string& cn, vector<string>& deps)
{
    deps.clear();

    const MOF_Class_Decl* cd = _find_class(cn.c_str());

    if (!cd)
        err("unknown class: %s", cn.c_str());

    for (MOF_Feature_Info* p=cd->all_features; p; p=(MOF_Feature_Info*)p->next)
    {
        MOF_Reference_Decl* mrd = dynamic_cast<MOF_Reference_Decl*>(p->feature);

        if (mrd && !contains(deps, mrd->class_name))
        {
            deps.push_back(mrd->class_name);
            continue;
        }

        MOF_Method_Decl* mmd = dynamic_cast<MOF_Method_Decl*>(p->feature);

        if (mmd)
        {
            MOF_Parameter* p;

            for (p = mmd->parameters; p; p = (MOF_Parameter*)p->next)
            {
                if (p->data_type == TOK_REF)
                {
                    if (p->ref_name && !contains(deps, p->ref_name))
                    {
                        deps.push_back(p->ref_name);
                    }
                }
            }
            continue;
        }
    }
}

static void recursive_dependencies(const string& cn, vector<string>& deps)
{
    const MOF_Class_Decl* cd = _find_class(cn.c_str());

    if (!cd)
        err("unknown class: %s", cn.c_str());

    if (contains(deps, cd->name))
        return;

    deps.push_back(cd->name);

    if (cd->super_class)
        recursive_dependencies(cd->super_class->name, deps);

    for (MOF_Feature_Info* p=cd->all_features; p; p=(MOF_Feature_Info*)p->next)
    {
        MOF_Reference_Decl* mrd = dynamic_cast<MOF_Reference_Decl*>(p->feature);

        if (mrd && !contains(deps, mrd->class_name))
        {
            recursive_dependencies(mrd->class_name, deps);
            continue;
        }

        MOF_Method_Decl* mmd = dynamic_cast<MOF_Method_Decl*>(p->feature);

        if (mmd)
        {
            MOF_Parameter* p;

            for (p = mmd->parameters; p; p = (MOF_Parameter*)p->next)
            {
                if (p->data_type == TOK_REF)
                {
                    if (p->ref_name && !contains(deps, p->ref_name))
                    {
                        recursive_dependencies(p->ref_name, deps);
                    }
                }
                else if (p->qualifiers->has_key("EmbeddedInstance"))
                {
                    char *name = p->qualifiers->get("EmbeddedInstance")->params->string_value;
                    if (!contains(deps, name)) {
                        recursive_dependencies(name, deps);
                    }
                }
            }
            continue;
        }
    }
}

static bool _is_key(const MOF_Class_Decl* cd, const char* name)
{
    MOF_Feature_Info* p = cd->all_features;

    for (; p; p = (MOF_Feature_Info*)p->next)
    {
        if (strcasecmp(p->feature->name, name) == 0 &&
            p->feature->qual_mask & MOF_QT_KEY)
            return true;
    }

    return false;
}

static const char* _ctype_name(int data_type)
{
    switch (data_type)
    {
        case TOK_BOOLEAN:
            return "CMPIBoolean";
        case TOK_UINT8:
            return "CMPIUint8";
        case TOK_SINT8:
            return "CMPISint8";
        case TOK_UINT16:
            return "CMPIUint16";
        case TOK_SINT16:
            return "CMPISint16";
        case TOK_UINT32:
            return "CMPIUint32";
        case TOK_SINT32:
            return "CMPISint32";
        case TOK_UINT64:
            return "CMPIUint64";
        case TOK_SINT64:
            return "CMPISint64";
        case TOK_REAL32:
            return "CMPIReal32";
        case TOK_REAL64:
            return "CMPIReal64";
        case TOK_CHAR16:
            return "CMPIChar16";
        case TOK_STRING:
            return "CMPIString*";
        case TOK_DATETIME:
            return "CMPIDateTime*";
        case TOK_INSTANCE:
            return "CMPIInstance*";
    }

    // Unreachable
    assert(0);
    return 0;
}

static const char* _ktype_name(int data_type)
{
    switch (data_type)
    {
        case TOK_BOOLEAN:
            return "KBoolean";
        case TOK_UINT8:
            return "KUint8";
        case TOK_SINT8:
            return "KSint8";
        case TOK_UINT16:
            return "KUint16";
        case TOK_SINT16:
            return "KSint16";
        case TOK_UINT32:
            return "KUint32";
        case TOK_SINT32:
            return "KSint32";
        case TOK_UINT64:
            return "KUint64";
        case TOK_SINT64:
            return "KSint64";
        case TOK_REAL32:
            return "KReal32";
        case TOK_REAL64:
            return "KReal64";
        case TOK_CHAR16:
            return "KChar16";
        case TOK_STRING:
            return "KString";
        case TOK_DATETIME:
            return "KDateTime";
        case TOK_REF:
            return "KRef";
        case TOK_INSTANCE:
            return "KInstance";
    }

    // Unreachable
    assert(0);
    return 0;
}

static KTag _ktag(
    int data_type, 
    bool array, 
    bool key,
    bool in,
    bool out)
{
    KTag tag = 0;

    switch (data_type)
    {
        case TOK_BOOLEAN:
            tag = KTYPE_BOOLEAN;
            break;
        case TOK_UINT8:
            tag = KTYPE_UINT8;
            break;
        case TOK_SINT8:
            tag = KTYPE_SINT8;
            break;
        case TOK_UINT16:
            tag = KTYPE_UINT16;
            break;
        case TOK_SINT16:
            tag = KTYPE_SINT16;
            break;
        case TOK_UINT32:
            tag = KTYPE_UINT32;
            break;
        case TOK_SINT32:
            tag = KTYPE_SINT32;
            break;
        case TOK_UINT64:
            tag = KTYPE_UINT64;
            break;
        case TOK_SINT64:
            tag = KTYPE_SINT64;
            break;
        case TOK_REAL32:
            tag = KTYPE_REAL32;
            break;
        case TOK_REAL64:
            tag = KTYPE_REAL64;
            break;
        case TOK_CHAR16:
            tag = KTYPE_CHAR16;
            break;
        case TOK_STRING:
            tag = KTYPE_STRING;
            break;
        case TOK_DATETIME:
            tag = KTYPE_DATETIME;
            break;
        case TOK_REF:
            tag = KTYPE_REFERENCE;
            break;
        case TOK_INSTANCE:
            tag = KTYPE_INSTANCE;
            break;
    }

    if (array)
        tag |= KTAG_ARRAY;

    if (key)
        tag |= KTAG_KEY;

    if (in)
        tag |= KTAG_IN;

    if (out)
        tag |= KTAG_OUT;

    // Unreachable
    return tag;
}

static void pack_count(vector<unsigned char>& sig, size_t count)
{
    assert(count < 256);
    sig.push_back(count);
}

static void pack_tag(vector<unsigned char>& sig, KTag tag)
{
    sig.push_back(tag);
}

static void pack_name(vector<unsigned char>& sig, const char* name)
{
    size_t n = strlen(name);
    assert(n < 256);
    sig.push_back(n);
    sig.insert(sig.end(), name, name + n + 1);
}

static void gen_feature_decls(
    FILE* os, 
    const MOF_Class_Decl* cd,
    const MOF_Class_Decl* lcd,
    vector<unsigned char>& sig,
    size_t& count,
    bool ref)
{
    put(os, "    /* $0 features */\n", cd->name, NULL);

    for (MOF_Feature_Info* p = cd->all_features; p; 
        p = (MOF_Feature_Info*)p->next)
    {
        MOF_Feature* mf = p->feature;

        if (strcasecmp(cd->name, p->class_origin->name) != 0)
            continue;

        bool key = _is_key(lcd, mf->name);

        // CIM_Class_Property:

        MOF_Property_Decl* pd = dynamic_cast<MOF_Property_Decl*>(mf);

        if (pd)
        {
            const char* pn = pd->name;
            const char* ktn = _ktype_name(pd->data_type);

            if (ref && !key)
                continue;

            if (pd->qualifiers->has_key("EmbeddedInstance")) {
                if (pd->array_index)
                    put(os, "    const KInstanceA $0;\n", pn, NULL);
                else
                    put(os, "    const KInstance $0;\n", pn, NULL);
            }
            else
            {
                if (pd->array_index == 0)
                    put(os, "    const $0 $1;\n", ktn, pn, NULL);
                else
                    put(os, "    const $0A $1;\n", ktn, pn, NULL);
            }

            // Add sig entry [type][length][name][zero-terminator]

            KTag tag = _ktag(pd->data_type, pd->array_index, key, false, false);
            pack_tag(sig, tag);
            pack_name(sig, pd->name);
            count++;
            continue;
        }

        // CIM_Class_Reference:

        MOF_Reference_Decl* mrd = dynamic_cast<MOF_Reference_Decl*>(mf);

        if (mrd)
        {
            if (ref && !key)
                continue;

            // Find reference in leaf class since the type may have changed:

            MOF_Feature_Info* q = lcd->all_features;
            const char* sn = NULL;

            for (; q; q = (MOF_Feature_Info*)q->next)
            {
                if (strcasecmp(q->feature->name, mrd->name) == 0)
                {
                    MOF_Reference_Decl* qref =
                        dynamic_cast<MOF_Reference_Decl*>(q->feature);

                    if (qref)
                    {
                        sn = qref->class_name;
                        break;
                    }
                }
            }

            if (!sn)
            {
                fprintf(stderr, "%s: unexpected\n", arg0);
                exit(1);
            }

            // Print field declaration:

            const char* rn = mrd->name;
            put(os, "    const KRef $1; /* $0 */\n", alias(sn), rn, NULL);

            // Add sig entry [type][length][name][zero-terminator]

            KTag tag = KTYPE_REFERENCE;

            if (key)
                tag |= KTAG_KEY;

            pack_tag(sig, tag);
            pack_name(sig, rn);
            count++;
            continue;
        }

        // CIM_Class_Method:

        MOF_Method_Decl* mmd = dynamic_cast<MOF_Method_Decl*>(mf);

        if (mmd)
        {
            continue;
        }
    }
}

static void gen_feature_decls_recursive(
    FILE* os,
    const MOF_Class_Decl* cd,
    const MOF_Class_Decl* lcd,
    vector<unsigned char>& sig,
    size_t& count,
    bool ref)
{
    if (cd->super_class)
        gen_feature_decls_recursive(os, cd->super_class, lcd, sig, count, ref);
    
    gen_feature_decls(os, cd, lcd, sig, count, ref);
}


static void gen_sig(
    FILE* os, 
    const char* sn,
    const vector<unsigned char>& sig)
{
    fprintf(os, "static const unsigned char __%s_sig[] =\n", sn);
    fprintf(os, "{");

    /* Write signature bytes */

    for (size_t i = 0; i < sig.size(); i++)
    {
        if (i % 15 == 0)
            fprintf(os, "\n    ");
        fprintf(os, "0x%02x,", sig[i]);
    }

    fprintf(os, "\n};\n\n");
}

static void gen_param(FILE* os, MOF_Parameter* p, vector<unsigned char>& sig)
{
    bool in = p->qual_mask & MOF_QT_IN;
    bool out = p->qual_mask & MOF_QT_OUT;

    KTag tag = _ktag(p->data_type, p->array_index, false, in, out);

    if (in && out)
        put(os, "    /* IN OUT */\n", NULL);
    else if (in)
        put(os, "    /* IN */\n", NULL);
    else if (out)
        put(os, "    /* OUT */\n", NULL);

    if (p->data_type == TOK_REF)
    {
        const char* rn = alias(p->ref_name);

        if (p->array_index)
            put(os, "    KRefA $0; /* $1 */\n", p->name, rn, NULL);
        else
            put(os, "    KRef $0; /* $1 */\n", p->name, rn, NULL);
    }
    else
    {
        const char* ktn = _ktype_name(p->data_type);

        if (p->qualifiers->has_key("EmbeddedInstance")) {
            if (p->array_index)
                put(os, "    KInstanceA $0;\n", p->name, NULL);
            else
                put(os, "    KInstance $0;\n", p->name, NULL);
        }
        else
        {
            if (p->array_index)
                put(os, "    $0A $1;\n", ktn, p->name, NULL);
            else
                put(os, "    $0 $1;\n", ktn, p->name, NULL);
        }
    }

    pack_tag(sig, tag);
    pack_name(sig, p->name);
}

static void gen_method(
    FILE* os, 
    const MOF_Class_Decl* cd, 
    const MOF_Method_Decl* md)
{
    char msn[1024];
    const char* sn = alias(cd->name);

    sprintf(msn, "%s_%s_Args", sn, md->name);

    // Class header:

    /* $0=cn $1=msn */
    const char HEADER[] =
        "/* classname=$0 */\n"
        "typedef struct _$1\n"
        "{\n"
        "    KBase __base;"
        "\n";

    put(os, HEADER, cd->name, msn, NULL);

    vector<unsigned char> sig;
    pack_name(sig, md->name);
    size_t pos = sig.size();
    pack_count(sig, 0xFF);
    size_t count = 0;

    for (MOF_Parameter* p = md->parameters; p; p = (MOF_Parameter*)p->next)
    {
        gen_param(os, p, sig);
        count++;
    }

    assert(count < 256);
    sig[pos] = count;

    // Trailer:

    /* $0=sn $1=mn */
    const char TRAILER[] = 
        "}\n"
        "$0_$1_Args;\n"
        "\n";

    put(os, TRAILER, sn, md->name, NULL);

    gen_sig(os, msn, sig);
}

static void gen_meth_init(
    FILE* os, 
    const MOF_Class_Decl* cd, 
    const MOF_Method_Decl* md)
{
    char msn[1024];
    const char* sn = alias(cd->name);

    /* $0=msn */
    const char FMT1[] =
        "KINLINE void $0_Init(\n"
        "    $0* self,\n"
        "    const CMPIBroker* cb)\n"
        "{\n"
        "    const unsigned char* sig = __$0_sig;\n"
        "    KBase_Init(&self->__base, cb, sizeof(*self), sig, NULL);\n";

    sprintf(msn, "%s_%s_Args", sn, md->name);

    put(os, FMT1, msn, NULL);

    for (MOF_Parameter* p = md->parameters; p; p = (MOF_Parameter*)p->next)
    {
        if (p->data_type == TOK_REF)
        {
            if (p->array_index)
            {
                put(os, "    self->$0.__sig = __$1_sig;\n",
                    p->name, alias(p->ref_name), NULL);
            }
            else
            {
                put(os, "    self->$0.__sig = __$1_sig;\n",
                    p->name, alias(p->ref_name), NULL);
            }
        }
    }

    put(os, "}\n\n", NULL);
}

static void gen_meth_init_from_args(
    FILE* os, 
    const MOF_Class_Decl* cd, 
    const MOF_Method_Decl* md)
{
    char msn[1024];
    const char* sn = alias(cd->name);

    /* $0=msn */
    const char FMT1[] =
        "KINLINE CMPIStatus $0_InitFromArgs(\n"
        "    $0* self,\n"
        "    const CMPIBroker* cb,\n"
        "    const CMPIArgs* x,\n"
        "    CMPIBoolean in,\n"
        "    CMPIBoolean out)\n"
        "{\n"
        "    $0_Init(self, cb);\n"
        "    return KBase_FromArgs(&self->__base, x, in, out);\n"
        "}\n"
        "\n";

    sprintf(msn, "%s_%s_Args", sn, md->name);
    put(os, FMT1, msn, NULL);
}

static void gen_meth_print(
    FILE* os, 
    const MOF_Class_Decl* cd, 
    const MOF_Method_Decl* md)
{
    const char* sn = alias(cd->name);

    char msn[1024];
    sprintf(msn, "%s_%s_Args", sn, md->name);

    /* $0=sn $1=ref */
    const char FMT[] =
        "KINLINE void $0_Print(\n"
        "    const $0* self,\n"
        "    FILE* os)\n"
        "{\n"
        "    KBase_Print(os, &self->__base, 'a');\n"
        "}\n"
        "\n";

    put(os, FMT, msn, NULL);
}

static void gen_meth_args(
    FILE* os, 
    const MOF_Class_Decl* cd, 
    const MOF_Method_Decl* md)
{
    const char* sn = alias(cd->name);
    const char* mn = md->name;

    /* $0=sn $1=mn */
    const char FMT[] =
        "KINLINE CMPIArgs* $0_$1_Args_ToArgs(\n"
        "    const $0_$1_Args* self,\n"
        "    CMPIBoolean in,\n"
        "    CMPIBoolean out,\n"
        "    CMPIStatus* status)\n"
        "{\n"
        "    return KBase_ToArgs(&self->__base, in, out, status);\n"
        "}\n"
        "\n";

    put(os, FMT, sn, mn, NULL);
}

static void gen_meth_set_args(
    FILE* os, 
    const MOF_Class_Decl* cd, 
    const MOF_Method_Decl* md)
{
    const char* sn = alias(cd->name);
    const char* mn = md->name;

    /* $0=sn $1=mn */
    const char FMT[] =
        "KINLINE CMPIStatus $0_$1_Args_SetArgs(\n"
        "    const $0_$1_Args* self,\n"
        "    CMPIBoolean in,\n"
        "    CMPIBoolean out,\n"
        "    CMPIArgs* ca)\n"
        "{\n"
        "    return KBase_SetToArgs(&self->__base, in, out, ca);\n"
        "}\n"
        "\n";

    put(os, FMT, sn, mn, NULL);
}

static void gen_meth_header(
    FILE* os, 
    const MOF_Class_Decl* cd, 
    const MOF_Method_Decl* md,
    bool prototype)
{
    const char* sn = alias(cd->name);
    const char* mn = md->name;
    const char* ktn = _ktype_name(md->data_type);

    if (prototype)
        put(os, "KEXTERN ", NULL);

    /* $0=sn $1=mn $2=ktn */
    const char HEADER[] =
        "$2 $0_$1(\n"
        "    const CMPIBroker* cb,\n"
        "    CMPIMethodMI* mi,\n"
        "    const CMPIContext* context,\n"
        "    const $0Ref* self,\n";

    const char STATIC_HEADER[] = 
        "$2 $0_$1(\n"
        "    const CMPIBroker* cb,\n"
        "    CMPIMethodMI* mi,\n"
        "    const CMPIContext* context,\n";

    if (md->qual_mask & MOF_QT_STATIC)
        put(os, STATIC_HEADER, sn, mn, ktn, NULL);
    else
        put(os, HEADER, sn, mn, ktn, NULL);

    for (MOF_Parameter* p = md->parameters; p; p = (MOF_Parameter*)p->next)
    {
        const char* mod = (p->qual_mask & MOF_QT_OUT) ? "" : "const ";

        if (p->data_type == TOK_REF)
        {
            if (p->array_index)
                put(os, "    $0KRefA* $1,\n", mod, p->name, NULL);
            else
                put(os, "    $0KRef* $1,\n", mod, p->name, NULL);
        }
        else
        {
            const char* ktn = _ktype_name(p->data_type);
            if (p->qualifiers->has_key("EmbeddedInstance")) {
                if (p->array_index)
                    put(os, "    $0KInstanceA* $1,\n", mod, p->name, NULL);
                else
                    put(os, "    $0KInstance* $1,\n", mod, p->name, NULL);
            } else {
                if (p->array_index)
                    put(os, "    $0$1A* $2,\n", mod, ktn, p->name, NULL);
                else
                    put(os, "    $0$1* $2,\n", mod, ktn, p->name, NULL);
            }
        }
    }

    const char TRAILER[] =
        "    CMPIStatus* status)";
    const char TRAILER_AROUND[] =
        "    CMPIStatus* __status)";
  

    if (around)
        put(os, TRAILER_AROUND, sn, mn, NULL);
    else
        put(os, TRAILER, sn, mn, NULL);

    if (prototype)
        put(os, ";\n\n", NULL);
    else
        put(os, "\n", NULL);
}

static const char* to_upper(char* buf, const char* s)
{
    char* p = buf;

    while (*s)
        *p++ = toupper(*s++);

    *p = '\0';

    return buf;
}

static void exmethod(FILE* os, const MOF_Class_Decl* cd, const MOF_Method_Decl* md)
{

    string append;
/*    if (md->qual_mask & MOF_QT_STATIC)
        put(os, STATIC_HEADER, sn, mn, ktn, NULL);
    else
        put(os, HEADER, sn, mn, ktn, NULL);
*/
    for (MOF_Parameter* p = md->parameters; p; p = (MOF_Parameter*)p->next)
    {
/*        const char* mod = (p->qual_mask & MOF_QT_OUT) ? "" : "const ";*/

        string type;
        string name;
        if (p->data_type == TOK_REF)
        {
            if (p->array_index)
                type = string("KrefA");
            else
       
                type = string("Kref");
        }
        else
        {
            const char* ktn = _ktype_name(p->data_type);

            if (p->array_index)
                type = string(ktn)+string("A"); /* p->name */
            else
                type = string(ktn);
        }
        name = p->name;

        for (vector<map<string,string> >::iterator mi = mall.begin(); mi != mall.end(); mi++)
        {
            map<string,string> mrules = (*mi);
            if (mrules.find(type) != mrules.end()){
                append = mrules[type];
                printf("\nTo be appended to method %s\n", append.c_str());
                substitute(append, "<MTYPE>", type);
                substitute(append, "<MNAME>", name);
                printf("\nTransformed method appendix %s\n", append.c_str());
                put(os, append.c_str(), NULL);

            }

        }
        
    }


    return;

}

static void gen_meth_stub(
    FILE* os, 
    const MOF_Class_Decl* cd, 
    const MOF_Method_Decl* md)
{
    const char* ktn = _ktype_name(md->data_type);
    char buf[1024];

    gen_meth_header(os, cd, md, false);

    // $0=ktn $0=ktnu
    const char BODY_LEAD[] =
        "{\n"
        "    $0 result = $1_INIT;\n"
        "\n";
    const char BODY_LEAD_AROUND[] =
        "{\n"
        "    $0 result = $1_INIT;\n"
        "\n";

    if (around)
        put(os, BODY_LEAD_AROUND, ktn, to_upper(buf, ktn), NULL);
    else
        put(os, BODY_LEAD, ktn, to_upper(buf, ktn), NULL);

    exmethod(os, cd, md);

    const char BODY_OUT[] =
        "    KSetStatus(status, ERR_NOT_SUPPORTED);\n"
        "    return result;\n"
        "}\n"
        "\n";
    const char BODY_OUT_AROUND[] =
        "    KSetStatus(__status, ERR_NOT_SUPPORTED);\n"
        "    return result;\n"
        "}\n"
        "\n";

    if (around)
        put(os, BODY_OUT_AROUND, ktn, to_upper(buf, ktn), NULL);
    else
        put(os, BODY_OUT, ktn, to_upper(buf, ktn), NULL);

}

static void gen_meth_call(
    FILE* os, 
    const MOF_Class_Decl* cd, 
    const MOF_Method_Decl* md)
{
    const char* sn = alias(cd->name);
    const char* mn = md->name;
    const char* ktn = _ktype_name(md->data_type);
    const char* tn = ktn + 1;

    // $0=sn $1=mn $2=ktn
    const char HEADER[] =
        "    if (strcasecmp(meth, \"$1\") == 0)\n"
        "    {\n"
        "        CMPIStatus st = KSTATUS_INIT;\n"
        "        $0_$1_Args args;\n"
        "        $2 r;\n"
        "\n"
        "        KReturnIf($0_$1_Args_InitFromArgs(\n"
        "            &args, cb, in, 1, 0));\n"
        "\n"
        "        r = $0_$1(\n"
        "            cb,\n"
        "            mi,\n"
        "            cc,\n";

    put(os, HEADER, sn, mn, ktn, NULL);


    if (!(md->qual_mask & MOF_QT_STATIC))
        put(os, "            &self,\n", NULL);

    for (MOF_Parameter* p = md->parameters; p; p = (MOF_Parameter*)p->next)
    {
        put(os, "            &args.$0,\n", p->name, NULL);
    }

    // $0=sn $1=mn $2=tn
    const char TRAILER[] =
        "            &st);\n"
        "\n"
        "        if (!KOkay(st))\n"
        "            return st;\n"
        "\n"
        "        if (!r.exists)\n"
        "            KReturn(ERR_FAILED);\n"
        "\n"
        "        KReturnIf($0_$1_Args_SetArgs(\n"
        "            &args, 0, 1, out));\n"
        "        KReturn$2Data(cr, &r);\n"
        "        CMReturnDone(cr);\n"
        "\n"
        "        KReturn(OK);\n"
        "    }\n";

    put(os, TRAILER, sn, mn, tn, NULL);
}

static size_t count_methods(const MOF_Class_Decl* cd)
{
    size_t n = 0;

    MOF_Feature_Info* p;

    for (p = cd->all_features; p; p = (MOF_Feature_Info*)p->next)
    {

        MOF_Method_Decl* md = dynamic_cast<MOF_Method_Decl*>(p->feature);

        if (md)
            n++;
    }

    return n;
}

static void gen_meth_stubs(FILE* os, const MOF_Class_Decl* cd)
{
    // Avoid all this if class has no methods.

    if (count_methods(cd) == 0)
        return;

    // Write the stubs:

    MOF_Feature_Info* p;

    for (p = cd->all_features; p; p = (MOF_Feature_Info*)p->next)
    {

        MOF_Method_Decl* md = dynamic_cast<MOF_Method_Decl*>(p->feature);

        if (md)
            gen_meth_stub(os, cd, md);
    }
}

static void gen_methods(FILE* os, const MOF_Class_Decl* cd, const char* sn)
{
    MOF_Feature_Info* p;

    for (p = cd->all_features; p; p = (MOF_Feature_Info*)p->next)
    {

        MOF_Method_Decl* md = dynamic_cast<MOF_Method_Decl*>(p->feature);

        if (md)
        {
            gen_method(os, cd, md);
            gen_meth_init(os, cd, md);
            gen_meth_init_from_args(os, cd, md);
            gen_meth_args(os, cd, md);
            gen_meth_set_args(os, cd, md);
            gen_meth_print(os, cd, md);
        }
    }

    // Generate comment box:

    const char BOX[] =
        "/*\n"
        "**$1$1\n"
        "**\n"
        "** $0 methods\n"
        "**\n"
        "**$1$1\n"
        "*/\n"
        "\n";

    put(os, BOX, sn, LINE39, NULL);

    // Generate prototypes:

    size_t num_methods = 0;

    for (p = cd->all_features; p; p = (MOF_Feature_Info*)p->next)
    {

        MOF_Method_Decl* md = dynamic_cast<MOF_Method_Decl*>(p->feature);

        if (md)
        {
            gen_meth_header(os, cd, md, true);
            num_methods++;
        }
    }

    // Generate InvokeMethod(). 

    // $0=sn
    const char HEADER[] =
        "KINLINE CMPIStatus $0_DispatchMethod(\n"
        "    const CMPIBroker* cb,\n"
        "    CMPIMethodMI* mi,\n"
        "    const CMPIContext* cc,\n"
        "    const CMPIResult* cr,\n"
        "    const CMPIObjectPath* cop,\n"
        "    const char* meth,\n"
        "    const CMPIArgs* in,\n"
        "    CMPIArgs* out)\n"
        "{\n"
        "    $0Ref self;\n"
        "\n"
        "    KReturnIf($0Ref_InitFromObjectPath(&self, cb, cop));\n"
        "\n";

    put(os, HEADER, sn, NULL);

    for (p = cd->all_features; p; p = (MOF_Feature_Info*)p->next)
    {

        MOF_Method_Decl* md = dynamic_cast<MOF_Method_Decl*>(p->feature);

        if (md)
            gen_meth_call(os, cd, md);
    }

    const char TRAILER[] =
        "\n"
        "    KReturn(ERR_METHOD_NOT_FOUND);\n"
        "}\n"
        "\n";

    put(os, TRAILER, NULL);
}

static void gen_class(
    FILE* os, 
    const MOF_Class_Decl* cd,
    const char* sn,
    bool ref)
{
    // Comment box:

    const char BOX[] =
        "/*\n"
        "**$1$1\n"
        "**\n"
        "** struct $0 \n"
        "**\n"
        "**$1$1\n"
        "*/\n"
        "\n";

    put(os, BOX, sn, LINE39, NULL);

    // Class header:

    const char HEADER[] =
        "/* classname=$0 */\n"
        "typedef struct _$1\n"
        "{\n"
        "    KBase __base;"
        "\n";

    put(os, HEADER, cd->name, sn, NULL);

    // Features declarations:

    vector<unsigned char> sig;
    pack_name(sig, cd->name);
    size_t pos = sig.size();
    pack_count(sig, 0xFF);
    size_t count = 0;

    gen_feature_decls_recursive(os, cd, cd, sig, count, ref);

    // Pack count:
    sig[pos] = count;

    // Trailer:

    const char TRAILER[] = 
        "}\n"
        "$0;\n"
        "\n";

    put(os, TRAILER, sn, NULL);

    gen_sig(os, sn, sig);
}

static void gen_init(
    FILE* os, 
    const MOF_Class_Decl* cd,
    const char* sn,
    bool ref)
{
    /* $0=sn */
    const char FMT1[] =
        "KINLINE void $0_Init(\n"
        "    $0* self,\n"
        "    const CMPIBroker* cb,\n"
        "    const char* ns)\n"
        "{\n"
        "    const unsigned char* sig = __$0_sig;\n"
        "    KBase_Init(&self->__base, cb, sizeof(*self), sig, ns);\n";

    put(os, FMT1, sn, NULL);

    for (MOF_Feature_Info* p = cd->all_features; p; 
        p = (MOF_Feature_Info*)p->next)
    {
        MOF_Feature* mf = p->feature;

        MOF_Reference_Decl* mrd = dynamic_cast<MOF_Reference_Decl*>(mf);

        if (mrd)
        {
            const char FMT[] = "    ((KRef*)&self->$0)->__sig = __$1_sig;\n";
            put(os, FMT, mrd->name, alias(mrd->class_name), NULL);
        }
    }

    put(os, "}\n\n", NULL);

    /* $0=sn $1=(Instance|ObjectPath) */
    const char FMT2[] =
        "KINLINE CMPIStatus $0_InitFrom$1(\n"
        "    $0* self,\n"
        "    const CMPIBroker* cb,\n"
        "    const CMPI$1* x)\n"
        "{\n"
        "    $0_Init(self, cb, NULL);\n"
        "    return KBase_From$1(&self->__base, x);\n"
        "}\n"
        "\n";

    put(os, FMT2, sn, "Instance", NULL);
    put(os, FMT2, sn, "ObjectPath", NULL);
}

static void gen_instance(
    FILE* os, 
    const MOF_Class_Decl* cd,
    const char* sn,
    bool ref)
{
    const char FMT[] =
        "KINLINE CMPIInstance* $0_ToInstance(\n"
        "    const $0* self,\n"
        "    CMPIStatus* status)\n"
        "{\n"
        "    return KBase_ToInstance(&self->__base, status);\n"
        "}\n"
        "\n";

    put(os, FMT, sn, NULL);
}

static void gen_object_path(
    FILE* os, 
    const MOF_Class_Decl* cd,
    const char* sn,
    bool ref)
{
    const char SOURCE[] =
        "KINLINE CMPIObjectPath* $0_ToObjectPath(\n"
        "    const $0* self,\n"
        "    CMPIStatus* status)\n"
        "{\n"
        "    return KBase_ToObjectPath(&self->__base, status);\n"
        "}\n"
        "\n";

    put(os, SOURCE, sn, NULL);
}

static void gen_ns(FILE* os, const char* sn)
{
    /* $0=sn */
    const char FMT[] =
        "KINLINE const char* $0_NameSpace(\n"
        "    $0* self)\n"
        "{\n"
        "    if (self && self->__base.magic == KMAGIC)\n"
        "        return self->__base.ns ? KChars(self->__base.ns) : NULL;\n"
        "    return NULL;\n"
        "}\n"
        "\n";

    put(os, FMT, sn, NULL);
}

static void gen_print(
    FILE* os, 
    const MOF_Class_Decl* cd,
    const char* sn,
    bool ref)
{
    /* $0=sn $1=ref */
    const char FMT[] =
        "KINLINE void $0_Print(\n"
        "    const $0* self,\n"
        "    FILE* os)\n"
        "{\n"
        "    KBase_Print(os, &self->__base, $1);\n"
        "}\n"
        "\n";

    put(os, FMT, sn, (ref ? "'r'" : "'i'"), NULL);
}

static void gen_array_init(
    FILE* os, 
    const MOF_Class_Decl* cd,
    const MOF_Property_Decl* mpd,
    const char* sn)
{
    const char* pn = mpd->name;
    const char* ktn = _ktype_name(mpd->data_type);

    /* $0=sn $1=pn $2=ktn */
    const char FMT1[] =
        "KINLINE CMPIBoolean $0_Init_$1(\n"
        "    $0* self,\n"
        "    CMPICount count)\n"
        "{\n"
        "    if (self && self->__base.magic == KMAGIC)\n"
        "    {\n"
        "        $2A* field = ($2A*)&self->$1;\n"
        "        return $2A_Init(field, self->__base.cb, count);\n" \
        "    }\n"
        "    return 0;\n"
        "}\n"
        "\n";
    const char FMT2[] =
        "KINLINE void $0_InitNull_$1(\n"
        "    $0* self)\n"
        "{\n"
        "    if (self && self->__base.magic == KMAGIC)\n"
        "    {\n"
        "        $2A* field = ($2A*)&self->$1;\n"
        "        $2A_InitNull(field);\n" \
        "    }\n"
        "}\n"
        "\n";

    if (mpd->array_index != 0)
    {
        put(os, FMT1, sn, pn, ktn, NULL);
        put(os, FMT2, sn, pn, ktn, NULL);
    }
}

static void gen_set(
    FILE* os, 
    const MOF_Class_Decl* cd,
    const MOF_Property_Decl* mpd,
    const char* sn)
{
    const char* pn = mpd->name;
    const char* ctn = _ctype_name(mpd->data_type);
    const char* ktn = _ktype_name(mpd->data_type);
    const char* ext = mpd->data_type == TOK_STRING ? "String" : "";

    /* $0=sn $1=pn $2=ctn $3=ktn $4=ext */
    const char FMT1[] =
        "KINLINE void $0_Set$4_$1(\n"
        "    $0* self,\n"
        "    $2 x)\n"
        "{\n"
        "    if (self && self->__base.magic == KMAGIC)\n"
        "    {\n"
        "        $3* field = ($3*)&self->$1;\n"
        "        $3_Set$4(field, x);\n" \
        "    }\n"
        "}\n"
        "\n";
    /* $0=sn $1=pn $2=ctn $3=ktn */
    const char FMT2[] =
        "KINLINE void $0_Set_$1(\n"
        "    $0* self,\n"
        "    const char* s)\n"
        "{\n"
        "    if (self && self->__base.magic == KMAGIC)\n"
        "    {\n"
        "        $3* field = ($3*)&self->$1;\n"
        "        KString_Set(field, self->__base.cb, s);\n"
        "    }\n"
        "}\n"
        "\n";
    /* $0=sn $1=pn $2=ctn $3=ktn $4=ext */
    const char FMT3[] =
        "KINLINE CMPIBoolean $0_Set$4_$1(\n"
        "    $0* self,\n"
        "    CMPICount i,\n"
        "    $2 x)\n"
        "{\n"
        "    if (self && self->__base.magic == KMAGIC)\n"
        "    {\n"
        "        $3A* field = ($3A*)&self->$1;\n"
        "        return $3A_Set$4(field, i, x);\n"
        "    }\n"
        "    return 0;\n"
        "}\n"
        "\n";
    /* $0=sn $1=pn $2=ctn $3=ktn */
    const char FMT4[] =
        "KINLINE CMPIBoolean $0_Set_$1(\n"
        "    $0* self,\n"
        "    CMPICount i,\n"
        "    const char* s)\n"
        "{\n"
        "    if (self && self->__base.magic == KMAGIC)\n"
        "    {\n"
        "        $3A* field = ($3A*)&self->$1;\n"
        "        return KStringA_Set(field, self->__base.cb, i, s);\n"
        "    }\n"
        "    return 0;\n"
        "}\n"
        "\n";

    if (mpd->array_index == 0)
    {
        if (mpd->data_type == TOK_STRING)
        {
            put(os, FMT1, sn, pn, ctn, ktn, ext, NULL);
            put(os, FMT2, sn, pn, ctn, ktn, NULL);
        }
        else
            put(os, FMT1, sn, pn, ctn, ktn, ext, NULL);
    }
    else
    {
        if (mpd->data_type == TOK_STRING)
        {
            put(os, FMT3, sn, pn, ctn, ktn, ext, NULL);
            put(os, FMT4, sn, pn, ctn, ktn, NULL);
        }
        else
        {
            put(os, FMT3, sn, pn, ctn, ktn, ext, NULL);
        }
    }
}

static void gen_get(
    FILE* os, 
    const MOF_Class_Decl* cd,
    const MOF_Property_Decl* mpd,
    const char* sn)
{
    const char* pn = mpd->name;
    const char* ctn = _ctype_name(mpd->data_type);
    const char* ktn = _ktype_name(mpd->data_type);
    const char* ext = mpd->data_type == TOK_STRING ? "String" : "";

    /* $0=sn $1=pn $2=ctn $3=ktn $4=ext */
    const char FMT1[] =
        "KINLINE $3 $0_Get$4_$1(\n"
        "    $0* self,\n"
        "    CMPICount i)\n"
        "{\n"
        "    if (self && self->__base.magic == KMAGIC)\n"
        "    {\n"
        "        $3A* field = ($3A*)&self->$1;\n"
        "        return $3A_Get$4(field, i);\n"
        "    }\n"
        "    return $3A_Get$4(NULL, 0);\n"
        "}\n"
        "\n";
    const char FMT2[] =
        "KINLINE const char* $0_Get_$1(\n"
        "    $0* self,\n"
        "    CMPICount i)\n"
        "{\n"
        "    if (self && self->__base.magic == KMAGIC)\n"
        "    {\n"
        "        $2A* field = ($2A*)&self->$1;\n"
        "        return $2A_Get(field, i);\n"
        "    }\n"
        "    return NULL;\n"
        "}\n"
        "\n";

    if (mpd->array_index != 0)
    {
        put(os, FMT1, sn, pn, ctn, ktn, ext, NULL);

        if (mpd->data_type == TOK_STRING)
            put(os, FMT2, sn, pn, ktn, NULL);
    }
}

static void gen_set_ref(
    FILE* os, 
    const MOF_Class_Decl* cd,
    const MOF_Reference_Decl* mrd,
    const char* sn)
{
    const char* pn = mrd->name;
    const char* al = alias(mrd->class_name);

    /* $0=sn $1=pn $2=al */
    const char FMT1[] =
        "KINLINE void $0_SetObjectPath_$1(\n"
        "    $0* self,\n"
        "    const CMPIObjectPath* x)\n"
        "{\n"
        "    if (self && self->__base.magic == KMAGIC)\n"
        "    {\n"
        "        KRef* field = (KRef*)&self->$1;\n"
        "        KRef_SetObjectPath(field, x);\n"
        "    }\n"
        "}\n"
        "\n";
    /* ATTN: Add namespace to object path */
    /* $0=sn $1=pn $2=al */
    const char FMT2[] =
        "KINLINE CMPIStatus $0_Set_$1(\n"
        "    $0* self,\n"
        "    const $2Ref* x)\n"
        "{\n"
        "    if (self && self->__base.magic == KMAGIC)\n"
        "    {\n"
        "        KRef* field = (KRef*)&self->$1;\n"
        "        return KRef_Set(field, &x->__base);\n"
        "    }\n"
        "    CMReturn(CMPI_RC_ERR_FAILED);\n"
        "}\n"
        "\n";

    put(os, FMT1, sn, pn, al, NULL);
    put(os, FMT2, sn, pn, al, NULL);
}

static void gen_null_ref(
    FILE* os, 
    const MOF_Class_Decl* cd,
    const MOF_Reference_Decl* mrd,
    const char* sn)
{
    const char* pn = mrd->name;
    const char* al = alias(mrd->class_name);

    /* $0=sn $1=pn $2=al */
    const char FMT[] =
        "KINLINE void $0_Null_$1(\n"
        "    $0* self)\n"
        "{\n"
        "    if (self && self->__base.magic == KMAGIC)\n"
        "    {\n"
        "        KRef* field = (KRef*)&self->$1;\n"
        "        KRef_Null(field);\n"
        "    }\n"
        "}\n"
        "\n";

    put(os, FMT, sn, pn, al, NULL);
}

static void gen_clr_ref(
    FILE* os, 
    const MOF_Class_Decl* cd,
    const MOF_Reference_Decl* mrd,
    const char* sn)
{
    const char* pn = mrd->name;
    const char* al = alias(mrd->class_name);

    /* $0=sn $1=pn $2=al */
    const char FMT[] =
        "KINLINE void $0_Clr_$1(\n"
        "    $0* self)\n"
        "{\n"
        "    if (self && self->__base.magic == KMAGIC)\n"
        "    {\n"
        "        KRef* field = (KRef*)&self->$1;\n"
        "        KRef_Clr(field);\n"
        "    }\n"
        "}\n"
        "\n";

    put(os, FMT, sn, pn, al, NULL);
}

static void gen_null(
    FILE* os, 
    const MOF_Class_Decl* cd,
    const MOF_Property_Decl* mpd,
    const char* sn)
{
    const char* pn = mpd->name;
    const char* ktn = _ktype_name(mpd->data_type);

    /* $0=sn $1=pn $2=ktn */
    const char FMT1[] =
        "KINLINE void $0_Null_$1(\n"
        "    $0* self)\n"
        "{\n"
        "    if (self && self->__base.magic == KMAGIC)\n"
        "    {\n"
        "        $2* field = ($2*)&self->$1;\n"
        "        $2_Null(field);\n"
        "    }\n"
        "}\n"
        "\n";
    /* $0=sn $1=pn $2=ktn */
    const char FMT2[] =
        "KINLINE CMPIBoolean $0_Null_$1(\n"
        "    $0* self,\n"
        "    CMPICount i)\n"
        "{\n"
        "    if (self && self->__base.magic == KMAGIC)\n"
        "    {\n"
        "        $2A* field = ($2A*)&self->$1;\n"
        "        return $2A_Null(field, i);\n"
        "    }\n"
        "    return 0;\n"
        "}\n"
        "\n";

    if (mpd->array_index == 0)
        put(os, FMT1, sn, pn, ktn, NULL);
    else
        put(os, FMT2, sn, pn, ktn, NULL);
}

static void gen_clr(
    FILE* os, 
    const MOF_Class_Decl* cd,
    const MOF_Property_Decl* mpd,
    const char* sn)
{
    const char* pn = mpd->name;
    const char* ktn = _ktype_name(mpd->data_type);

    /* $0=sn $1=pn $2=ktn */
    const char FMT1[] =
        "KINLINE void $0_Clr_$1(\n"
        "    $0* self)\n"
        "{\n"
        "    if (self && self->__base.magic == KMAGIC)\n"
        "    {\n"
        "        $2* field = ($2*)&self->$1;\n"
        "        $2_Clr(field);\n"
        "    }\n"
        "}\n"
        "\n";
    /* $0=sn $1=pn $2=ktn */
    const char FMT2[] =
        "KINLINE void $0_Clr_$1(\n"
        "    $0* self)\n"
        "{\n"
        "    if (self && self->__base.magic == KMAGIC)\n"
        "    {\n"
        "        $2A* field = ($2A*)&self->$1;\n"
        "        $2A_Clr(field);\n"
        "    }\n"
        "}\n"
        "\n";

    if (mpd->array_index == 0)
        put(os, FMT1, sn, pn, ktn, NULL);
    else
        put(os, FMT2, sn, pn, ktn, NULL);
}

string normalize_value_qual_name(const char* name)
{
    string r;

    for (const char* p = name; *p; p++)
    {
        // Skip parentheses:

        if (*p == '(' || *p == ')')
            continue;

        if (isalnum(*p) || *p == '_')
            r += *p;
        else
            r += '_';
    }

    return r;
}

static void gen_enums(
    FILE* os, 
    const MOF_Class_Decl* cd,
    const MOF_Property_Decl* mpd,
    const char* sn)
{
    const char* pn = mpd->name;

    const MOF_Qualifier* values_qual  = 0;
    const MOF_Qualifier* value_map_qual  = 0;

    for (const MOF_Qualifier_Info* q = mpd->all_qualifiers; q; 
        q = (const MOF_Qualifier_Info*)q->next)
    {
        const MOF_Qualifier* qual = q->qualifier;

        if (strcasecmp(qual->name, "Values") == 0)
            values_qual  = qual;
        else if (strcasecmp(qual->name, "ValueMap") == 0)
            value_map_qual  = qual;
    }

    if (values_qual && value_map_qual)
    {
        vector<string> values;
        vector<long> value_map;

        assert(values_qual->params);

        for (MOF_Literal* l = values_qual->params; l; 
            l = (MOF_Literal*)l->next)
        {
            assert(l->value_type == TOK_STRING_VALUE);
            values.push_back(l->string_value);
        }

        for (MOF_Literal* l = value_map_qual->params; l; 
            l = (MOF_Literal*)l->next)
        {
            assert(l->value_type == TOK_STRING_VALUE);
            long tmp = atol(l->string_value);
            value_map.push_back(tmp);
        }

        set<string> names;
        if (values.size())
        {
            put(os, "typedef enum _$0_$1_Enum\n", sn, pn, NULL);
            put(os, "{\n", NULL);

            for (size_t i = 0; i < values.size(); i++)
            {
                const char* vn = values[i].c_str();
                string nvn = normalize_value_qual_name(vn);
                long x = i < value_map.size() ?  value_map[i] : 0;
                char buf[32];
                sprintf(buf, "%ld", x);

                // Name must be unique
                string nvn_ = nvn;
                if (names.find(nvn_) != names.end()) {
                    nvn_ = nvn + "_" + buf;
                }
                names.insert(nvn_);
                nvn = nvn_;

                put(os, "    $0_$1_$3 = $4,\n", 
                    sn, pn, vn, nvn.c_str(), buf, NULL);
            }

            put(os, "}\n", NULL);
            put(os, "$0_$1_Enum;\n\n", sn, pn, NULL);
        }

        // Generate enuemrated setters:

        names.clear();
        for (size_t i = 0; i < values.size(); i++)
        {
            const char* vn = values[i].c_str();
            string nvn = normalize_value_qual_name(vn);

            long x = i < value_map.size() ?  value_map[i] : 0;
            char buf[32];
            sprintf(buf, "%ld", x);

            // Name must be unique
            string nvn_ = nvn;
            if (names.find(nvn_) != names.end()) {
                nvn_ = nvn + "_" + buf;
            }
            names.insert(nvn_);
            nvn = nvn_;

            // $0 = sn
            // $1 = pn
            // $2 = nvn
            // $3 = buf
            const char FMT1[] =
                "/* \"$2\" */\n"
                "#define $0_Set_$1_$3(SELF) \\\n"
                "    $0_Set_$1(SELF, $4)\n\n";
            const char FMT2[] =
                "/* \"$2\" */\n"
                "#define $0_Set_$1_$3(SELF, INDEX)\\\n"
                "    $0_Set_$1(SELF, INDEX, $4)\n\n";

            if (mpd->array_index == 0)
                put(os, FMT1, sn, pn, vn, nvn.c_str(), buf, NULL);
            else
                put(os, FMT2, sn, pn, vn, nvn.c_str(), buf, NULL);
        }
    }
}

static void gen_features(
    FILE* os, 
    const MOF_Class_Decl* cd,
    const char* sn,
    bool ref)
{
    for (MOF_Feature_Info* p = cd->all_features; p; 
        p = (MOF_Feature_Info*)p->next)
    {
        MOF_Feature* mf = p->feature;

        // CIM_Class_Property:

        MOF_Property_Decl* mpd = dynamic_cast<MOF_Property_Decl*>(mf);

        if (ref && !(mf->qual_mask & MOF_QT_KEY))
            continue;

        // ATTN: handle arrays!

        if (mpd)
        {
            gen_array_init(os, cd, mpd, sn);
            gen_set(os, cd, mpd, sn);
            gen_get(os, cd, mpd, sn);
            gen_null(os, cd, mpd, sn);
            gen_clr(os, cd, mpd, sn);
            gen_enums(os, cd, mpd, sn);
            continue;
        }

        // CIM_Class_Reference:

        MOF_Reference_Decl* mrd = dynamic_cast<MOF_Reference_Decl*>(mf);

        if (mrd)
        {
            gen_set_ref(os, cd, mrd, sn);
            gen_null_ref(os, cd, mrd, sn);
            gen_clr_ref(os, cd, mrd, sn);
            continue;
        }

        // CIM_Class_Method:

        MOF_Method_Decl* mmd = dynamic_cast<MOF_Method_Decl*>(mf);

        if (mmd)
        {
            continue;
        }
    }
}

const char INSTANCE_PROVIDER[] =
    "#include <konkret/konkret.h>\n"
    "#include \"<ALIAS>.h\"\n"
    "\n"
    "static const CMPIBroker* _cb = NULL;\n"
    "\n"
    "static void <ALIAS>Initialize()\n"
    "{\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>Cleanup(\n"
    "    CMPIInstanceMI* mi,\n"
    "    const CMPIContext* cc,\n"
    "    CMPIBoolean term)\n"
    "{\n"
    "    CMReturn(CMPI_RC_OK);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>EnumInstanceNames(\n"
    "    CMPIInstanceMI* mi,\n"
    "    const CMPIContext* cc,\n"
    "    const CMPIResult* cr,\n"
    "    const CMPIObjectPath* cop)\n"
    "{\n"
    "    return KDefaultEnumerateInstanceNames(\n"
    "        _cb, mi, cc, cr, cop);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>EnumInstances(\n"
    "    CMPIInstanceMI* mi,\n"
    "    const CMPIContext* cc,\n"
    "    const CMPIResult* cr,\n"
    "    const CMPIObjectPath* cop,\n"
    "    const char** properties)\n"
    "{\n"
    "    CMReturn(CMPI_RC_OK);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>GetInstance(\n"
    "    CMPIInstanceMI* mi,\n"
    "    const CMPIContext* cc,\n"
    "    const CMPIResult* cr,\n"
    "    const CMPIObjectPath* cop,\n"
    "    const char** properties)\n"
    "{\n"
    "    return KDefaultGetInstance(\n"
    "        _cb, mi, cc, cr, cop, properties);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>CreateInstance(\n"
    "    CMPIInstanceMI* mi,\n"
    "    const CMPIContext* cc,\n"
    "    const CMPIResult* cr,\n"
    "    const CMPIObjectPath* cop,\n"
    "    const CMPIInstance* ci)\n"
    "{\n"
    "    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>ModifyInstance(\n"
    "    CMPIInstanceMI* mi,\n"
    "    const CMPIContext* cc,\n"
    "    const CMPIResult* cr,\n"
    "    const CMPIObjectPath* cop,\n"
    "    const CMPIInstance* ci,\n"
    "    const char** properties)\n"
    "{\n"
    "    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>DeleteInstance(\n"
    "    CMPIInstanceMI* mi,\n"
    "    const CMPIContext* cc,\n"
    "    const CMPIResult* cr,\n"
    "    const CMPIObjectPath* cop)\n"
    "{\n"
    "    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>ExecQuery(\n"
    "    CMPIInstanceMI* mi,\n"
    "    const CMPIContext* cc,\n"
    "    const CMPIResult* cr,\n"
    "    const CMPIObjectPath* cop,\n"
    "    const char* lang,\n"
    "    const char* query)\n"
    "{\n"
    "    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);\n"
    "}\n"
    "\n"
    "CMInstanceMIStub(\n"
    "    <ALIAS>,\n"
    "    <CLASS>,\n"
    "    _cb,\n"
    "    <ALIAS>Initialize())\n"
    "\n"
    "static CMPIStatus <ALIAS>MethodCleanup(\n"
    "    CMPIMethodMI* mi,\n"
    "    const CMPIContext* cc,\n"
    "    CMPIBoolean term)\n"
    "{\n"
    "    CMReturn(CMPI_RC_OK);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>InvokeMethod(\n"
    "    CMPIMethodMI* mi,\n"
    "    const CMPIContext* cc,\n"
    "    const CMPIResult* cr,\n"
    "    const CMPIObjectPath* cop,\n"
    "    const char* meth,\n"
    "    const CMPIArgs* in,\n"
    "    CMPIArgs* out)\n"
    "{\n"
    "    return <ALIAS>_DispatchMethod(\n"
    "        _cb, mi, cc, cr, cop, meth, in, out);\n"
    "}\n"
    "\n"
    "CMMethodMIStub(\n"
    "    <ALIAS>,\n"
    "    <CLASS>,\n"
    "    _cb,\n"
    "    <ALIAS>Initialize())\n";

const char ASSOCIATION_PROVIDER[] =
    "#include <konkret/konkret.h>\n"
    "#include \"<ALIAS>.h\"\n"
    "\n"
    "static const CMPIBroker* _cb;\n"
    "\n"
    "static void <ALIAS>Initialize()\n"
    "{\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>Cleanup( \n"
    "    CMPIInstanceMI* mi,\n"
    "    const CMPIContext* cc, \n"
    "    CMPIBoolean term)\n"
    "{\n"
    "    CMReturn(CMPI_RC_OK);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>EnumInstanceNames( \n"
    "    CMPIInstanceMI* mi,\n"
    "    const CMPIContext* cc,\n"
    "    const CMPIResult* cr,\n"
    "    const CMPIObjectPath* cop)\n"
    "{\n"
    "    return KDefaultEnumerateInstanceNames(\n"
    "        _cb, mi, cc, cr, cop);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>EnumInstances( \n"
    "    CMPIInstanceMI* mi,\n"
    "    const CMPIContext* cc, \n"
    "    const CMPIResult* cr, \n"
    "    const CMPIObjectPath* cop, \n"
    "    const char** properties) \n"
    "{\n"
    "    CMReturn(CMPI_RC_OK);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>GetInstance( \n"
    "    CMPIInstanceMI* mi, \n"
    "    const CMPIContext* cc,\n"
    "    const CMPIResult* cr, \n"
    "    const CMPIObjectPath* cop, \n"
    "    const char** properties) \n"
    "{\n"
    "    return KDefaultGetInstance(\n"
    "        _cb, mi, cc, cr, cop, properties);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>CreateInstance( \n"
    "    CMPIInstanceMI* mi, \n"
    "    const CMPIContext* cc, \n"
    "    const CMPIResult* cr, \n"
    "    const CMPIObjectPath* cop, \n"
    "    const CMPIInstance* ci) \n"
    "{\n"
    "    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>ModifyInstance( \n"
    "    CMPIInstanceMI* mi, \n"
    "    const CMPIContext* cc, \n"
    "    const CMPIResult* cr, \n"
    "    const CMPIObjectPath* cop,\n"
    "    const CMPIInstance* ci, \n"
    "    const char**properties) \n"
    "{\n"
    "    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>DeleteInstance( \n"
    "    CMPIInstanceMI* mi, \n"
    "    const CMPIContext* cc, \n"
    "    const CMPIResult* cr, \n"
    "    const CMPIObjectPath* cop) \n"
    "{\n"
    "    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>ExecQuery(\n"
    "    CMPIInstanceMI* mi, \n"
    "    const CMPIContext* cc, \n"
    "    const CMPIResult* cr, \n"
    "    const CMPIObjectPath* cop, \n"
    "    const char* lang, \n"
    "    const char* query) \n"
    "{\n"
    "    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>AssociationCleanup( \n"
    "    CMPIAssociationMI* mi,\n"
    "    const CMPIContext* cc, \n"
    "    CMPIBoolean term) \n"
    "{\n"
    "    CMReturn(CMPI_RC_OK);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>Associators(\n"
    "    CMPIAssociationMI* mi,\n"
    "    const CMPIContext* cc,\n"
    "    const CMPIResult* cr,\n"
    "    const CMPIObjectPath* cop,\n"
    "    const char* assocClass,\n"
    "    const char* resultClass,\n"
    "    const char* role,\n"
    "    const char* resultRole,\n"
    "    const char** properties)\n"
    "{\n"
    "    return KDefaultAssociators(\n"
    "        _cb,\n"
    "        mi,\n"
    "        cc,\n"
    "        cr,\n"
    "        cop,\n"
    "        <ALIAS>_ClassName,\n"
    "        assocClass,\n"
    "        resultClass,\n"
    "        role,\n"
    "        resultRole,\n"
    "        properties);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>AssociatorNames(\n"
    "    CMPIAssociationMI* mi,\n"
    "    const CMPIContext* cc,\n"
    "    const CMPIResult* cr,\n"
    "    const CMPIObjectPath* cop,\n"
    "    const char* assocClass,\n"
    "    const char* resultClass,\n"
    "    const char* role,\n"
    "    const char* resultRole)\n"
    "{\n"
    "    return KDefaultAssociatorNames(\n"
    "        _cb,\n"
    "        mi,\n"
    "        cc,\n"
    "        cr,\n"
    "        cop,\n"
    "        <ALIAS>_ClassName,\n"
    "        assocClass,\n"
    "        resultClass,\n"
    "        role,\n"
    "        resultRole);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>References(\n"
    "    CMPIAssociationMI* mi,\n"
    "    const CMPIContext* cc,\n"
    "    const CMPIResult* cr,\n"
    "    const CMPIObjectPath* cop,\n"
    "    const char* assocClass,\n"
    "    const char* role,\n"
    "    const char** properties)\n"
    "{\n"
    "    return KDefaultReferences(\n"
    "        _cb,\n"
    "        mi,\n"
    "        cc,\n"
    "        cr,\n"
    "        cop,\n"
    "        <ALIAS>_ClassName,\n"
    "        assocClass,\n"
    "        role,\n"
    "        properties);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>ReferenceNames(\n"
    "    CMPIAssociationMI* mi,\n"
    "    const CMPIContext* cc,\n"
    "    const CMPIResult* cr,\n"
    "    const CMPIObjectPath* cop,\n"
    "    const char* assocClass,\n"
    "    const char* role)\n"
    "{\n"
    "    return KDefaultReferenceNames(\n"
    "        _cb,\n"
    "        mi,\n"
    "        cc,\n"
    "        cr,\n"
    "        cop,\n"
    "        <ALIAS>_ClassName,\n"
    "        assocClass,\n"
    "        role);\n"
    "}\n"
    "\n"
    "CMInstanceMIStub( \n"
    "    <ALIAS>,\n"
    "    <CLASS>,\n"
    "    _cb,\n"
    "    <ALIAS>Initialize())\n"
    "\n"
    "CMAssociationMIStub( \n"
    "    <ALIAS>,\n"
    "    <CLASS>,\n"
    "    _cb,\n"
    "    <ALIAS>Initialize())\n";

const char INDICATION_PROVIDER[] =
    "#include <konkret/konkret.h>\n"
    "#include \"<ALIAS>.h\"\n"
    "\n"
    "static const CMPIBroker* _cb = NULL;\n"
    "\n"
    "static void <ALIAS>Initialize()\n"
    "{\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>IndicationCleanup(\n"
    "    CMPIIndicationMI* mi,\n"
    "    const CMPIContext* cc,\n"
    "    CMPIBoolean term)\n"
    "{\n"
    "    CMReturn(CMPI_RC_OK);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>AuthorizeFilter(\n"
    "    CMPIIndicationMI* mi,\n"
    "    const CMPIContext* cc,\n"
    "    const CMPISelectExp* se,\n"
    "    const char* ns,\n"
    "    const CMPIObjectPath* op,\n"
    "    const char* user)\n"
    "{\n"
    "    CMReturn(CMPI_RC_OK);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>MustPoll(\n"
    "    CMPIIndicationMI* mi,\n"
    "    const CMPIContext* cc,\n"
    "    const CMPISelectExp* se,\n"
    "    const char* ns, \n"
    "    const CMPIObjectPath* op)\n"
    "{\n"
    "    CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>ActivateFilter(\n"
    "    CMPIIndicationMI* mi,\n"
    "    const CMPIContext* cc,\n"
    "    const CMPISelectExp* se,\n"
    "    const char* ns,\n"
    "    const CMPIObjectPath* op,\n"
    "    CMPIBoolean firstActivation)\n"
    "{\n"
    "    CMReturn(CMPI_RC_OK);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>DeActivateFilter(\n"
    "    CMPIIndicationMI* mi,\n"
    "    const CMPIContext* cc,\n"
    "    const CMPISelectExp* se,\n"
    "    const char* ns,\n"
    "    const CMPIObjectPath* op,\n"
    "    CMPIBoolean lastActivation)\n"
    "{\n"
    "    CMReturn(CMPI_RC_OK);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>EnableIndications(\n"
    "    CMPIIndicationMI* mi, \n"
    "    const CMPIContext* cc)\n"
    "{\n"
    "    CMReturn(CMPI_RC_OK);\n"
    "}\n"
    "\n"
    "static CMPIStatus <ALIAS>DisableIndications(\n"
    "    CMPIIndicationMI* mi, \n"
    "    const CMPIContext* cc)\n"
    "{\n"
    "    CMReturn(CMPI_RC_OK);\n"
    "}\n"
    "\n"
    "CMIndicationMIStub(\n"
    "    <ALIAS>,\n"
    "    <CLASS>, \n"
    "    _cb, \n"
    "    <ALIAS>Initialize())\n";


static void expropers_decls(string &chunk, map<string,string> prules, const MOF_Class_Decl* cd)
{
    string append;

    printf("Properties of %s\n", cd->name);
    for (MOF_Feature_Info* p = cd->all_features; p;
        p = (MOF_Feature_Info*)p->next)
    {
        MOF_Feature* mf = p->feature;

        if (strcasecmp(cd->name, p->class_origin->name) != 0)
            continue;

        // CIM_Class_Property:

        MOF_Property_Decl* pd = dynamic_cast<MOF_Property_Decl*>(mf);

        if (pd)
        {
            const char* pn = pd->name;
            const char* ktn = _ktype_name(pd->data_type);
            
            string type;
            if (pd->array_index == 0) {
                // is not array
                type = string(_ktype_name(pd->data_type));
            } else {
                // is array
                type = string(_ktype_name(pd->data_type))+string("A");
            }

            // XXX may need to extend by property name, class name, subclass name, superclass name rules
            printf("Lookup %s\n", type.c_str());
            for (map<string,string>::iterator pi = prules.begin(); pi!=prules.end(); pi++) {
                printf("  among %s\n", (*pi).first.c_str());
            }
            if (prules.find(type) != prules.end()) {
                // type has mapping rule
                append = prules[type];
                printf("\nTo be appended %s\n", append.c_str());
                substitute(append, "<PTYPE>", type);
                substitute(append, "<PNAME>", pd->name);
                printf("\nTransformed appendix %s\n", append.c_str());
                chunk+=append;
                printf("\nMerged chunk %s\n", chunk.c_str());
            }

            if (pd->array_index == 0) {
                printf("    const %s %s;\n", ktn, pn);
            } else {
                printf("    const %sA %s;\n", ktn, pn);
           }
        }
    }
    return;
}

static void expropers_recursive(string &chunk, const map<string,string> prules, const MOF_Class_Decl* cd)
{
    if (cd->super_class) {
        expropers_recursive(chunk, prules, cd->super_class);
        printf("Nested chunk =======\n%s=======\n", chunk.c_str());
    }
    expropers_decls(chunk, prules, cd);
    return;
}

static void expropers(string &text, const pair<string,map<string,string> > pset, const MOF_Class_Decl* cd)
{
    printf("Property pattern %s\n",pset.first.c_str());
    string chunk;
    expropers_recursive(chunk, pset.second, cd);
    printf("Finished properties ==========\n%s==========\n", chunk.c_str());
    substitute(text, pset.first, chunk);
    return;
}

static void exreplace(string &text, const pair<string,string> rrule)
{
    ifstream rchunk(rrule.second.c_str(), ios::in|ios::ate|ios::binary);
    if (!rchunk) {
        err("problem opening file %s", rrule.first.c_str());
    }
    streampos length = rchunk.tellg();
    rchunk.seekg(0,ios::beg);

    string templ;
    templ.reserve(length);
    templ.assign(istreambuf_iterator<char>(rchunk),istreambuf_iterator<char>());

    rchunk.close();
    substitute(text, rrule.first.c_str(), templ);
    return;
}

static void transform(string &text, const MOF_Class_Decl* cd)
{
    
    vector<pair<string,string> >::iterator ri = rall.begin();
    vector<pair<string,map<string,string> > >::iterator pi = pall.begin();
    for (vector<char>::iterator t = torder.begin(); t != torder.end(); t++) {
        switch (*t) {
        case 'R':
            printf("%s replaced for %s\n", (*ri).first.c_str(), (*ri).second.c_str());
            exreplace(text, *ri);
            ri++;
        break;
        case 'P':
            printf("%s transformed to properties.\n", (*pi).first.c_str());
            expropers(text, *pi, cd);
            pi++;
        break;
        }
    }
    return;
}

static void gen_provider(const MOF_Class_Decl* cd)
{
    const char* sn = alias(cd->name);
    FILE* os;

    // Form source file name

    char path[1024];
    if (ofile.length()<=0) {
        sprintf(path, "%sProvider.c", sn);
    } else {
        sprintf(path, "%s", ofile.c_str());
    }

    if ((os = fopen(path, "r")))
    {
        fclose(os);
        printf("Skipped %s (already exists)\n", path);
        return;
    }

    // Open stubs source file:

    if (!(os = fopen(path, "w")))
    {
        err("failed to open %s for write", path);
    }

    // Generate provider source file.

    if (cd->qual_mask & MOF_QT_ASSOCIATION)
    {
        // Association provider:
        string text;
        if (eta.empty())
        {
            text = ASSOCIATION_PROVIDER;
        } else {
            text = eta;
        }
        transform(text, cd);
        substitute(text, "<ALIAS>", sn);
        substitute(text, "<CLASS>", cd->name);
        fprintf(os, "%s", text.c_str());
    }
    else if (cd->qual_mask & MOF_QT_INDICATION)
    {
        // Indication provider:
        string text;
        if (etn.empty())
        {
            text = INDICATION_PROVIDER;
        } else {
            text = etn;
        }
        transform(text, cd);
        substitute(text, "<ALIAS>", sn);
        substitute(text, "<CLASS>", cd->name);
        fprintf(os, "%s", text.c_str());
    }
    else
    {
        // Instance provider:
        string text;
        if (eti.empty())
        {
            text = INSTANCE_PROVIDER;
        } else {
            text = eti;
        }
        transform(text, cd);
        substitute(text, "<ALIAS>", sn);
        substitute(text, "<CLASS>", cd->name);
        fprintf(os, "%s", text.c_str());
    }

    // Generate method stubs.

    fprintf(os, "\n");
    gen_meth_stubs(os, cd);

    // Write out KONKRET_REGISTRATION() macro:

    string provider_types;

    if (cd->qual_mask & MOF_QT_ASSOCIATION)
        provider_types = "instance association";
    else if (cd->qual_mask & MOF_QT_INDICATION)
        provider_types = "indication";
    else
        provider_types = "instance method";

    fprintf(os, "KONKRET_REGISTRATION(\n");
    fprintf(os, "    \"%s\",\n", "root/cimv2");
    fprintf(os, "    \"%s\",\n", cd->name);
    fprintf(os, "    \"%s\",\n", cd->name);
    fprintf(os, "    \"%s\")\n", provider_types.c_str());

    // Close the file:
    fclose(os);
    printf("Created %s\n", path);
}

static void gen1(const MOF_Class_Decl* cd, const char* al)
{
    char path[1024];
    char rn[1024];
    char cn[1024];

    sprintf(rn, "%sRef", al);
    sprintf(cn, "%s", al);
    sprintf(path, "%s.h", al);

    // Generate the structure names for the class and reference:

    // Open output file

    FILE* os = fopen(path, "wb");

    if (!os)
        err("failed to open %s", path);

    // Generate comment box:

    const char BOX[] =
        "/*\n"
        "**$0$0\n"
        "**\n"
        "** CAUTION: This file generated by KonkretCMPI. Please do not edit.\n"
        "**\n"
        "**$0$0\n"
        "*/\n"
        "\n";

    put(os, BOX, LINE39, NULL);

    // Header:

    const char HEADER[] =
        "#ifndef _konkrete_%s_h\n"
        "#define _konkrete_%s_h\n"
        "\n"
        "#include <konkret/konkret.h>\n"
        "\n"
        "#include <strings.h>\n";

    fprintf(os, HEADER, cn, cn);

    vector<string> deps;
    direct_dependencies(cd->name, deps);

    for (size_t i = 0; i < deps.size(); i++)
    {
        fprintf(os, "#include \"%s.h\"\n", alias(deps[i].c_str()));
    }

    fprintf(os, "\n");

    // Generate reference:

    gen_class(os, cd, rn, true);
    gen_init(os, cd, rn, true);
    gen_print(os, cd, rn, true);
    gen_instance(os, cd, rn, true);
    gen_object_path(os, cd, rn, true);
    gen_ns(os, rn);
    gen_features(os, cd, rn, true);

    // Generate class:

    gen_class(os, cd, cn, false);
    gen_init(os, cd, cn, false);
    gen_print(os, cd, cn, false);
    gen_instance(os, cd, cn, false);
    gen_object_path(os, cd, cn, false);
    gen_ns(os, cn);
    gen_features(os, cd, cn, false);

    // Generate methods:
    gen_methods(os, cd, cn);

    // Generate classname macro:

    fprintf(os, "#define %s_ClassName \"%s\"\n\n", al, cd->name);

    // Trailer:

    const char TRAILER[] =
        "#endif /* _konkrete_%s_h */\n";

    fprintf(os, TRAILER, cn);

    // Close file:

    fclose(os);

    printf("Created %s\n", path);

    if (contains(skeletons, cd->name))
        gen_provider(cd);
}

static void gen()
{
    for (size_t i = 0; i < classnames.size(); i++)
    {
        const MOF_Class_Decl* cd;
        string cn = classnames[i];

        if (!(cd = _find_class(cn.c_str())))
            err("unknown class: %s", cn.c_str());

        gen1(cd, aliases[i].c_str());
    }
}

static int _find_schema_mof(const char* path, string& schema_mof)
{
    schema_mof.erase(schema_mof.begin(), schema_mof.end());

    // Search for a file of the form cimv[0-9]*.mof in this directory.

    DIR* dir = opendir(path);

    if (!dir)
        return -1;

    struct dirent* ent;

    while ((ent = readdir(dir)))
    {
        const char* name = ent->d_name;

        if (strncmp(name, "cimv", 4) == 0)
        {
            const char* p = name + 4;

            while (isdigit(*p))
                p++;

            if (strcmp(p, ".mof") == 0)
            {
                struct stat st;

                string tmp = string(path) + string("/") + string(name);

                if (stat(tmp.c_str(), &st) == 0)
                {
                    schema_mof = tmp;
                    closedir(dir);
                    return 0;
                }
            }
        }
    }

    // Look for file called CIM_Schema.mof.

    {
        string tmp = string(path) + string("/CIM_Schema.mof");
        struct stat st;

        if (stat(tmp.c_str(), &st) == 0)
        {
            schema_mof = tmp;
            return 0;
        }
    }

    closedir(dir);

    // Not found!
    return -1;
}

string extemplate(const char *filename)
{
    ifstream pt(filename, ios::in|ios::ate|ios::binary);
    if (!pt) {
        err("problem opening file %s", filename);
    }
    streampos length = pt.tellg();
    pt.seekg(0,ios::beg);

    string templ;
    templ.reserve(length);
    templ.assign(istreambuf_iterator<char>(pt),istreambuf_iterator<char>());

    pt.close();
    return(templ);
}

int main(int argc, char** argv)
{
    arg0 = argv[0];
    const char USAGE[] =
        "Usage: %s [OPTIONS] CLASS=ALIAS[!]...\n"
        "\n"
        "OPTIONS:\n"
        "  -P STR=FILE Replace STR with properties. With property rules in FILE\n"
        "  -R STR=FILE Replace STR for contents of FILE\n"
        "  -M FILE     Method argument type processing rules in FILE\n"
        "  -I DIR      Search for included MOF files in this directory\n"
        "  -m FILE     Add MOF file to list of MOFs to parse\n"
        "  -v          Print the version\n"
        "  -s CLASS    Write provider skeleton for CLASS to <ALIAS>Provider.c\n"
        "              (or use CLASS=ALIAS! form instead).\n"
        "  -o FILE     Write main skeleton to custom FILE.\n"
        "  -f FILE     Read CLASS=ALIAS[!] argumetns the given file.\n"
        "  -h          Print this help message\n"
        "  -a FILE     Template for association provider\n"
        "  -c FILE     Template for class instance provider\n"
        "  -n FILE     Template for indication provider\n"
        "\n"
        "ENVIRONMENT VARIABLES:\n"
        "  KONKRET_SCHEMA_DIR -- searched for schema MOF files\n"
        "\n";
    string schema_mof;
    vector<string> mofs;

    // Turn on MOF warnings.

    MOF_Options::warn = true;

    // Add KONKRET_SCHEMA_DIR to MOF path if defined

    bool use_cimschema = false;
    const char* schema_dir = getenv("KONKRET_SCHEMA_DIR");

#if defined(CIMSCHEMA)
    if (!schema_dir)
    {
        schema_dir = CIMSCHEMA;
        use_cimschema = true;
    }
#endif

    if (schema_dir)
    {
        struct stat st;

        if (stat(schema_dir, &st) != 0 || !(st.st_mode & S_IFDIR))
        {
            if (use_cimschema)
            {
                err("The directory given by --with-cim-schema=%s during "
                    "package configuration does not exist so please use "
                    "the KONKRET_SCHEMA_DIR environment variable to "
                    "locate the CIM schema.", schema_dir);
            }
            else
            {
                err("The directory given by the KONKRET_SCHEMA_DIR "
                    "environment variable does not exist: %s", schema_dir);
            }
        }

        MOF_include_paths[MOF_num_include_paths++] = schema_dir;

        if (_find_schema_mof(schema_dir, schema_mof) != 0)
        {
            if (use_cimschema)
            {
                err("Cannot locate master schema MOF file under directory "
                    "given by --with-cim-schema=%s during package "
                    "configuration. Please use the KONKRET_SCHEMA_DIR "
                    "environment variable to locate the CIM schema.",
                    schema_dir);
            }
            else
            {
                err("Cannot locate master schema MOF file under "
                    "KONKRET_SCHEMA_DIR=%s", schema_dir);
            }
        }

        mofs.push_back(schema_mof);
    }

    // Process command-line options.

    vector<string> args;

    for (int opt; (opt = getopt(argc, argv, "P:R:I:m:vhs:pf:a:c:n:o:kM:")) != -1; )
    {
        switch (opt)
        {
            case 'o':
            {
                ofile = string(optarg);
                break;
            }
            case 'P':
            {
                torder.push_back('P');
                string replace;
                replace.assign(optarg);
                
                string token = replace.substr(0, replace.find('='));
                string pfilename = replace.substr(replace.find('=') + 1);

                if (pfilename.size() == 0)
                {
                    err("Invalid -P option %s. Use -P PATTERN=FILENAME", optarg);
                }

                ifstream pmap(pfilename.c_str(), ios::in|ios::binary);
                if (!pmap) {
                    err("Property replacement file %s missing or unreadable.", pfilename.c_str());
                } else {
                    printf("Processing %s\n", pfilename.c_str());
                }

                string line;
                map <string,string> tlist;
                while (getline(pmap, line)) {
                        printf("-P line: %s\n", line.c_str());
                        string ptype = line.substr(0, line.find('='));
                        string pfile = line.substr(line.find('=') + 1);

                        ifstream codefile(pfile.c_str(), ios::in|ios::ate|ios::binary);
                        if (!codefile) {
                                err("Property type %s replacement file %s missing or unreadable.", ptype.c_str(), pfile.c_str());
                        }
                        codefile.close();
                        string pcode = extemplate(pfile.c_str());
                        tlist[ptype] = pcode;
                        printf("Property type %s for %s\n", ptype.c_str(), pcode.c_str());
                }
                pmap.close();

                pall.push_back(pair<string,map<string, string> >(token, tlist));

                break;
            }

            case 'R':
            {
                torder.push_back('R');
                string replace;
                replace.assign(optarg);
                
                string tmpstr = replace.substr(0, replace.find('='));
                string tmpfile = replace.substr(replace.find('=') + 1);

                if (tmpfile.size() == 0)
                {
                    err("Invalid -P option %s. Use -P PATTERN=FILENAME", optarg);
                }

                ifstream ifile(tmpfile.c_str(), ios::in|ios::ate|ios::binary);
                if (!ifile) {
                    err("Replacement file %s missing or unreadable.", tmpfile.c_str());
                }
                ifile.close();

                rall.push_back(pair<string,string>(tmpstr, tmpfile));
                
                break;
            }

            case 'M':
            {
                string mfilename;
                mfilename.assign(optarg);
                
                if (mfilename.size() == 0)
                {
                    err("Invalid -M option %s. Use -M FILENAME", optarg);
                }

                ifstream imamap(mfilename.c_str(), ios::in|ios::binary);
                if (!imamap) {
                    err("Method argument replacement file %s missing or unreadable.", mfilename.c_str());
                } else {
                    printf("Processing %s\n", mfilename.c_str());
                }

                string line;
                map <string,string> tlist;
                while (getline(imamap, line)) {
                        printf("-M line: %s\n", line.c_str());
                        string mtype = line.substr(0, line.find('='));
                        string mfile = line.substr(line.find('=') + 1);

                        ifstream codefile(mfile.c_str(), ios::in|ios::ate|ios::binary);
                        if (!codefile) {
                                err("Method argument type %s replacement file %s missing or unreadable.", mtype.c_str(), mfile.c_str());
                        }
                        codefile.close();
                        string mcode = extemplate(mfile.c_str());
                        tlist[mtype] = mcode;
                        printf("Method argument type %s for %s\n", mtype.c_str(), mcode.c_str());
                }
                imamap.close();

                mall.push_back(map<string, string>(tlist));

                break;
            }

            case 'I':
            {
                if (MOF_num_include_paths == MAX_INCLUDES)
                {
                    err("too many -I options");
                    exit(1);
                }

                MOF_include_paths[MOF_num_include_paths++] = optarg;
                break;
            }

            case 'm':
                mofs.push_back(optarg);
                break;

            case 'h':
                printf((char*)USAGE, arg0);
                exit(0);
                break;

            case 'v':
                printf("%s\n", KONKRET_VERSION_STRING);
                exit(0);
                break;

            case 's':
                skeletons.push_back(optarg);
                break;

            case 'f':
            {
                FILE* is = fopen(optarg, "r");
                char buf[1024];

                if (!is)
                    err("failed to open %s", optarg);

                while (fgets(buf, sizeof(buf), is) != NULL)
                {
                    // Skip leading whitespace.

                    const char* p = buf;

                    while (isspace(*p))
                        p++;

                    // Skip trailing whitespace.

                    char* q = buf + strlen(buf);

                    while (q != buf && isspace(q[-1]))
                        *--q = '\0';

                    // Skip blank and comment lines.

                    if (*p == '\0' || *p == '#')
                        continue;

                    args.push_back(buf);
                }

                fclose(is);
                break;
            }
            case 'a':
                eta = extemplate(optarg);
                break;
            case 'c':
                eti = extemplate(optarg);
                break;
            case 'n':
                etn = extemplate(optarg);
                break; 
            case 'k':
                around = true;
                break;

            default:
                err("invalid option: %c; try -h for help", opt);
                break;
        }
    }

    if (args.size() == 0 && optind == argc)
        err("insufficient command line arguments. Try -h for help");

    // Print using message:

    printf("Using: %s\n", schema_mof.c_str());

    // Populate args vector.

    for (int i = optind; i < argc; i++)
        args.push_back(argv[i]);

    // Gather classnames and optional aliases.

    for (size_t i = 0; i < args.size(); i++)
    {
        string arg = args[i];
        string cn = arg.substr(0, arg.find('='));
        string al = arg.substr(arg.find('=') + 1);

        if (al.size() == 0)
            al = cn;

        if (al.size() && al[al.size()-1] == '!')
        {
            al = al.substr(0, al.size() - 1);
            skeletons.push_back(cn);
        }

        classnames.push_back(cn);
        aliases.push_back(al);
    }

    // There must be at least one MOF file.

    if (mofs.size() == 0)
        err("no MOF files to parse. Try -h for help");

    // Parse all the MOF files:

    for (size_t i = 0; i < mofs.size(); i++)
    {
        MOF_parse_file(mofs[i].c_str());
    }

    // Calculate dependencies (updating classnames and aliases).
    {
        vector<string> tmp = classnames;

        for (size_t i = 0; i < tmp.size(); i++)
        {
            vector<string> deps;
            recursive_dependencies(tmp[i], deps);

            for (size_t j = 0; j < deps.size(); j++)
            {
                if (!contains(classnames, deps[j]))
                {
                    classnames.push_back(deps[j]);
                    aliases.push_back(deps[j]);
                }
            }
        }
    }

    // Check that classes will be generated for each skeleton.

    for (size_t i = 0; i < skeletons.size(); i++)
    {
        if (!contains(classnames, skeletons[i]))
        {
            err("Class given by -s option (%s) must also appear on the command "
                "line on the class list. For example:\n\n"
                "    $ genclass -s CLASS CLASS=ALIAS");
        }
    }

    // Write files:

    gen();

    return 0;
}
