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

#define enumInstances enumerateInstances
#define enumInstanceNames enumerateInstanceNames
#include "konkret.h"

#define DIRECT_CALL

#if defined(DIRECT_CALL)

typedef struct _DefaultGI_Handle
{
    const CMPIResult* result;
    const CMPIObjectPath* cop;
    int found;
}
DefaultGI_Handle;

typedef struct _DefaultGI_Result
{
    void* hdl;
    CMPIResultFT* ft;
}
DefaultGI_Result;

static CMPIStatus _DefaultGI_release(
    CMPIResult* self)
{
    KReturn(OK);
}

static CMPIResult* _DefaultGI_clone(
    const CMPIResult* self, 
    CMPIStatus* status)
{
    if (status)
        KSetStatus(status, ERR_FAILED);

    return NULL;
}

static CMPIStatus _DefaultGI_returnData(
    const CMPIResult* self, 
    const CMPIValue* value, 
    const CMPIType type)
{
    KReturn(ERR_FAILED);
}

static CMPIStatus _DefaultGI_returnInstance(
    const CMPIResult* self, 
    const CMPIInstance* ci)
{
    DefaultGI_Handle* handle = 
        (DefaultGI_Handle*)(((DefaultGI_Result*)self)->hdl);
    const CMPIResult* result = handle->result;
    const CMPIObjectPath* cop;
    CMPIStatus status;

    if (!(cop = CMGetObjectPath(ci, &status)))
        return status;

    if (KMatch(cop, handle->cop))
    {
        handle->found = 1;
        return result->ft->returnInstance(result, ci);
    }

    KReturn(OK);
}

static CMPIStatus _DefaultGI_returnObjectPath(
    const CMPIResult* self, 
    const CMPIObjectPath* cop)
{
    KReturn(ERR_FAILED);
}

static CMPIStatus _DefaultGI_returnDone(
    const CMPIResult * self)
{
    KReturn(ERR_FAILED);
}

static CMPIStatus _DefaultGI_returnError(
    const CMPIResult* self, 
    const CMPIError* err)
{
    KReturn(ERR_FAILED);
}

#endif /* defined(DIRECT_CALL) */

CMPIStatus KDefaultGetInstance( 
    const CMPIBroker* mb,
    CMPIInstanceMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char** properties)
{
#if defined(DIRECT_CALL)

    static CMPIResultFT _ft =
    {
        CMPICurrentVersion,
        _DefaultGI_release,
        _DefaultGI_clone,
        _DefaultGI_returnData,
        _DefaultGI_returnInstance,
        _DefaultGI_returnObjectPath,
        _DefaultGI_returnDone,
        _DefaultGI_returnError
    };
    DefaultGI_Result result;
    DefaultGI_Handle handle;
    CMPIStatus st;

    handle.result = cr;
    handle.cop = cop;
    handle.found = 0;

    result.hdl = (void*)&handle;
    result.ft = &_ft;

    st = (*mi->ft->enumerateInstances)(
        mi, cc, (CMPIResult*)(void*)&result, cop, NULL);

    if (!st.rc)
        return st;

    if (!handle.found)
    {
        KReturn(ERR_NOT_FOUND);
    }

    KReturn(OK);

#else /* defined(DIRECT_CALL) */

    CMPIEnumeration* e;
    CMPIStatus st;
    CMPIObjectPath* ccop;

    /* Create an object path with just the class from cop */

    ccop = CMNewObjectPath(mb, KNameSpace(cop), KClassName(cop), &st);

    if (!ccop)
        KReturn(ERR_FAILED);

    /* Enumerate all instances of this class */

    if (!(e = mb->bft->enumerateInstances(mb, cc, ccop, properties, &st)))
        KReturn(ERR_FAILED);

    while (CMHasNext(e, &st))
    {
        CMPIData cd;
        CMPIObjectPath* tcop;

        cd = CMGetNext(e, &st);

        if (st.rc || cd.type != CMPI_instance || (cd.state & CMPI_nullValue))
            KReturn(ERR_FAILED);

        if (!(tcop = CMGetObjectPath(cd.value.inst, &st)))
            KReturn(ERR_FAILED);

        if (KMatch(tcop, cop))
        {
            CMReturnInstance(cr, cd.value.inst);
            KReturn(OK);
        }
    }

    KReturn(ERR_NOT_FOUND);

#endif /* !defined(DIRECT_CALL) */
}
