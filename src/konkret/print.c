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

static void _indent(FILE* os, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++)
        fprintf(os, "    ");
}

static void _print_scalar(
    FILE* os,
    const CMPIData* cd, 
    size_t n)
{
    if (cd->state & CMPI_nullValue)
    {
        fprintf(os, "null");
        return;
    }

    switch (cd->type & ~CMPI_ARRAY)
    {
        case CMPI_boolean:
            fprintf(os, "%s", cd->value.boolean ? "true" : "false");
            break;
        case CMPI_uint8:
            fprintf(os, "%u", cd->value.uint8);
            break;
        case CMPI_sint8:
            fprintf(os, "%d", cd->value.sint8);
            break;
        case CMPI_uint16:
            fprintf(os, "%u", cd->value.uint16);
            break;
        case CMPI_sint16:
            fprintf(os, "%d", cd->value.sint16);
            break;
        case CMPI_uint32:
            fprintf(os, "%u", cd->value.uint32);
            break;
        case CMPI_sint32:
            fprintf(os, "%d", cd->value.sint32);
            break;
        case CMPI_uint64:
            fprintf(os, "%llu", cd->value.uint64);
            break;
        case CMPI_sint64:
            fprintf(os, "%lld", cd->value.sint64);
            break;
        case CMPI_real32:
            fprintf(os, "%f", cd->value.real32);
            break;
        case CMPI_real64:
            fprintf(os, "%f", cd->value.real64);
            break;
        case CMPI_char16:
            fprintf(os, "%u", cd->value.char16);
            break;
        case CMPI_string:
            /* ATTN: improve string formatting */
            fprintf(os, "\"%s\"", KChars(cd->value.string));
            break;
        case CMPI_dateTime:
            fprintf(os, "%s", 
                KChars(CMGetStringFormat(cd->value.dateTime, NULL)));
            break;
        case CMPI_ref:
            KPrintObjectPath(os, cd->value.ref, n);
            break;
        case CMPI_instance:
            KPrintInstance(os, cd->value.inst, n);
            break;
    }
}

static void _print_array(FILE* os, const CMPIData* cd, size_t n)
{
    CMPICount count;
    CMPIArray* array;
    CMPICount i;

    if (cd->state & CMPI_nullValue)
    {
        fprintf(os, "null");
        return;
    }

    if (!(array = cd->value.array))
        return;

    fprintf(os, "{");

    count = CMGetArrayCount(array, NULL);

    for (i = 0; i < count; i++)
    {
        CMPIStatus st = KSTATUS_INIT;
        CMPIData cd = CMGetArrayElementAt(array, i, &st);

        if (KOkay(st))
        {
            _print_scalar(os, &cd, n);

            if (i + 1 != count)
                fprintf(os, ", ");
        }
    }

    fprintf(os, "}");
}

CMPIStatus KPrintObjectPath(
    FILE* os,
    const CMPIObjectPath* self, 
    size_t n)
{
    CMPIString* cn;
    CMPIString* ns;
    CMPIStatus st = KSTATUS_INIT;
    CMPICount count;
    CMPICount i;

    if (!self)
        KReturn(ERR_FAILED);

    /* Get namespace */

    if (!(ns = CMGetNameSpace(self, &st)))
        return st;

    /* Get classname */

    if (!(cn = CMGetClassName(self, &st)))
        return st;

    /* Print namesapce:classname */

    fprintf(os, "%s:%s\n", KChars(ns), KChars(cn));
    _indent(os, n);
    fprintf(os, "{\n");
    n++;

    /* Print properties */

    count = CMGetKeyCount(self, &st);

    if (!KOkay(st))
        return st;

    for (i = 0; i < count; i++)
    {
        CMPIData cd;
        CMPIString* pn = NULL;

        /* Get i-th property */

        cd = CMGetKeyAt(self, i, &pn, &st);

        if (!KOkay(st))
            return st;

        if (!pn)
            KReturn(ERR_FAILED);

        /* Print property name */

        _indent(os, n);
        fprintf(os, "%s=", KChars(pn));

        /* Print property value */

        if (cd.type & CMPI_ARRAY)
            _print_array(os, &cd, n);
        else
            _print_scalar(os, &cd, n);

        fprintf(os, "\n");
    }

    n--;
    _indent(os, n);
    fprintf(os, "}\n");

    KReturn(OK);
}

CMPIStatus KPrintInstance(
    FILE* os,
    const CMPIInstance* self, 
    size_t n)
{
    CMPIObjectPath* cop;
    CMPIString* cn;
    CMPIString* ns;
    CMPIStatus st = KSTATUS_INIT;
    CMPICount count;
    CMPICount i;

    if (!self)
        KReturn(ERR_FAILED);

    /* Get object path */

    if (!(cop = CMGetObjectPath(self, &st)))
        return st;

    /* Get namespace */

    if (!(ns = CMGetNameSpace(cop, &st)))
    {
        /* Ignore */
    }

    /* Get classname */

    if (!(cn = CMGetClassName(cop, &st)))
        return st;

    /* Print namesapce:classname */

    fprintf(os, "%s:%s\n", KChars(ns), KChars(cn));
    _indent(os, n);
    fprintf(os, "{\n");
    n++;

    /* Print properties */

    count = CMGetPropertyCount(self, &st);

    if (!KOkay(st))
        return st;

    for (i = 0; i < count; i++)
    {
        CMPIData cd;
        CMPIString* pn = NULL;

        /* Get i-th property */

        cd = CMGetPropertyAt(self, i, &pn, &st);

        if (!KOkay(st))
            return st;

        if (!pn)
            KReturn(ERR_FAILED);

        /* Print property name */

        _indent(os, n);
        fprintf(os, "%s=", KChars(pn));

        /* Print property value */

        if (cd.type & CMPI_ARRAY)
            _print_array(os, &cd, n);
        else
            _print_scalar(os, &cd, n);

        if (cd.type != CMPI_ref || cd.state & CMPI_nullValue)
            fprintf(os, "\n");
    }

    n--;
    _indent(os, n);
    fprintf(os, "}\n");

    KReturn(OK);
}

CMPIStatus KPrintArgs(
    FILE* os,
    const CMPIArgs* self, 
    size_t n)
{
    CMPIStatus st = KSTATUS_INIT;
    CMPICount count;
    CMPICount i;

    if (!self)
        KReturn(ERR_FAILED);

    /* Print args name */

    fprintf(os, "Args\n");
    _indent(os, n);
    fprintf(os, "{\n");
    n++;

    /* Print properties */

    count = CMGetArgCount(self, &st);

    if (!KOkay(st))
        return st;

    for (i = 0; i < count; i++)
    {
        CMPIData cd;
        CMPIString* pn = NULL;

        /* Get i-th property */

        cd = CMGetArgAt(self, i, &pn, &st);

        if (!KOkay(st))
            return st;

        if (!pn)
            KReturn(ERR_FAILED);

        /* Print property name */

        _indent(os, n);
        fprintf(os, "%s=", KChars(pn));

        /* Print property value */

        if (cd.type & CMPI_ARRAY)
            _print_array(os, &cd, n);
        else
            _print_scalar(os, &cd, n);

        fprintf(os, "\n");
    }

    n--;
    _indent(os, n);
    fprintf(os, "}\n");

    KReturn(OK);
}

CMPIStatus KBase_Print(FILE* os, const KBase* self, char type)
{
    CMPIStatus st = KSTATUS_INIT;

    if (type == 'r')
    {
        CMPIObjectPath* cop;

        if (!(cop = KBase_ToObjectPath(self, &st)))
            return st;

        return KPrintObjectPath(os, cop, 0);
    }
    else if (type == 'i')
    {
        CMPIInstance* ci;

        if (!(ci = KBase_ToInstance(self, &st)))
            return st;

        return KPrintInstance(os, ci, 0);
    }
    else if (type == 'a')
    {
        CMPIArgs* ca;

        if (!(ca = KBase_ToArgs(self, 1, 1, &st)))
            return st;

        return KPrintArgs(os, ca, 0);
    }

    KReturn(ERR_FAILED);
}
