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

#ifndef _konkret_h
#define _konkret_h

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <cmpidt.h>
#include <cmpift.h>
#include <cmpimacs.h>
#include <cmpios.h>

/*
**==============================================================================
**
** KONKRET_VERSION
**
**==============================================================================
*/

#define KONKRET_MAJOR 0
#define KONKRET_MINOR 9
#define KONKRET_REVISION 0

#define KONKRET_VERSION \
    ((KONKRET_MAJOR << 24) | (KONKRET_MINOR << 16) | (KONKRET_REVISION << 8))

#define __KONKRET_VERSION_STRING2(MAJOR, MINOR, REVISION) \
    #MAJOR"."#MINOR"."#REVISION

#define __KONKRET_VERSION_STRING1(MAJOR, MINOR, REVISION) \
    __KONKRET_VERSION_STRING2(MAJOR, MINOR, REVISION)

#define KONKRET_VERSION_STRING \
    __KONKRET_VERSION_STRING1(KONKRET_MAJOR, KONKRET_MINOR, KONKRET_REVISION)

/*
**==============================================================================
**
** KHIDE
**
**==============================================================================
*/

#define KHIDE __attribute__((visibility("hidden")))

/*
**==============================================================================
**
** KEXTERN
**
**==============================================================================
*/

#define KEXTERN extern

/*
**==============================================================================
**
** KINLINE
**
**==============================================================================
*/

#ifndef KINLINE
# if defined(__GNUC__)
#  define KINLINE static __inline__
# else
#  define KINLINE static
# endif
#endif


/*
**==============================================================================
**
** KUNUSED
**
**==============================================================================
*/

#ifdef __GNUC__
# define KUNUSED __attribute__((__unused__))
#else
# define KUNUSED /* empty */
#endif

/*
**==============================================================================
**
** KTRACE
**
**==============================================================================
*/

#define KTRACE printf("KTRACE: %s(%d)\n", __FILE__, __LINE__)

/*
**==============================================================================
**
** KMAGIC
**
**==============================================================================
*/

#define KMAGIC 0xA40F36C9

/*
**==============================================================================
**
** CMPIString
**
**==============================================================================
*/

KINLINE const char* KChars(const CMPIString* s)
{
    return s ?  CMGetCharsPtr(s, NULL) : NULL;
}

KINLINE CMPIString* KNewString(const CMPIBroker* cb, const char* s)
{
    return (cb && s) ?  CMNewString(cb, s, NULL) : NULL;
}

/*
**==============================================================================
**
** CMPIStatus
**
**==============================================================================
*/

#define KSTATUS_INIT { CMPI_RC_OK, NULL }

KINLINE CMPIBoolean KOkay(CMPIStatus st)
{
    return st.rc == CMPI_RC_OK;
}

KINLINE void __KSetStatus(CMPIStatus* status, CMPIrc rc)
{
    if (status)
    {
        status->rc = rc;
        status->msg = NULL;
    }
}

#define KSetStatus(STATUS, CODE) __KSetStatus(STATUS, CMPI_RC_##CODE)

KINLINE void __KSetStatus2(
    const CMPIBroker* cb, CMPIStatus* status, CMPIrc rc, const char* msg)
{
    if (status)
        CMSetStatusWithChars(cb, status, rc, msg);
}

#define KSetStatus2(CB, STATUS, CODE, MSG) \
    __KSetStatus2(CB, STATUS, CMPI_RC_##CODE, MSG)

KINLINE CMPIStatus __KReturn(CMPIrc RC)
{
    CMPIStatus status;
    __KSetStatus(&status, RC);
    return status;
}

#define KReturn(CODE) return __KReturn(CMPI_RC_##CODE)

KINLINE void KPutStatus(CMPIStatus* st)
{
    if (st)
        fprintf(stderr, "CMPIStatus{%u, %s}\n", st->rc, KChars(st->msg));
}

KINLINE CMPIStatus __KReturn2(
    const CMPIBroker* cb, 
    CMPIrc rc,
    const char* format,
    ...)
{
    va_list args;
    char* str = NULL;
    va_start(args, format);
    vasprintf(&str, format, args);
    va_end(args);
    CMPIStatus stat={(rc),NULL};
    stat.msg=cb->eft->newString(cb, str, NULL);
    free(str);
    return stat;
}

#define KReturn2(CB, CODE, MSG, ...) return __KReturn2(CB, CMPI_RC_##CODE, MSG, ##__VA_ARGS__)

/*
**==============================================================================
**
** CMPIObjectPath
**
**==============================================================================
*/

KINLINE const char* KNameSpace(const CMPIObjectPath* cop)
{
    CMPIStatus st;
    const char* ns;

    if (!cop)
        return NULL;

    ns = KChars(CMGetNameSpace(cop, &st));

    if (!KOkay(st))
        return NULL;

    return ns;
}

KINLINE const char* KClassName(const CMPIObjectPath* cop)
{
    CMPIStatus st;
    const char* cn;

    if (!cop)
        return NULL;

    cn = KChars(CMGetClassName(cop, &st));

    if (!KOkay(st))
        return NULL;

    return cn;
}

/*
**==============================================================================
**
** KType
**
**==============================================================================
*/

typedef enum _KType
{
    KTYPE_BOOLEAN,
    KTYPE_UINT8,
    KTYPE_SINT8,
    KTYPE_UINT16,
    KTYPE_SINT16,
    KTYPE_UINT32,
    KTYPE_SINT32,
    KTYPE_UINT64,
    KTYPE_SINT64,
    KTYPE_REAL32,
    KTYPE_REAL64,
    KTYPE_CHAR16,
    KTYPE_STRING,
    KTYPE_DATETIME,
    KTYPE_REFERENCE,
    KTYPE_INSTANCE
}
KType;

/*
**==============================================================================
**
** KTag
**
**==============================================================================
*/

/* (KTAG_ARRAY | KTAG_KEY | KTAG_IN | KTAG_OUT | KType) */
typedef unsigned char KTag;

#define KTAG_ARRAY  0x80
#define KTAG_KEY    0x40
#define KTAG_IN     0x20
#define KTAG_OUT    0x10

KINLINE KTag KTypeOf(KTag tag)
{
    return 0x0F & tag;
}

KEXTERN size_t KTypeSize(KTag tag);

/*
**==============================================================================
**
** KBase
**
**==============================================================================
*/

/* Every generated class includes this structure first */
typedef struct _KBase
{
    /* Magic number */
    CMPIUint32 magic;

    /* Structure size */
    CMPIUint32 size;

    /* Type signature */
    const unsigned char* sig;

    /* Pointer to broker */
    const CMPIBroker* cb;

    /* Namespace */
    const CMPIString* ns;
}
KBase;

KEXTERN void KBase_Init(
    KBase* self,
    const CMPIBroker* cb,
    size_t size,
    const unsigned char* sig,
    const char* ns);

KEXTERN CMPIObjectPath* KBase_ToObjectPath(
    const KBase* self, 
    CMPIStatus* status);

KEXTERN CMPIInstance* KBase_ToInstance(
    const KBase* self, 
    CMPIStatus* status);

KEXTERN CMPIStatus KBase_SetToArgs(
    const KBase* self, 
    CMPIBoolean in,
    CMPIBoolean out,
    CMPIArgs* args);

KEXTERN CMPIArgs* KBase_ToArgs(
    const KBase* self, 
    CMPIBoolean in,
    CMPIBoolean out,
    CMPIStatus* st);

KEXTERN CMPIStatus KBase_FromInstance(
    KBase* self, 
    const CMPIInstance* ci);

KEXTERN CMPIStatus KBase_FromObjectPath(
    KBase* self, 
    const CMPIObjectPath* cop);

KEXTERN CMPIStatus KBase_FromArgs(
    KBase* self, 
    const CMPIArgs* ca,
    CMPIBoolean in,
    CMPIBoolean out);

KEXTERN CMPIStatus KBase_Print(
    FILE* os, 
    const KBase* self, 
    char type);

/*
**==============================================================================
**
** KValue
**
**==============================================================================
*/

typedef struct _KValue
{
    CMPIUint32 exists;
    CMPIUint32 null;
    union
    {
        CMPIBoolean boolean;
        CMPIUint8 uint8;
        CMPISint8 sint8;
        CMPIUint16 uint16;
        CMPISint16 sint16;
        CMPIUint32 uint32;
        CMPISint32 sint32;
        CMPIUint64 uint64;
        CMPISint64 sint64;
        CMPIReal32 real32;
        CMPIReal64 real64;
        CMPIChar16 char16;
        CMPIString* string;
        CMPIDateTime* dateTime;
        CMPIObjectPath* ref;
        CMPIInstance* instance;
        CMPIArray* array;
    }
    u;
}
KValue;

KINLINE CMPIBoolean __KHasValue(KValue* kv)
{
    return kv->exists && !kv->null;
}

#define KHasValue(VALUE) __KHasValue((KValue*)VALUE)

/*
**==============================================================================
**
** KBoolean
**
**==============================================================================
*/

typedef struct _KBoolean
{
    CMPIUint32 exists;
    CMPIUint32 null;
    CMPIBoolean value;
    char __padding[7];
}
KBoolean;

KINLINE void KBoolean_Set(KBoolean* self, CMPIBoolean value)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->value = value;
    }
}

KINLINE void KBoolean_Null(KBoolean* self)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->null = 1;
    }
}

KINLINE void KBoolean_Clr(KBoolean* self)
{
    if (self)
        memset(self, 0, sizeof(*self));
}

#define KBOOLEAN_INIT { 0, 0, 0, {0} }

/*
**==============================================================================
**
** KUint8
**
**==============================================================================
*/

typedef struct _KUint8
{
    CMPIUint32 exists;
    CMPIUint32 null;
    CMPIUint8 value;
    char __padding[7];
}
KUint8;

KINLINE void KUint8_Set(KUint8* self, CMPIUint8 value)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->value = value;
    }
}

KINLINE void KUint8_Null(KUint8* self)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->null = 1;
    }
}

KINLINE void KUint8_Clr(KUint8* self)
{
    if (self)
        memset(self, 0, sizeof(*self));
}

#define KUINT8_INIT { 0, 0, 0, {0} }

/*
**==============================================================================
**
** KSint8
**
**==============================================================================
*/

typedef struct _KSint8
{
    CMPIUint32 exists;
    CMPIUint32 null;
    CMPISint8 value;
    char __padding[7];
}
KSint8;

KINLINE void KSint8_Set(KSint8* self, CMPISint8 value)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->value = value;
    }
}

KINLINE void KSint8_Null(KSint8* self)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->null = 1;
    }
}

KINLINE void KSint8_Clr(KSint8* self)
{
    if (self)
        memset(self, 0, sizeof(*self));
}

#define KSINT8_INIT { 0, 0, 0, {0} }

/*
**==============================================================================
**
** KUint16
**
**==============================================================================
*/

typedef struct _KUint16
{
    CMPIUint32 exists;
    CMPIUint32 null;
    CMPIUint16 value;
    char __padding[6];
}
KUint16;

KINLINE void KUint16_Set(KUint16* self, CMPIUint16 value)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->value = value;
    }
}

KINLINE void KUint16_Null(KUint16* self)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->null = 1;
    }
}

KINLINE void KUint16_Clr(KUint16* self)
{
    if (self)
        memset(self, 0, sizeof(*self));
}

#define KUINT16_INIT { 0, 0, 0, {0} }

/*
**==============================================================================
**
** KSint16
**
**==============================================================================
*/

typedef struct _KSint16
{
    CMPIUint32 exists;
    CMPIUint32 null;
    CMPISint16 value;
    char __padding[6];
}
KSint16;

KINLINE void KSint16_Set(KSint16* self, CMPISint16 value)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->value = value;
    }
}

KINLINE void KSint16_Null(KSint16* self)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->null = 1;
    }
}

KINLINE void KSint16_Clr(KSint16* self)
{
    if (self)
        memset(self, 0, sizeof(*self));
}

#define KSINT16_INIT { 0, 0, 0, {0} }

/*
**==============================================================================
**
** KUint32
**
**==============================================================================
*/

typedef struct _KUint32
{
    CMPIUint32 exists;
    CMPIUint32 null;
    CMPIUint32 value;
    char __padding[4];
}
KUint32;

KINLINE void KUint32_Set(KUint32* self, CMPIUint32 value)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->value = value;
    }
}

KINLINE void KUint32_Null(KUint32* self)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->null = 1;
    }
}

KINLINE void KUint32_Clr(KUint32* self)
{
    if (self)
        memset(self, 0, sizeof(*self));
}

#define KUINT32_INIT { 0, 0, 0, {0} }

/*
**==============================================================================
**
** KSint32
**
**==============================================================================
*/

typedef struct _KSint32
{
    CMPIUint32 exists;
    CMPIUint32 null;
    CMPISint32 value;
    char __padding[4];
}
KSint32;

KINLINE void KSint32_Set(KSint32* self, CMPISint32 value)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->value = value;
    }
}

KINLINE void KSint32_Null(KSint32* self)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->null = 1;
    }
}

KINLINE void KSint32_Clr(KSint32* self)
{
    if (self)
        memset(self, 0, sizeof(*self));
}

#define KSINT32_INIT { 0, 0, 0, {0} }

/*
**==============================================================================
**
** KUint64
**
**==============================================================================
*/

typedef struct _KUint64
{
    CMPIUint32 exists;
    CMPIUint32 null;
    CMPIUint64 value;
}
KUint64;

KINLINE void KUint64_Set(KUint64* self, CMPIUint64 value)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->value = value;
    }
}

KINLINE void KUint64_Null(KUint64* self)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->null = 1;
    }
}

KINLINE void KUint64_Clr(KUint64* self)
{
    if (self)
        memset(self, 0, sizeof(*self));
}

#define KUINT64_INIT { 0, 0, 0, {0} }

/*
**==============================================================================
**
** KSint64
**
**==============================================================================
*/

typedef struct _KSint64
{
    CMPIUint32 exists;
    CMPIUint32 null;
    CMPISint64 value;
}
KSint64;

KINLINE void KSint64_Set(KSint64* self, CMPISint64 value)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->value = value;
    }
}

KINLINE void KSint64_Null(KSint64* self)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->null = 1;
    }
}

KINLINE void KSint64_Clr(KSint64* self)
{
    if (self)
        memset(self, 0, sizeof(*self));
}

#define KSINT64_INIT { 0, 0, 0, {0} }

/*
**==============================================================================
**
** KReal32
**
**==============================================================================
*/

typedef struct _KReal32
{
    CMPIUint32 exists;
    CMPIUint32 null;
    CMPIReal32 value;
    char __padding[4];
}
KReal32;

#define KREAL32_INIT { 0, 0, 0.0, {0} }

KINLINE void KReal32_Set(KReal32* self, CMPIReal32 value)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->value = value;
    }
}

KINLINE void KReal32_Null(KReal32* self)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->null = 1;
    }
}

KINLINE void KReal32_Clr(KReal32* self)
{
    if (self)
        memset(self, 0, sizeof(*self));
}

/*
**==============================================================================
**
** KReal64
**
**==============================================================================
*/

typedef struct _KReal64
{
    CMPIUint32 exists;
    CMPIUint32 null;
    CMPIReal64 value;
}
KReal64;

KINLINE void KReal64_Set(KReal64* self, CMPIReal64 value)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->value = value;
    }
}

KINLINE void KReal64_Null(KReal64* self)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->null = 1;
    }
}

KINLINE void KReal64_Clr(KReal64* self)
{
    if (self)
        memset(self, 0, sizeof(*self));
}

#define KREAL64_INIT { 0, 0, 0.0, {0} }

/*
**==============================================================================
**
** KChar16
**
**==============================================================================
*/

typedef struct _KChar16
{
    CMPIUint32 exists;
    CMPIUint32 null;
    CMPIChar16 value;
    char __padding[6];
}
KChar16;

KINLINE void KChar16_Set(KChar16* self, CMPIChar16 value)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->value = value;
    }
}

KINLINE void KChar16_Null(KChar16* self)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->null = 1;
    }
}

KINLINE void KChar16_Clr(KChar16* self)
{
    if (self)
        memset(self, 0, sizeof(*self));
}

#define KCHAR16_INIT { 0, 0, 0, {0} }

/*
**==============================================================================
**
** KString
**
**==============================================================================
*/

typedef struct _KString
{
    CMPIUint32 exists;
    CMPIUint32 null;
    CMPIString* value;
    char __padding1[16 - sizeof(CMPIString*)];
    const char* chars;
    char __padding2[16 - sizeof(CMPIString*)];
}
KString;

KEXTERN void KString_SetString(KString* self, CMPIString* value);

KEXTERN CMPIStatus KString_Set(
    KString* self, 
    const CMPIBroker* cb, 
    const char* s);

KINLINE void KString_Null(KString* self)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->null = 1;
    }
}

KINLINE void KString_Clr(KString* self)
{
    if (self)
        memset(self, 0, sizeof(*self));
}

#define KSTRING_INIT { 0, 0, NULL, {0}, NULL, {0} }

/*
**==============================================================================
**
** KDateTime
**
**==============================================================================
*/

typedef struct _KDateTime
{
    CMPIUint32 exists;
    CMPIUint32 null;
    CMPIDateTime* value;
    char __padding[16 - sizeof(CMPIDateTime*)];
}
KDateTime;

KINLINE void KDateTime_Set(KDateTime* self, CMPIDateTime* value)
{
    if (self && value)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->value = value;
    }
}

KINLINE void KDateTime_Null(KDateTime* self)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->null = 1;
    }
}

KINLINE void KDateTime_Clr(KDateTime* self)
{
    if (self)
        memset(self, 0, sizeof(*self));
}

#define KDATETIME_INIT { 0, 0, NULL, {0} }

/*
**==============================================================================
**
** KRef
**
**==============================================================================
*/

typedef struct _KRef
{
    CMPIUint32 exists;
    CMPIUint32 null;
    const CMPIObjectPath* value;
    char __padding1[16 - sizeof(CMPIArray*)];
    const unsigned char* __sig;
    char __padding2[16 - sizeof(char*)];
}
KRef;

KINLINE void KRef_SetObjectPath(KRef* self, const CMPIObjectPath* cop)
{
    if (self)
    {
        const unsigned char* sig = self->__sig;
        memset(self, 0, sizeof(*self));
        self->__sig = sig;
        self->exists = 1;
        self->value = cop;
    }
}

KINLINE CMPIStatus KRef_Set(
    KRef* self,
    const KBase* x)
{
    CMPIObjectPath* cop;
    CMPIStatus status = KSTATUS_INIT;

    if (!(cop = KBase_ToObjectPath(x, &status)))
        return status;

    KRef_SetObjectPath(self, cop);
    KReturn(OK);
}

KINLINE void KRef_Null(KRef* self)
{
    if (self)
    {
        const unsigned char* sig = self->__sig;
        memset(self, 0, sizeof(*self));
        self->__sig = sig;
        self->exists = 1;
        self->null = 1;
    }
}

KINLINE void KRef_Clr(KRef* self)
{
    if (self)
    {
        const unsigned char* sig = self->__sig;
        memset(self, 0, sizeof(*self));
        self->__sig = sig;
    }
}

#define KREF_INIT { 0, 0, NULL, {0}, NULL, {0} }

/*
**==============================================================================
**
** KInstance
**
**==============================================================================
*/

typedef struct _KInstance
{
    CMPIUint32 exists;
    CMPIUint32 null;
    const CMPIInstance* value;
    char __padding1[16 - sizeof(CMPIArray*)];
    const unsigned char* __sig;
    char __padding2[16 - sizeof(char*)];
}
KInstance;

KINLINE void KInstance_Set(KInstance* self, const CMPIInstance* ci)
{
    if (self)
    {
        const unsigned char* sig = self->__sig;
        memset(self, 0, sizeof(*self));
        self->__sig = sig;
        self->exists = 1;
        self->value = ci;
    }
}

KINLINE CMPIStatus KInstance_SetInstance(
    KInstance* self,
    const KBase* x)
{
    CMPIInstance* ci;
    CMPIStatus status = KSTATUS_INIT;

    if (!(ci = KBase_ToInstance(x, &status)))
        return status;

    KInstance_Set(self, ci);
    KReturn(OK);
}

KINLINE void KInstance_Null(KInstance* self)
{
    if (self)
    {
        const unsigned char* sig = self->__sig;
        memset(self, 0, sizeof(*self));
        self->__sig = sig;
        self->exists = 1;
        self->null = 1;
    }
}

KINLINE void KInstance_Clr(KInstance* self)
{
    if (self)
    {
        const unsigned char* sig = self->__sig;
        memset(self, 0, sizeof(*self));
        self->__sig = sig;
    }
}

#define KINSTANCE_INIT { 0, 0, NULL, {0}, NULL, {0} }

/*
**==============================================================================
**
** KArray
**
**==============================================================================
*/

typedef struct _KArray
{
    CMPIUint32 exists;
    CMPIUint32 null;
    const CMPIArray* value;
    char __padding1[16 - sizeof(CMPIArray*)];
    CMPIUint32 count;
    char __padding2[4];
}
KArray;

KEXTERN CMPIBoolean KArray_Init(
    KArray* self, 
    const CMPIBroker* cb, 
    CMPICount max,
    CMPIType type);

KINLINE void KArray_InitNull(KArray* self)
{
    if (self)
    {
        memset(self, 0, sizeof(*self));
        self->exists = 1;
        self->null = 1;
    }
}

KEXTERN CMPIBoolean KArray_Set(
    KArray* self, 
    CMPICount i,
    void* value,
    CMPIType type);

KEXTERN CMPIBoolean KArray_Null(KArray* self, CMPICount i, CMPIType type);

KEXTERN void KArray_Get(
    const KArray* self,
    CMPICount i, 
    CMPIType type, 
    KValue* value);

KINLINE void KArray_Clr(KArray* self)
{
    if (self)
        memset(self, 0, sizeof(*self));
}

#define KARRAY_INIT { 0, 0, NULL, {0}, 0, {0} }

/*
**==============================================================================
**
** KBooleanA
**
**==============================================================================
*/

typedef struct _KBooleanA
{
    CMPIUint32 exists;
    CMPIUint32 null;
    const CMPIArray* value;
    char __padding1[16 - sizeof(CMPIArray*)];
    CMPIUint32 count;
    char __padding2[4];
}
KBooleanA;

KINLINE CMPIBoolean KBooleanA_Init(
    KBooleanA* self, 
    const CMPIBroker* cb, 
    CMPICount max)
{
    return KArray_Init((KArray*)self, cb, max, CMPI_boolean);
}

KINLINE void KBooleanA_InitNull(KBooleanA* self)
{
    KArray_InitNull((KArray*)self);
}

KINLINE CMPIBoolean KBooleanA_Set(
    KBooleanA* self, 
    CMPICount i,
    CMPIBoolean x)
{
    return KArray_Set((KArray*)self, i, &x, CMPI_boolean);
}

KINLINE CMPIBoolean KBooleanA_Null(KBooleanA* self, CMPICount i)
{
    return KArray_Null((KArray*)self, i, CMPI_boolean);
}

KINLINE KBoolean KBooleanA_Get(const KBooleanA* self, CMPICount i)
{
    KBoolean result;
    KArray_Get((const KArray*)self, i, CMPI_boolean, (KValue*)&result);
    return result;
}

KINLINE void KBooleanA_Clr(KBooleanA* self)
{
    KArray_Clr((KArray*)self);
}

#define KBOOLEANA_INIT { 0, 0, NULL, {0}, 0, {0} }

/*
**==============================================================================
**
** KUint8A
**
**==============================================================================
*/

typedef struct _KUint8A
{
    CMPIUint32 exists;
    CMPIUint32 null;
    const CMPIArray* value;
    char __padding1[16 - sizeof(CMPIArray*)];
    CMPIUint32 count;
    char __padding2[4];
}
KUint8A;

KINLINE CMPIBoolean KUint8A_Init(
    KUint8A* self, 
    const CMPIBroker* cb, 
    CMPICount max)
{
    return KArray_Init((KArray*)self, cb, max, CMPI_uint8);
}

KINLINE void KUint8A_InitNull(KUint8A* self)
{
    KArray_InitNull((KArray*)self);
}

KINLINE CMPIBoolean KUint8A_Set(
    KUint8A* self, 
    CMPICount i,
    CMPIUint8 x)
{
    return KArray_Set((KArray*)self, i, &x, CMPI_uint8);
}

KINLINE CMPIBoolean KUint8A_Null(KUint8A* self, CMPICount i)
{
    return KArray_Null((KArray*)self, i, CMPI_uint8);
}

KINLINE KUint8 KUint8A_Get(const KUint8A* self, CMPICount i)
{
    KUint8 result;
    KArray_Get((const KArray*)self, i, CMPI_uint8, (KValue*)&result);
    return result;
}

KINLINE void KUint8A_Clr(KUint8A* self)
{
    KArray_Clr((KArray*)self);
}

#define KUINT8A_INIT { 0, 0, NULL, {0}, 0, {0} }

/*
**==============================================================================
**
** KSint8A
**
**==============================================================================
*/

typedef struct _KSint8A
{
    CMPIUint32 exists;
    CMPIUint32 null;
    const CMPIArray* value;
    char __padding1[16 - sizeof(CMPIArray*)];
    CMPIUint32 count;
    char __padding2[4];
}
KSint8A;

KINLINE CMPIBoolean KSint8A_Init(
    KSint8A* self, 
    const CMPIBroker* cb, 
    CMPICount max)
{
    return KArray_Init((KArray*)self, cb, max, CMPI_sint8);
}

KINLINE void KSint8A_InitNull(KSint8A* self)
{
    KArray_InitNull((KArray*)self);
}

KINLINE CMPIBoolean KSint8A_Set(
    KSint8A* self, 
    CMPICount i,
    CMPISint8 x)
{
    return KArray_Set((KArray*)self, i, &x, CMPI_sint8);
}

KINLINE CMPIBoolean KSint8A_Null(KSint8A* self, CMPICount i)
{
    return KArray_Null((KArray*)self, i, CMPI_sint8);
}

KINLINE KSint8 KSint8A_Get(const KSint8A* self, CMPICount i)
{
    KSint8 result;
    KArray_Get((const KArray*)self, i, CMPI_sint8, (KValue*)&result);
    return result;
}

KINLINE void KSint8A_Clr(KSint8A* self)
{
    KArray_Clr((KArray*)self);
}

#define KSINT8A_INIT { 0, 0, NULL, {0}, 0, {0} }

/*
**==============================================================================
**
** KUint16A
**
**==============================================================================
*/

typedef struct _KUint16A
{
    CMPIUint32 exists;
    CMPIUint32 null;
    const CMPIArray* value;
    char __padding1[16 - sizeof(CMPIArray*)];
    CMPIUint32 count;
    char __padding2[4];
}
KUint16A;

KINLINE CMPIBoolean KUint16A_Init(
    KUint16A* self, 
    const CMPIBroker* cb, 
    CMPICount max)
{
    return KArray_Init((KArray*)self, cb, max, CMPI_uint16);
}

KINLINE void KUint16A_InitNull(KUint16A* self)
{
    KArray_InitNull((KArray*)self);
}

KINLINE CMPIBoolean KUint16A_Set(
    KUint16A* self, 
    CMPICount i,
    CMPIUint16 x)
{
    return KArray_Set((KArray*)self, i, &x, CMPI_uint16);
}

KINLINE CMPIBoolean KUint16A_Null(KUint16A* self, CMPICount i)
{
    return KArray_Null((KArray*)self, i, CMPI_uint16);
}

KINLINE KUint16 KUint16A_Get(const KUint16A* self, CMPICount i)
{
    KUint16 result;
    KArray_Get((const KArray*)self, i, CMPI_uint16, (KValue*)&result);
    return result;
}

KINLINE void KUint16A_Clr(KUint16A* self)
{
    KArray_Clr((KArray*)self);
}

#define KUINT16A_INIT { 0, 0, NULL, {0}, 0, {0} }

/*
**==============================================================================
**
** KSint16A
**
**==============================================================================
*/

typedef struct _KSint16A
{
    CMPIUint32 exists;
    CMPIUint32 null;
    const CMPIArray* value;
    char __padding1[16 - sizeof(CMPIArray*)];
    CMPIUint32 count;
    char __padding2[4];
}
KSint16A;

KINLINE CMPIBoolean KSint16A_Init(
    KSint16A* self, 
    const CMPIBroker* cb, 
    CMPICount max)
{
    return KArray_Init((KArray*)self, cb, max, CMPI_sint16);
}

KINLINE void KSint16A_InitNull(KSint16A* self)
{
    KArray_InitNull((KArray*)self);
}

KINLINE CMPIBoolean KSint16A_Set(
    KSint16A* self, 
    CMPICount i,
    CMPISint16 x)
{
    return KArray_Set((KArray*)self, i, &x, CMPI_sint16);
}

KINLINE CMPIBoolean KSint16A_Null(KSint16A* self, CMPICount i)
{
    return KArray_Null((KArray*)self, i, CMPI_sint16);
}

KINLINE KSint16 KSint16A_Get(const KSint16A* self, CMPICount i)
{
    KSint16 result;
    KArray_Get((const KArray*)self, i, CMPI_sint16, (KValue*)&result);
    return result;
}

KINLINE void KSint16A_Clr(KSint16A* self)
{
    KArray_Clr((KArray*)self);
}

#define KSINT16A_INIT { 0, 0, NULL, {0}, 0, {0} }

/*
**==============================================================================
**
** KUint32A
**
**==============================================================================
*/

typedef struct _KUint32A
{
    CMPIUint32 exists;
    CMPIUint32 null;
    const CMPIArray* value;
    char __padding1[16 - sizeof(CMPIArray*)];
    CMPIUint32 count;
    char __padding2[4];
}
KUint32A;

KINLINE CMPIBoolean KUint32A_Init(
    KUint32A* self, 
    const CMPIBroker* cb, 
    CMPICount max)
{
    return KArray_Init((KArray*)self, cb, max, CMPI_uint32);
}

KINLINE void KUint32A_InitNull(KUint32A* self)
{
    KArray_InitNull((KArray*)self);
}

KINLINE CMPIBoolean KUint32A_Set(
    KUint32A* self, 
    CMPICount i,
    CMPIUint32 x)
{
    return KArray_Set((KArray*)self, i, &x, CMPI_uint32);
}

KINLINE CMPIBoolean KUint32A_Null(KUint32A* self, CMPICount i)
{
    return KArray_Null((KArray*)self, i, CMPI_uint32);
}

KINLINE KUint32 KUint32A_Get(const KUint32A* self, CMPICount i)
{
    KUint32 result;
    KArray_Get((const KArray*)self, i, CMPI_uint32, (KValue*)&result);
    return result;
}

KINLINE void KUint32A_Clr(KUint32A* self)
{
    KArray_Clr((KArray*)self);
}

#define KUINT32A_INIT { 0, 0, NULL, {0}, 0, {0} }

/*
**==============================================================================
**
** KSint32A
**
**==============================================================================
*/

typedef struct _KSint32A
{
    CMPIUint32 exists;
    CMPIUint32 null;
    const CMPIArray* value;
    char __padding1[16 - sizeof(CMPIArray*)];
    CMPIUint32 count;
    char __padding2[4];
}
KSint32A;

KINLINE CMPIBoolean KSint32A_Init(
    KSint32A* self, 
    const CMPIBroker* cb, 
    CMPICount max)
{
    return KArray_Init((KArray*)self, cb, max, CMPI_sint32);
}

KINLINE void KSint32A_InitNull(KSint32A* self)
{
    KArray_InitNull((KArray*)self);
}

KINLINE CMPIBoolean KSint32A_Set(
    KSint32A* self, 
    CMPICount i,
    CMPISint32 x)
{
    return KArray_Set((KArray*)self, i, &x, CMPI_sint32);
}

KINLINE CMPIBoolean KSint32A_Null(KSint32A* self, CMPICount i)
{
    return KArray_Null((KArray*)self, i, CMPI_sint32);
}

KINLINE KSint32 KSint32A_Get(const KSint32A* self, CMPICount i)
{
    KSint32 result;
    KArray_Get((const KArray*)self, i, CMPI_sint32, (KValue*)&result);
    return result;
}

KINLINE void KSint32A_Clr(KSint32A* self)
{
    KArray_Clr((KArray*)self);
}

#define KSINT32A_INIT { 0, 0, NULL, {0}, 0, {0} }

/*
**==============================================================================
**
** KUint64A
**
**==============================================================================
*/

typedef struct _KUint64A
{
    CMPIUint32 exists;
    CMPIUint32 null;
    const CMPIArray* value;
    char __padding1[16 - sizeof(CMPIArray*)];
    CMPIUint32 count;
    char __padding2[4];
}
KUint64A;

KINLINE CMPIBoolean KUint64A_Init(
    KUint64A* self, 
    const CMPIBroker* cb, 
    CMPICount max)
{
    return KArray_Init((KArray*)self, cb, max, CMPI_uint64);
}

KINLINE void KUint64A_InitNull(KUint64A* self)
{
    KArray_InitNull((KArray*)self);
}

KINLINE CMPIBoolean KUint64A_Set(
    KUint64A* self, 
    CMPICount i,
    CMPIUint64 x)
{
    return KArray_Set((KArray*)self, i, &x, CMPI_uint64);
}

KINLINE CMPIBoolean KUint64A_Null(KUint64A* self, CMPICount i)
{
    return KArray_Null((KArray*)self, i, CMPI_uint64);
}

KINLINE KUint64 KUint64A_Get(const KUint64A* self, CMPICount i)
{
    KUint64 result;
    KArray_Get((const KArray*)self, i, CMPI_uint64, (KValue*)&result);
    return result;
}

KINLINE void KUint64A_Clr(KUint64A* self)
{
    KArray_Clr((KArray*)self);
}

#define KUINT64A_INIT { 0, 0, NULL, {0}, 0, {0} }

/*
**==============================================================================
**
** KSint64A
**
**==============================================================================
*/

typedef struct _KSint64A
{
    CMPIUint32 exists;
    CMPIUint32 null;
    const CMPIArray* value;
    char __padding1[16 - sizeof(CMPIArray*)];
    CMPIUint32 count;
    char __padding2[4];
}
KSint64A;

KINLINE CMPIBoolean KSint64A_Init(
    KSint64A* self, 
    const CMPIBroker* cb, 
    CMPICount max)
{
    return KArray_Init((KArray*)self, cb, max, CMPI_sint64);
}

KINLINE void KSint64A_InitNull(KSint64A* self)
{
    KArray_InitNull((KArray*)self);
}

KINLINE CMPIBoolean KSint64A_Set(
    KSint64A* self, 
    CMPICount i,
    CMPISint64 x)
{
    return KArray_Set((KArray*)self, i, &x, CMPI_sint64);
}

KINLINE CMPIBoolean KSint64A_Null(KSint64A* self, CMPICount i)
{
    return KArray_Null((KArray*)self, i, CMPI_sint64);
}

KINLINE KSint64 KSint64A_Get(const KSint64A* self, CMPICount i)
{
    KSint64 result;
    KArray_Get((const KArray*)self, i, CMPI_sint64, (KValue*)&result);
    return result;
}

KINLINE void KSint64A_Clr(KSint64A* self)
{
    KArray_Clr((KArray*)self);
}

#define KSINT64A_INIT { 0, 0, NULL, {0}, 0, {0} }

/*
**==============================================================================
**
** KReal32A
**
**==============================================================================
*/

typedef struct _KReal32A
{
    CMPIUint32 exists;
    CMPIUint32 null;
    const CMPIArray* value;
    char __padding1[16 - sizeof(CMPIArray*)];
    CMPIUint32 count;
    char __padding2[4];
}
KReal32A;

KINLINE CMPIBoolean KReal32A_Init(
    KReal32A* self, 
    const CMPIBroker* cb, 
    CMPICount max)
{
    return KArray_Init((KArray*)self, cb, max, CMPI_real32);
}

KINLINE void KReal32A_InitNull(KReal32A* self)
{
    KArray_InitNull((KArray*)self);
}

KINLINE CMPIBoolean KReal32A_Set(
    KReal32A* self, 
    CMPICount i,
    CMPIReal32 x)
{
    return KArray_Set((KArray*)self, i, &x, CMPI_real32);
}

KINLINE CMPIBoolean KReal32A_Null(KReal32A* self, CMPICount i)
{
    return KArray_Null((KArray*)self, i, CMPI_real32);
}

KINLINE KReal32 KReal32A_Get(const KReal32A* self, CMPICount i)
{
    KReal32 result;
    KArray_Get((const KArray*)self, i, CMPI_real32, (KValue*)&result);
    return result;
}

KINLINE void KReal32A_Clr(KReal32A* self)
{
    KArray_Clr((KArray*)self);
}

#define KREAL32A_INIT { 0, 0, NULL, {0}, 0, {0} }

/*
**==============================================================================
**
** KReal64A
**
**==============================================================================
*/

typedef struct _KReal64A
{
    CMPIUint32 exists;
    CMPIUint32 null;
    const CMPIArray* value;
    char __padding1[16 - sizeof(CMPIArray*)];
    CMPIUint32 count;
    char __padding2[4];
}
KReal64A;

KINLINE CMPIBoolean KReal64A_Init(
    KReal64A* self, 
    const CMPIBroker* cb, 
    CMPICount max)
{
    return KArray_Init((KArray*)self, cb, max, CMPI_real64);
}

KINLINE void KReal64A_InitNull(KReal64A* self)
{
    KArray_InitNull((KArray*)self);
}

KINLINE CMPIBoolean KReal64A_Set(
    KReal64A* self, 
    CMPICount i,
    CMPIReal64 x)
{
    return KArray_Set((KArray*)self, i, &x, CMPI_real64);
}

KINLINE CMPIBoolean KReal64A_Null(KReal64A* self, CMPICount i)
{
    return KArray_Null((KArray*)self, i, CMPI_real64);
}

KINLINE KReal64 KReal64A_Get(const KReal64A* self, CMPICount i)
{
    KReal64 result;
    KArray_Get((const KArray*)self, i, CMPI_real64, (KValue*)&result);
    return result;
}

KINLINE void KReal64A_Clr(KReal64A* self)
{
    KArray_Clr((KArray*)self);
}

#define KREAL64A_INIT { 0, 0, NULL, {0}, 0, {0} }

/*
**==============================================================================
**
** KChar16A
**
**==============================================================================
*/

typedef struct _KChar16A
{
    CMPIUint32 exists;
    CMPIUint32 null;
    const CMPIArray* value;
    char __padding1[16 - sizeof(CMPIArray*)];
    CMPIUint32 count;
    char __padding2[4];
}
KChar16A;

KINLINE CMPIBoolean KChar16A_Init(
    KChar16A* self, 
    const CMPIBroker* cb, 
    CMPICount max)
{
    return KArray_Init((KArray*)self, cb, max, CMPI_char16);
}

KINLINE void KChar16A_InitNull(KChar16A* self)
{
    KArray_InitNull((KArray*)self);
}

KINLINE CMPIBoolean KChar16A_Set(
    KChar16A* self, 
    CMPICount i,
    CMPIChar16 x)
{
    return KArray_Set((KArray*)self, i, &x, CMPI_char16);
}

KINLINE CMPIBoolean KChar16A_Null(KChar16A* self, CMPICount i)
{
    return KArray_Null((KArray*)self, i, CMPI_char16);
}

KINLINE KChar16 KChar16A_Get(const KChar16A* self, CMPICount i)
{
    KChar16 result;
    KArray_Get((const KArray*)self, i, CMPI_char16, (KValue*)&result);
    return result;
}

KINLINE void KChar16A_Clr(KChar16A* self)
{
    KArray_Clr((KArray*)self);
}

#define KCHAR16A_INIT { 0, 0, NULL, {0}, 0, {0} }

/*
**==============================================================================
**
** KStringA
**
**==============================================================================
*/

typedef struct _KStringA
{
    CMPIUint32 exists;
    CMPIUint32 null;
    const CMPIArray* value;
    char __padding1[16 - sizeof(CMPIArray*)];
    CMPIUint32 count;
    char __padding2[4];
}
KStringA;

KINLINE CMPIBoolean KStringA_Init(
    KStringA* self, 
    const CMPIBroker* cb, 
    CMPICount max)
{
    return KArray_Init((KArray*)self, cb, max, CMPI_string);
}

KINLINE void KStringA_InitNull(KStringA* self)
{
    KArray_InitNull((KArray*)self);
}

KINLINE CMPIBoolean KStringA_SetString(
    KStringA* self, 
    CMPICount i,
    CMPIString* x)
{
    return KArray_Set((KArray*)self, i, &x, CMPI_string);
}

KEXTERN CMPIBoolean KStringA_Set(
    KStringA* self, 
    const CMPIBroker* cb,
    CMPICount i,
    const char* s);

KINLINE CMPIBoolean KStringA_Null(KStringA* self, CMPICount i)
{
    return KArray_Null((KArray*)self, i, CMPI_string);
}

KINLINE KString KStringA_GetString(const KStringA* self, CMPICount i)
{
    KString result;

    KArray_Get((const KArray*)self, i, CMPI_string, (KValue*)&result);
    result.chars = KChars(result.value);

    return result;
}

KEXTERN const char* KStringA_Get(
    const KStringA* self,
    CMPICount i);

KINLINE void KStringA_Clr(KStringA* self)
{
    KArray_Clr((KArray*)self);
}

#define KSTRINGA_INIT { 0, 0, NULL, {0}, 0, {0} }

/*
**==============================================================================
**
** KDateTimeA
**
**==============================================================================
*/

typedef struct _KDateTimeA
{
    CMPIUint32 exists;
    CMPIUint32 null;
    const CMPIArray* value;
    char __padding1[16 - sizeof(CMPIArray*)];
    CMPIUint32 count;
    char __padding2[4];
}
KDateTimeA;

KINLINE CMPIBoolean KDateTimeA_Init(
    KDateTimeA* self, 
    const CMPIBroker* cb, 
    CMPICount max)
{
    return KArray_Init((KArray*)self, cb, max, CMPI_dateTime);
}

KINLINE void KDateTimeA_InitNull(KDateTimeA* self)
{
    KArray_InitNull((KArray*)self);
}

KINLINE CMPIBoolean KDateTimeA_Set(
    KDateTimeA* self, 
    CMPICount i,
    CMPIDateTime* x)
{
    return KArray_Set((KArray*)self, i, &x, CMPI_dateTime);
}

KINLINE CMPIBoolean KDateTimeA_Null(KDateTimeA* self, CMPICount i)
{
    return KArray_Null((KArray*)self, i, CMPI_dateTime);
}

KINLINE KDateTime KDateTimeA_Get(const KDateTimeA* self, CMPICount i)
{
    KDateTime result;
    KArray_Get((const KArray*)self, i, CMPI_dateTime, (KValue*)&result);
    return result;
}

KINLINE void KDateTimeA_Clr(KDateTimeA* self)
{
    KArray_Clr((KArray*)self);
}

KEXTERN CMPIBoolean KMatch(
    const CMPIObjectPath* cop1, 
    const CMPIObjectPath* cop2);

#define KDATETIMEA_INIT { 0, 0, NULL, {0}, 0, {0} }

/*
**==============================================================================
**
** KRefA
**
**==============================================================================
*/

typedef struct _KRefA
{
    CMPIUint32 exists;
    CMPIUint32 null;
    const CMPIArray* value;
    char __padding1[16 - sizeof(CMPIArray*)];
    CMPIUint32 count;
    char __padding2[4];
    const unsigned char* __sig;
    char __padding3[16 - sizeof(char*)];
}
KRefA;

KINLINE CMPIBoolean KRefA_Init(
    KRefA* self, 
    const CMPIBroker* cb, 
    CMPICount max)
{
    const unsigned char* sig = self ? self->__sig : NULL;

    if (!KArray_Init((KArray*)self, cb, max, CMPI_ref))
    {
        if (self)
            self->__sig = sig;
        return 0;
    }

    if (self)
        self->__sig = sig;

    return 1;
}

KINLINE void KRefA_InitNull(KRefA* self)
{
    const unsigned char* sig = self ? self->__sig : NULL;

    KArray_InitNull((KArray*)self);

    if (self)
        self->__sig = sig;
}

KINLINE CMPIBoolean KRefA_Set(
    KRefA* self, 
    CMPICount i,
    CMPIObjectPath* x)
{
    const unsigned char* sig = self ? self->__sig : NULL;

    if (!KArray_Set((KArray*)self, i, &x, CMPI_ref))
    {
        if (self)
            self->__sig = sig;
        return 0;
    }

    if (self)
        self->__sig = sig;

    return 1;
}

KINLINE CMPIBoolean KRefA_Null(KRefA* self, CMPICount i)
{
    const unsigned char* sig = self ? self->__sig : NULL;

    if (!KArray_Null((KArray*)self, i, CMPI_ref))
    {
        if (self)
            self->__sig = sig;
        return 0;
    }

    if (self)
        self->__sig = sig;

    return 1;
}

KINLINE KRef KRefA_Get(KRefA* self, CMPICount i)
{
    const unsigned char* sig = self ? self->__sig : NULL;
    KRef result;

    KArray_Get((KArray*)self, i, CMPI_ref, (KValue*)&result);

    if (self)
        self->__sig = sig;

    return result;
}

KINLINE void KRefA_Clr(KRefA* self)
{
    const unsigned char* sig = self ? self->__sig : NULL;

    KArray_Clr((KArray*)self);

    if (self)
        self->__sig = sig;
}

#define KREFA_INIT { 0, 0, NULL, {0}, 0, {0} }

/*
**==============================================================================
**
** KInstanceA
**
**==============================================================================
*/

typedef struct _KInstanceA
{
    CMPIUint32 exists;
    CMPIUint32 null;
    const CMPIArray* value;
    char __padding1[16 - sizeof(CMPIArray*)];
    CMPIUint32 count;
    char __padding2[4];
    const unsigned char* __sig;
    char __padding3[16 - sizeof(char*)];
}
KInstanceA;

KINLINE CMPIBoolean KInstanceA_Init(
    KInstanceA* self, 
    const CMPIBroker* cb, 
    CMPICount max)
{
    const unsigned char* sig = self ? self->__sig : NULL;

    if (!KArray_Init((KArray*)self, cb, max, CMPI_instance))
    {
        if (self)
            self->__sig = sig;
        return 0;
    }

    if (self)
        self->__sig = sig;

    return 1;
}

KINLINE void KInstanceA_InitNull(KInstanceA* self)
{
    const unsigned char* sig = self ? self->__sig : NULL;

    KArray_InitNull((KArray*)self);

    if (self)
        self->__sig = sig;
}

KINLINE CMPIBoolean KInstanceA_Set(
    KInstanceA* self, 
    CMPICount i,
    CMPIInstance* x)
{
    const unsigned char* sig = self ? self->__sig : NULL;

    if (!KArray_Set((KArray*)self, i, &x, CMPI_instance))
    {
        if (self)
            self->__sig = sig;
        return 0;
    }

    if (self)
        self->__sig = sig;

    return 1;
}

KINLINE CMPIBoolean KInstanceA_Null(KInstanceA* self, CMPICount i)
{
    const unsigned char* sig = self ? self->__sig : NULL;

    if (!KArray_Null((KArray*)self, i, CMPI_instance))
    {
        if (self)
            self->__sig = sig;
        return 0;
    }

    if (self)
        self->__sig = sig;

    return 1;
}

KINLINE KInstance KInstanceA_Get(KInstanceA* self, CMPICount i)
{
    const unsigned char* sig = self ? self->__sig : NULL;
    KInstance result;

    KArray_Get((KArray*)self, i, CMPI_instance, (KValue*)&result);

    if (self)
        self->__sig = sig;

    return result;
}

KINLINE void KInstanceA_Clr(KInstanceA* self)
{
    const unsigned char* sig = self ? self->__sig : NULL;

    KArray_Clr((KArray*)self);

    if (self)
        self->__sig = sig;
}

#define KINSTANCEA_INIT { 0, 0, NULL, {0}, 0, {0} }

/*
**==============================================================================
**
** KPos
**
**==============================================================================
*/

typedef struct _KPos
{
    const unsigned char* sig;
    const char* classname;
    size_t count;
    size_t index;
    const void* field;
    KTag tag;
    const char* name;
}
KPos;

KINLINE void KFirst(KPos* self, const KBase* base)
{
    size_t n;
    self->sig = base->sig;
    n = *self->sig++;
    self->classname = (char*)self->sig;
    self->sig += n + 1;
    self->count = *self->sig++;
    self->field = base + 1;
    self->tag = *self->sig++;
    n = *self->sig++;
    self->name = (char*)self->sig;
    self->sig += n + 1;
    self->index = 0;
}

KINLINE int KMore(const KPos* self)
{
    return self->index < self->count;
}

KINLINE void KNext(KPos* self)
{
    self->field = (char*)self->field + KTypeSize(self->tag);
    self->tag = *self->sig++;
    self->name = (char*)self->sig + 1;
    self->sig += *self->sig + 2;
    self->index++;
}

/*
**==============================================================================
**
** Return macros
**
**==============================================================================
*/

KINLINE CMPIStatus __KReturnInstance(const CMPIResult* result, KBase* base)
{
    CMPIStatus status;
    CMPIInstance* instance;

    if (!(instance = KBase_ToInstance(base, &status)))
        return status;

    CMReturnInstance(result, instance);
    KReturn(OK);
}

#define KReturnInstance(RESULT, INSTANCE) \
    do \
    { \
        CMPIStatus status = __KReturnInstance((RESULT), &(INSTANCE).__base); \
        if (!KOkay(status)) \
            return status; \
    } \
    while (0)

KINLINE CMPIStatus __KReturnObjectPath(const CMPIResult* result, KBase* base)
{
    CMPIStatus status;
    CMPIObjectPath* path;

    if (!(path = KBase_ToObjectPath(base, &status)))
        return status;

    CMReturnObjectPath(result, path);
    KReturn(OK);
}

#define KReturnObjectPath(RESULT, PATH) \
    do \
    { \
        CMPIStatus status = __KReturnObjectPath((RESULT), &(PATH).__base); \
        if (!KOkay(status)) \
            return status; \
    } \
    while (0)

KINLINE CMPIStatus __KReturnData(const CMPIResult* st, KValue* kv, CMPIType ct)
{
    if (kv->null)
        return CMReturnData(st, NULL, ct);
    else
        return CMReturnData(st, (CMPIValue*)&kv->u, ct);
}

KINLINE CMPIStatus KReturnBooleanData(const CMPIResult* st, const KBoolean* kv)
{
    return __KReturnData(st, (KValue*)kv, CMPI_boolean);
}

KINLINE CMPIStatus KReturnUint8Data(const CMPIResult* st, const KUint8* kv)
{
    return __KReturnData(st, (KValue*)kv, CMPI_uint8);
}

KINLINE CMPIStatus KReturnSint8Data(const CMPIResult* st, const KSint8* kv)
{
    return __KReturnData(st, (KValue*)kv, CMPI_sint8);
}

KINLINE CMPIStatus KReturnUint16Data(const CMPIResult* st, const KUint16* kv)
{
    return __KReturnData(st, (KValue*)kv, CMPI_uint16);
}

KINLINE CMPIStatus KReturnSint16Data(const CMPIResult* st, const KSint16* kv)
{
    return __KReturnData(st, (KValue*)kv, CMPI_sint16);
}

KINLINE CMPIStatus KReturnUint32Data(const CMPIResult* st, const KUint32* kv)
{
    return __KReturnData(st, (KValue*)kv, CMPI_uint32);
}

KINLINE CMPIStatus KReturnSint32Data(const CMPIResult* st, const KSint32* kv)
{
    return __KReturnData(st, (KValue*)kv, CMPI_sint32);
}

KINLINE CMPIStatus KReturnUint64Data(const CMPIResult* st, const KUint64* kv)
{
    return __KReturnData(st, (KValue*)kv, CMPI_uint64);
}

KINLINE CMPIStatus KReturnSint64Data(const CMPIResult* st, const KSint64* kv)
{
    return __KReturnData(st, (KValue*)kv, CMPI_sint64);
}

KINLINE CMPIStatus KReturnReal32Data(const CMPIResult* st, const KReal32* kv)
{
    return __KReturnData(st, (KValue*)kv, CMPI_real32);
}

KINLINE CMPIStatus KReturnReal64Data(const CMPIResult* st, const KReal64* kv)
{
    return __KReturnData(st, (KValue*)kv, CMPI_real64);
}

KINLINE CMPIStatus KReturnChar16Data(const CMPIResult* st, const KChar16* kv)
{
    return __KReturnData(st, (KValue*)kv, CMPI_char16);
}

KINLINE CMPIStatus KReturnStringData(const CMPIResult* st, const KString* kv)
{
    return __KReturnData(st, (KValue*)kv, CMPI_string);
}

KINLINE CMPIStatus KReturnDateTimeData(
    const CMPIResult* st, const KDateTime* kv)
{
    return __KReturnData(st, (KValue*)kv, CMPI_dateTime);
}

#define KReturnIf(EXPR) \
    do \
    { \
        CMPIStatus __st = (EXPR); \
        if (__st.rc != CMPI_RC_OK) \
            return __st; \
    } \
    while (0)

/*
**==============================================================================
**
** Default provider operations
**
**==============================================================================
*/

KEXTERN CMPIStatus KDefaultEnumerateInstanceNames( 
    const CMPIBroker* mb,
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop);

KEXTERN CMPIStatus KDefaultGetInstance( 
    const CMPIBroker* mb,
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties);

KEXTERN CMPIStatus KDefaultAssociatorNames(
    const CMPIBroker* mb,
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* thisClass,
    const char* assocClass,
    const char* resultClass,
    const char* role,
    const char* resultRole);

KEXTERN CMPIStatus KDefaultAssociators(
    const CMPIBroker* mb,
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* thisClass,
    const char* assocClass,
    const char* resultClass,
    const char* role,
    const char* resultRole,
    const char** properties);

KEXTERN CMPIStatus KDefaultReferenceNames(
    const CMPIBroker* mb,
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* thisClass,
    const char* assocClass,
    const char* role);

KEXTERN CMPIStatus KDefaultReferences(
    const CMPIBroker* mb,
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* thisClass,
    const char* assocClass,
    const char* role,
    const char** properties);

KEXTERN CMPIStatus KDefaultEnumerateInstancesOneToAll(
    const CMPIBroker* cb,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* assocCop,
    const CMPIObjectPath* fromCop,
    const char* fromRole,
    const CMPIObjectPath* toCop,
    const char* toRole);

/*
**==============================================================================
**
** Convenience functions
**
**==============================================================================
*/

KEXTERN CMPIStatus KPrintObjectPath(
    FILE* os,
    const CMPIObjectPath* self, 
    size_t n);

KEXTERN CMPIStatus KPrintInstance(
    FILE* os,
    const CMPIInstance* self, 
    size_t n);

KEXTERN CMPIStatus KPrintArgs(
    FILE* os,
    const CMPIArgs* self, 
    size_t n);

KEXTERN CMPIData KFindKey(
    const CMPIObjectPath* cop, 
    const char* name,
    CMPIStatus* status);

KEXTERN CMPIData KFindProperty(
    const CMPIInstance* ci, 
    const char* name,
    CMPIStatus* status);

/*
**==============================================================================
**
** KStrlcpy() and KStrlcat()
**
**==============================================================================
*/

KEXTERN size_t KStrlcpy(char* dest, const char* src, size_t size);

KEXTERN size_t KStrlcat(char* dest, const char* src, size_t size);

/*
**==============================================================================
**
** KONKRET_REGISTRATION
**
**==============================================================================
*/

#define KONKRET_REGISTRATION(NAMESPACE, CLASS, PROVIDERNAME, TYPES) \
    static volatile KUNUSED const char __konkret_registration[] = \
    "@(#)KONKRET_REGISTRATION=" NAMESPACE ":" CLASS ":" PROVIDERNAME ":" TYPES;

#endif /* _konkret_h */
