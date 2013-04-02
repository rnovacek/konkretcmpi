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

#include "konkret.h"

#include <strings.h>
/*
**==============================================================================
**
** KString
**
**==============================================================================
*/

void KString_SetString(KString* self, CMPIString* value)
{
    if (self && value)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->value = value;
        self->chars = KChars(value);
    }
}

CMPIStatus KString_Set(
    KString* self, 
    const CMPIBroker* cb, 
    const char* s)
{
    if (self && cb && s)
    {
        CMPIString* value;
        CMPIStatus st = KSTATUS_INIT;
        
        if ((value = CMNewString(cb, s, &st)) && KOkay(st))
        {
            memset(self, 0, sizeof(*self));
            self->exists = 1;
            self->value = value;
            self->chars = KChars(value);
            KReturn(OK);
        }
    }

    KReturn(ERR_FAILED);
}

/*
**==============================================================================
**
** KBase
**
**==============================================================================
*/

void KBase_Init(
    KBase* self,
    const CMPIBroker* cb,
    size_t size,
    const unsigned char* sig,
    const char* ns)
{
    memset(self, 0, size);
    self->magic = KMAGIC;
    self->size = size;
    self->sig = sig;
    self->cb = cb;
    self->ns = KNewString(cb, ns);
}

static const size_t _type_sizes[] =
{
    sizeof(KBoolean),
    sizeof(KUint8),
    sizeof(KSint8),
    sizeof(KUint16),
    sizeof(KSint16),
    sizeof(KUint32),
    sizeof(KSint32),
    sizeof(KUint64),
    sizeof(KSint64),
    sizeof(KReal32),
    sizeof(KReal64),
    sizeof(KChar16),
    sizeof(KString),
    sizeof(KDateTime),
    sizeof(KRef),
    sizeof(KInstance),
};

size_t KTypeSize(KTag tag)
{
    if ((tag & KTAG_ARRAY))
        return sizeof(KArray);
    else
        return _type_sizes[KTypeOf(tag)];
}

static KTag _cmpitype_to_ktag(CMPIType type)
{
    switch (type)
    {
        case CMPI_boolean:
            return KTYPE_BOOLEAN;
        case CMPI_uint8:
            return KTYPE_UINT8;
        case CMPI_sint8:
            return KTYPE_SINT8;
        case CMPI_uint16:
            return KTYPE_UINT16;
        case CMPI_sint16:
            return KTYPE_SINT16;
        case CMPI_uint32:
            return KTYPE_UINT32;
        case CMPI_sint32:
            return KTYPE_SINT32;
        case CMPI_uint64:
            return KTYPE_UINT64;
        case CMPI_sint64:
            return KTYPE_SINT64;
        case CMPI_real32:
            return KTYPE_REAL32;
        case CMPI_real64:
            return KTYPE_REAL64;
        case CMPI_char16:
            return KTYPE_CHAR16;
        case CMPI_string:
            return KTYPE_STRING;
        case CMPI_dateTime:
            return KTYPE_DATETIME;
        case CMPI_ref:
            return KTYPE_REFERENCE;
        case CMPI_instance:
            return KTYPE_INSTANCE;
        case CMPI_booleanA:
            return KTAG_ARRAY | KTYPE_BOOLEAN;
        case CMPI_uint8A:
            return KTAG_ARRAY | KTYPE_UINT8;
        case CMPI_sint8A:
            return KTAG_ARRAY | KTYPE_SINT8;
        case CMPI_uint16A:
            return KTAG_ARRAY | KTYPE_UINT16;
        case CMPI_sint16A:
            return KTAG_ARRAY | KTYPE_SINT16;
        case CMPI_uint32A:
            return KTAG_ARRAY | KTYPE_UINT32;
        case CMPI_sint32A:
            return KTAG_ARRAY | KTYPE_SINT32;
        case CMPI_uint64A:
            return KTAG_ARRAY | KTYPE_UINT64;
        case CMPI_sint64A:
            return KTAG_ARRAY | KTYPE_SINT64;
        case CMPI_real32A:
            return KTAG_ARRAY | KTYPE_REAL32;
        case CMPI_real64A:
            return KTAG_ARRAY | KTYPE_REAL64;
        case CMPI_char16A:
            return KTAG_ARRAY | KTYPE_CHAR16;
        case CMPI_stringA:
            return KTAG_ARRAY | KTYPE_STRING;
        case CMPI_dateTimeA:
            return KTAG_ARRAY | KTYPE_DATETIME;
        case CMPI_refA:
            return KTAG_ARRAY | KTYPE_REFERENCE;
        case CMPI_instanceA:
            return KTAG_ARRAY | KTYPE_INSTANCE;
    }

    return 0xFF;
}

static CMPIData _data(const KValue* value, KTag tag)
{
    CMPIData cd;

    if (value->null)
    {
        cd.state = CMPI_nullValue;
        cd.value.uint64 = 0;
    }
    else
    {
        cd.state = 0;
        cd.value.uint64 = value->u.uint64;
    }

    /* Set type */

    switch (KTypeOf(tag))
    {
        case KTYPE_BOOLEAN:
            cd.type = CMPI_boolean;
            break;
        case KTYPE_UINT8:
            cd.type = CMPI_uint8;
            break;
        case KTYPE_SINT8:
            cd.type = CMPI_sint8;
            break;
        case KTYPE_UINT16:
            cd.type = CMPI_uint16;
            break;
        case KTYPE_SINT16:
            cd.type = CMPI_sint16;
            break;
        case KTYPE_UINT32:
            cd.type = CMPI_uint32;
            break;
        case KTYPE_SINT32:
            cd.type = CMPI_sint32;
            break;
        case KTYPE_UINT64:
            cd.type = CMPI_uint64;
            break;
        case KTYPE_SINT64:
            cd.type = CMPI_sint64;
            break;
        case KTYPE_REAL32:
            cd.type = CMPI_real32;
            break;
        case KTYPE_REAL64:
            cd.type = CMPI_real64;
            break;
        case KTYPE_CHAR16:
            cd.type = CMPI_char16;
            break;
        case KTYPE_STRING:
            cd.type = CMPI_string;
            break;
        case KTYPE_DATETIME:
            cd.type = CMPI_dateTime;
            break;
        case KTYPE_REFERENCE:
            cd.type = CMPI_ref;
            break;
        case KTYPE_INSTANCE:
            cd.type = CMPI_instance;
            break;
        default:
            cd.type = 0;
    }

    if ((tag & KTAG_ARRAY))
        cd.type |= CMPI_ARRAY;

    return cd;
}

CMPIObjectPath* KBase_ToObjectPath(
    const KBase* self, 
    CMPIStatus* st)
{
    KPos pos;
    CMPIObjectPath* cop;

    /* Check parameters */

    if (self->magic != KMAGIC)
    {
        KSetStatus(st, ERR_FAILED);
        return NULL;
    }

    /* Create object path */

    KFirst(&pos, self);

    if (!(cop = CMNewObjectPath(self->cb, KChars(self->ns), pos.classname, 
        st)))
    {
        return NULL;
    }

    /* Set keys */

    while (KMore(&pos))
    {
        CMPIData cd;
        const KValue* value = (const KValue*)pos.field;

        if (value->exists && (pos.tag & KTAG_KEY))
        {
            CMPIStatus st;

            cd = _data(value, pos.tag);

            if (value->null)
                st = CMAddKey(cop, pos.name, NULL, cd.type);
            else
                st = CMAddKey(cop, pos.name, &cd.value, cd.type);

            if (!KOkay(st))
            {
                /* ATTN: log this but do not return! */
            }
        }

        KNext(&pos);
    }

    return cop;
}

CMPIInstance* KBase_ToInstance(
    const KBase* self, 
    CMPIStatus* st)
{
    KPos pos;
    CMPIObjectPath* cop;
    CMPIInstance* ci;

    /* Check parameters */

    if (self->magic != KMAGIC)
    {
        KSetStatus(st, ERR_FAILED);
        return NULL;
    }

    if (!self->ns)
    {
        /* Ignore */
    }

    /* Create object path */

    if (!(cop = KBase_ToObjectPath(self, st)))
    {
        return NULL;
    }

    /* Create instance */

    if (!(ci = CMNewInstance(self->cb, cop, st)))
        return NULL;

    /* Set properties */

    KFirst(&pos, self);

    while (KMore(&pos))
    {
        CMPIData cd;
        const KValue* value = (const KValue*)pos.field;

        if (value->exists)
        {
            CMPIStatus st;

            cd = _data(value, pos.tag);

            if (value->null)
                st = CMSetProperty(ci, pos.name, NULL, cd.type);
            else
                st = CMSetProperty(ci, pos.name, &cd.value, cd.type);

            if (!KOkay(st))
            {
                /* ATTN: log this but do not return! */
            }
        }

        KNext(&pos);
    }

    return ci;
}

CMPIStatus KBase_SetToArgs(
    const KBase* self, 
    CMPIBoolean in,
    CMPIBoolean out,
    CMPIArgs* ca)
{
    KPos pos;

    /* Check parameters */

    if (!ca || !self || self->magic != KMAGIC)
    {
        KReturn(ERR_FAILED);
    }

    /* Set properties */

    KFirst(&pos, self);

    while (KMore(&pos))
    {
        CMPIData cd;
        const KValue* value = (const KValue*)pos.field;

        do
        {
            CMPIStatus st;

            if (!value->exists)
                break;

            cd = _data(value, pos.tag);

            if (in && !(pos.tag & KTAG_IN))
                break;

            if (out && !(pos.tag & KTAG_OUT))
                break;

            if (value->null)
                st = CMAddArg(ca, pos.name, NULL, cd.type);
            else
                st = CMAddArg(ca, pos.name, &cd.value, cd.type);

            if (!KOkay(st))
            {
                printf("%s() failed on %s\n", __FUNCTION__, pos.name);
            }
        }
        while (0);

        KNext(&pos);
    }

    KReturn(OK);
}

CMPIArgs* KBase_ToArgs(
    const KBase* self, 
    CMPIBoolean in,
    CMPIBoolean out,
    CMPIStatus* status)
{
    CMPIArgs* ca;
    CMPIStatus st;

    /* Check parameters */

    if (!self || self->magic != KMAGIC)
    {
        KSetStatus(&st, ERR_FAILED);
        return NULL;
    }

    /* Create args */

    if (!(ca = CMNewArgs(self->cb, status)))
        return NULL;

    if (!KOkay(st = KBase_SetToArgs(self, in, out, ca)))
    {
        __KSetStatus(status, st.rc);
        return NULL;
    }

    return ca;
}

static CMPIStatus _set_value(KValue* kv, KTag tag, const CMPIData* cd)
{
    /* Strip unused flags */
    tag = KTypeOf(tag) | (tag & KTAG_ARRAY);

    if (_cmpitype_to_ktag(cd->type) != tag)
    {
        KReturn(ERR_FAILED);
    }

    kv->exists = 1;

    if (cd->state & CMPI_nullValue)
    {
        kv->null = 1;
        kv->u.uint64 = 0;
        KReturn(OK);
    }
    else
    {
        kv->u.uint64 = cd->value.uint64;

        if (tag == KTYPE_STRING)
        {
            KString* ks = (KString*)kv;
            ks->chars = KChars(ks->value);
        }
        if (tag & KTAG_ARRAY)
        {
            KArray* ks = (KArray*)kv;
            ks->count = CMGetArrayCount(ks->value, NULL);
        }

        /* ATTN: validate references and instances */
        KReturn(OK);
    }
}

static CMPIStatus _get_int(const CMPIData* cd, CMPISint64* x)
{
    switch (cd->type)
    {
        case CMPI_uint8:
            *x = cd->value.uint8;
            KReturn(OK);
        case CMPI_sint8:
            *x = cd->value.sint8;
            KReturn(OK);
        case CMPI_uint16:
            *x = cd->value.uint16;
            KReturn(OK);
        case CMPI_sint16:
            *x = cd->value.sint16;
            KReturn(OK);
        case CMPI_uint32:
            *x = cd->value.uint32;
            KReturn(OK);
        case CMPI_sint32:
            *x = cd->value.sint32;
            KReturn(OK);
        case CMPI_uint64:
            *x = cd->value.uint64;
            KReturn(OK);
        case CMPI_sint64:
            *x = cd->value.sint64;
            KReturn(OK);
        default:
            KReturn(ERR_TYPE_MISMATCH);
    }
}

static CMPIStatus _set_key_value(KValue* kv, KTag tag, const CMPIData* cd)
{
    CMPISint64 x;

    /* Strip unused flags */
    tag = KTypeOf(tag) | (tag & KTAG_ARRAY);
    kv->exists = 1;

    /* Keys cannot be arrays */

    if (tag & KTAG_ARRAY)
        KReturn(ERR_TYPE_MISMATCH);

    /* Handle null case */

    if (cd->state & CMPI_nullValue)
    {
        kv->null = 1;
        kv->u.uint64 = 0;
        KReturn(OK);
    }
    else
        kv->null = 0;

    /* Handle case with value */

    switch (KTypeOf(tag))
    {
        case KTYPE_BOOLEAN:
        {
            if (cd->type != CMPI_boolean)
                KReturn(ERR_TYPE_MISMATCH);
            kv->u.boolean = cd->value.boolean;
            KReturn(OK);
        }
        case KTYPE_UINT8:
        {
            if (!KOkay(_get_int(cd, &x)))
                KReturn(ERR_FAILED);
            kv->u.uint8 = x;
            KReturn(OK);
        }
        case KTYPE_SINT8:
        {
            if (!KOkay(_get_int(cd, &x)))
                KReturn(ERR_FAILED);
            kv->u.sint8 = x;
            KReturn(OK);
        }
        case KTYPE_UINT16:
        {
            if (!KOkay(_get_int(cd, &x)))
                KReturn(ERR_FAILED);
            kv->u.uint16 = x;
            KReturn(OK);
        }
        case KTYPE_SINT16:
        {
            if (!KOkay(_get_int(cd, &x)))
                KReturn(ERR_FAILED);
            kv->u.sint16 = x;
            KReturn(OK);
        }
        case KTYPE_UINT32:
        {
            if (!KOkay(_get_int(cd, &x)))
                KReturn(ERR_FAILED);
            kv->u.uint32 = x;
            KReturn(OK);
        }
        case KTYPE_SINT32:
        {
            if (!KOkay(_get_int(cd, &x)))
                KReturn(ERR_FAILED);
            kv->u.sint32 = x;
            KReturn(OK);
        }
        case KTYPE_UINT64:
        {
            if (!KOkay(_get_int(cd, &x)))
                KReturn(ERR_FAILED);
            kv->u.uint64 = x;
            KReturn(OK);
        }
        case KTYPE_SINT64:
        {
            if (!KOkay(_get_int(cd, &x)))
                KReturn(ERR_FAILED);
            kv->u.sint64 = x;
            KReturn(OK);
        }
        case KTYPE_STRING:
        {
            KString* ks = (KString*)kv;

            if (cd->type != CMPI_string)
                KReturn(ERR_FAILED);

            ks->value = cd->value.string;
            ks->chars = KChars(ks->value);
            KReturn(OK);
        }
        case KTYPE_REFERENCE:
        {
            KRef* kr = (KRef*)kv;

            if (cd->type != CMPI_ref)
                KReturn(ERR_FAILED);

            kr->value = cd->value.ref;
            KReturn(OK);
        }
        case KTYPE_DATETIME:
        {
            /* ATTN: implement! */
            KReturn(ERR_TYPE_MISMATCH);
        }
        default:
            KReturn(ERR_TYPE_MISMATCH);
    }

    KReturn(OK);
}

static KValue* _find_property(KBase* self, const char* name, KTag* tag)
{
    KPos pos;
    *tag = 0;

    KFirst(&pos, self);

    while (KMore(&pos))
    {
        if (strcasecmp(pos.name, name) == 0)
        {
            *tag = pos.tag;
            return (KValue*)pos.field;
        }

        KNext(&pos);
    }

    /* Not found */
    return NULL;
}

CMPIStatus KBase_FromInstance(KBase* self, const CMPIInstance* ci)
{
    CMPIObjectPath* cop;
    CMPIString* cn;
    CMPIString* ns;
    CMPIStatus st = KSTATUS_INIT;
    CMPICount count;
    CMPICount i;
    KValue* kv;

    if (!self || self->magic != KMAGIC)
        KReturn(ERR_FAILED);

    /* Get object path */

    if (!(cop = CMGetObjectPath(ci, &st)))
        return st;

    /* Set namespace */

    if (!(ns = CMGetNameSpace(cop, &st)))
        return st;

    self->ns = ns;

    /* Get classname */

    if (!(cn = CMGetClassName(cop, &st)))
        return st;

    /* For each property */

    count = CMGetPropertyCount(ci, &st); 

    if (!KOkay(st))
        return st;

    for (i = 0; i < count; i++)
    {
        CMPIData cd;
        CMPIString* pn = NULL;
        KTag tag;

        /* Get i-th property */

        cd = CMGetPropertyAt(ci, i, &pn, &st);

        if (!KOkay(st))
            return st;

        if (!pn)
            KReturn(ERR_FAILED);

        /* Find the given property */

        if ((kv = _find_property(self, KChars(pn), &tag)))
        {
            _set_value(kv, tag, &cd);
        }
    }

    KReturn(OK);
}

CMPIStatus KBase_FromObjectPath(KBase* self, const CMPIObjectPath* cop)
{
    CMPIString* cn;
    CMPIString* ns;
    CMPIStatus st = KSTATUS_INIT;
    CMPICount count;
    CMPICount i;
    KValue* kv;

    if (!self || self->magic != KMAGIC)
        KReturn(ERR_FAILED);

    /* Set namespace */

    if (!(ns = CMGetNameSpace(cop, &st)))
        return st;

    self->ns = ns;

    /* Get classname */

    if (!(cn = CMGetClassName(cop, &st)))
        return st;

    /* For each property */

    count = CMGetKeyCount(cop, &st); 

    if (!KOkay(st))
        return st;

    for (i = 0; i < count; i++)
    {
        CMPIData cd;
        CMPIString* pn = NULL;
        KTag tag;

        /* Get i-th property */

        cd = CMGetKeyAt(cop, i, &pn, &st);

        if (!KOkay(st))
            return st;

        if (!pn)
            KReturn(ERR_FAILED);

        /* Find the given property */

        if ((kv = _find_property(self, KChars(pn), &tag)))
        {
            _set_key_value(kv, tag, &cd);
        }
    }

    KReturn(OK);
}

CMPIStatus KBase_FromArgs(
    KBase* self, 
    const CMPIArgs* ca, 
    CMPIBoolean in,
    CMPIBoolean out)
{
    CMPIStatus st = KSTATUS_INIT;
    CMPICount count;
    CMPICount i;
    KValue* kv;

    if (!self || self->magic != KMAGIC)
        KReturn(ERR_FAILED);

    /* For each arg */

    count = CMGetArgCount(ca, &st); 

    if (!KOkay(st))
        return st;

    for (i = 0; i < count; i++)
    {
        CMPIData cd;
        CMPIString* name = NULL;
        KTag tag;

        /* Get i-th property */

        cd = CMGetArgAt(ca, i, &name, &st);

        if (!KOkay(st))
            return st;

        if (!name)
            KReturn(ERR_FAILED);

        /* Find the given property */

        if ((kv = _find_property(self, KChars(name), &tag)))
        {
            if (in && !(tag & KTAG_IN))
                continue;

            if (out && !(tag & KTAG_OUT))
                continue;

            _set_value(kv, tag, &cd);
        }
    }

    KReturn(OK);
}

static CMPIBoolean _match_key(const CMPIData* cd1, const CMPIData* cd2)
{
    if (cd1->type != cd2->type)
    {
        return 0;
    }

    if (cd1->state & CMPI_nullValue)
    {
        return cd2->state & CMPI_nullValue ? 1 : 0;
    }

    if (cd2->state & CMPI_nullValue)
    {
        return cd1->state & CMPI_nullValue ? 1 : 0;
    }

    switch (cd1->type)
    {
        case CMPI_boolean:
            return cd1->value.boolean == cd2->value.boolean;
        case CMPI_uint8:
            return cd1->value.uint8 == cd2->value.uint8;
        case CMPI_sint8:
            return cd1->value.sint8 == cd2->value.sint8;
        case CMPI_uint16:
            return cd1->value.uint16 == cd2->value.uint16;
        case CMPI_sint16:
            return cd1->value.sint16 == cd2->value.sint16;
        case CMPI_uint32:
            return cd1->value.uint32 == cd2->value.uint32;
        case CMPI_sint32:
            return cd1->value.sint32 == cd2->value.sint32;
        case CMPI_uint64:
            return cd1->value.uint64 == cd2->value.uint64;
        case CMPI_sint64:
            return cd1->value.sint64 == cd2->value.sint64;
        case CMPI_real32:
            return cd1->value.real32 == cd2->value.real32;
        case CMPI_real64:
            return cd1->value.real64 == cd2->value.real64;
        case CMPI_char16:
            return cd1->value.char16 == cd2->value.char16;
        case CMPI_string:
        {
            const char* s1 = KChars(cd1->value.string);
            const char* s2 = KChars(cd2->value.string);
            return s1 && s2 && strcmp(s1, s2) == 0;
        }
        case CMPI_dateTime:
        {
            CMPIUint64 x1 = CMGetBinaryFormat(cd1->value.dateTime, NULL);
            CMPIUint64 x2 = CMGetBinaryFormat(cd2->value.dateTime, NULL);
            return x1 == x2;
        }
        case CMPI_ref:
            return KMatch(cd1->value.ref, cd2->value.ref);
        default:
            return 0;
    }

    return 1;
}

CMPIBoolean KMatch(const CMPIObjectPath* cop1, const CMPIObjectPath* cop2)
{
    CMPIStatus st = KSTATUS_INIT;
    CMPICount count1;
    CMPICount count2;
    CMPICount i;

    if (!cop1 || !cop2)
        return 0;

    count1 = CMGetKeyCount(cop1, &st); 

    if (!KOkay(st))
        return 0;

    count2 = CMGetKeyCount(cop2, &st); 

    if (!KOkay(st))
        return 0;

    if (count1 != count2)
        return 0;

    for (i = 0; i < count1; i++)
    {
        CMPIData cd1;
        CMPIData cd2;
        CMPIString* pn;

        cd1 = CMGetKeyAt(cop1, i, &pn, &st);

        if (!KOkay(st) || !pn)
            return 0;

        cd2 = CMGetKey(cop2, KChars(pn), &st);

        if (!KOkay(st))
            return 0;

        if (!_match_key(&cd1, &cd2))
            return 0;
    }

    /* Identicial */
    return 1;
}

/*
**==============================================================================
**
** KArray
**
**==============================================================================
*/

CMPIBoolean KArray_Init(
    KArray* self, 
    const CMPIBroker* cb, 
    CMPICount max,
    CMPIType type)
{
    CMPIArray* array;

    if (!self || !cb)
        return 0;

    if (!(array = CMNewArray(cb, max, type, NULL)))
        return 0;

    self->value = array;
    self->exists = 1;
    self->null = 0;
    self->count = max;

    return 1;
}

CMPIBoolean KArray_Set(
    KArray* self, 
    CMPICount i,
    void* value,
    CMPIType type)
{
    CMPIStatus st = KSTATUS_INIT;

    if (!self || !self->exists || !self->value)
        return 0;

    st = CMSetArrayElementAt((CMPIArray*)self->value, i, value, type);

    if (!KOkay(st))
        return 0;

    return 1;
}

CMPIBoolean KArray_Null(
    KArray* self, 
    CMPICount i,
    CMPIType type)
{
    CMPIStatus st = KSTATUS_INIT;

    if (!self || !self->exists || !self->value)
        return 0;

    st = CMSetArrayElementAt((CMPIArray*)self->value, i, NULL, type);

    if (!KOkay(st))
        return 0;

    return 1;
}

void KArray_Get(
    const KArray* self,
    CMPICount i,
    CMPIType type,
    KValue* value)
{
    CMPIData cd;
    CMPIStatus st = KSTATUS_INIT;

    if (!self || !self->exists || !self->value)
    {
        memset(value, 0, sizeof(*value));
        return;
    }

    cd = CMGetArrayElementAt(self->value, i, &st);

    if (!KOkay(st) || cd.type != type)
    {
        memset(value, 0, sizeof(*value));
        return;
    }

    memset(value, 0, sizeof(*value));
    value->exists = 1;

    if (cd.state & CMPI_nullValue)
        value->null = 1;
    else
        value->u.uint64 = cd.value.uint64;
}

CMPIBoolean KStringA_Set(
    KStringA* self, 
    const CMPIBroker* cb,
    CMPICount i,
    const char* s)
{
    CMPIString* x = CMNewString(cb, s, NULL);
    
    if (!x)
        return 0;

    return KStringA_SetString(self, i, x);
}

const char* KStringA_Get(
    const KStringA* self,
    CMPICount i)
{
    KString ks;

    ks = KStringA_GetString(self, i);

    if (!ks.exists || ks.null)
        return NULL;

    return KChars(ks.value);
}

CMPIData KFindKey(
    const CMPIObjectPath* cop, 
    const char* name,
    CMPIStatus* status)
{
    CMPICount count;
    CMPICount i;
    CMPIData cd;

    count = CMGetKeyCount(cop, NULL);

    for (i = 0; i < count; i++)
    {
        CMPIString* pn = NULL;
        CMPIStatus st;

        cd = CMGetKeyAt(cop, i, &pn, &st);

        if (!KOkay(st) || !pn)
        {
            memset(&cd, 0, sizeof(cd));

            if (status)
                CMSetStatus(status, CMPI_RC_ERR_FAILED);

            return cd;
        }

        if (strcasecmp(KChars(pn), name) == 0)
        {
            if (status)
                CMSetStatus(status, CMPI_RC_OK);

            return cd;
        }
    }

    /* Not found! */

    memset(&cd, 0, sizeof(cd));

    if (status)
        CMSetStatus(status, CMPI_RC_ERR_NOT_FOUND);

    return cd;
}

CMPIData KFindProperty(
    const CMPIInstance* ci, 
    const char* name,
    CMPIStatus* status)
{
    CMPICount count;
    CMPICount i;
    CMPIData cd;

    count = CMGetPropertyCount(ci, NULL);

    for (i = 0; i < count; i++)
    {
        CMPIString* pn = NULL;
        CMPIStatus st;

        cd = CMGetPropertyAt(ci, i, &pn, &st);

        if (!KOkay(st) || !pn)
        {
            memset(&cd, 0, sizeof(cd));

            if (status)
                CMSetStatus(status, CMPI_RC_ERR_FAILED);

            return cd;
        }

        if (strcasecmp(KChars(pn), name) == 0)
        {
            if (status)
                CMSetStatus(status, CMPI_RC_OK);

            return cd;
        }
    }

    /* Not found! */

    memset(&cd, 0, sizeof(cd));

    if (status)
        CMSetStatus(status, CMPI_RC_ERR_NOT_FOUND);

    return cd;
}
