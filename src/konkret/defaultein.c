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

typedef struct _DefaultEIN_Result
{
    void* hdl;
    CMPIResultFT* ft;
}
DefaultEIN_Result;

static CMPIStatus _DefaultEIN_release(
    CMPIResult* self)
{
    KReturn(OK);
}

static CMPIResult* _DefaultEIN_clone(
    const CMPIResult* self, 
    CMPIStatus* status)
{
    if (status)
        KSetStatus(status, ERR_FAILED);

    return NULL;
}

static CMPIStatus _DefaultEIN_returnData(
    const CMPIResult* self, 
    const CMPIValue* value, 
    const CMPIType type)
{
    KReturn(ERR_FAILED);
}

static CMPIStatus _DefaultEIN_returnInstance(
    const CMPIResult* self, 
    const CMPIInstance* ci)
{
    CMPIResult* result = (CMPIResult*)(((DefaultEIN_Result*)self)->hdl);
    CMPIObjectPath* cop;
    CMPIStatus st;

    if (!(cop = CMGetObjectPath(ci, &st)) || !KOkay(st))
        return st;

    return result->ft->returnObjectPath(result, cop);
}

static CMPIStatus _DefaultEIN_returnObjectPath(
    const CMPIResult* self, 
    const CMPIObjectPath* cop)
{
    KReturn(ERR_FAILED);
}

static CMPIStatus _DefaultEIN_returnDone(
    const CMPIResult * self)
{
    KReturn(ERR_FAILED);
}

#ifdef CMPI_VER_200
static CMPIStatus _DefaultEIN_returnError(
    const CMPIResult* self, 
    const CMPIError* err)
{
    KReturn(ERR_FAILED);
}
#endif

CMPIStatus KDefaultEnumerateInstanceNames( 
    const CMPIBroker* mb,
    CMPIInstanceMI* mi, 
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* cop) 
{
    static CMPIResultFT _ft =
    {
        CMPICurrentVersion,
        _DefaultEIN_release,
        _DefaultEIN_clone,
        _DefaultEIN_returnData,
        _DefaultEIN_returnInstance,
        _DefaultEIN_returnObjectPath,
        _DefaultEIN_returnDone,
#ifdef CMPI_VER_200
        _DefaultEIN_returnError
#endif
    };
    DefaultEIN_Result result;

    result.hdl = (void*)cr;
    result.ft = &_ft;

    return (*mi->ft->enumerateInstances)(
        mi, cc, (CMPIResult*)(void*)&result, cop, NULL);
}
