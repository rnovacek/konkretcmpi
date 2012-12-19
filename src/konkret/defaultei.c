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

CMPIStatus KDefaultEnumerateInstancesOneToAll(
    const CMPIBroker* cb,
    const CMPIContext* cc, 
    const CMPIResult* cr, 
    const CMPIObjectPath* assocCop,
    const CMPIObjectPath* fromCop,
    const char* fromRole,
    const CMPIObjectPath* toCop,
    const char* toRole)
{
    CMPIEnumeration* e;
    CMPIStatus st;

    /* Check args */

    if (!cb || !cc || !cr || !assocCop || !fromCop || !fromRole || !toCop || 
        !toRole)
    {
        KReturn(ERR_FAILED);
    }

    /* Enumerate instances names of toCop */

    if (!(e = cb->bft->enumerateInstanceNames(cb, cc, toCop, &st)))
    {
        KReturn(ERR_FAILED);
    }

    while (CMHasNext(e, &st))
    {
        CMPIData cd;
        CMPIObjectPath* cop;
        CMPIInstance* ci;

        /* Get next instance name */

        cd = CMGetNext(e, &st);

        if (st.rc || cd.type != CMPI_ref || (cd.state & CMPI_nullValue))
        {
            KReturn(ERR_FAILED);
        }

        /* Create association object path */

        if (!(cop = CMNewObjectPath(cb, KNameSpace(assocCop), 
            KClassName(assocCop), &st)) || st.rc)
        {
            KReturn(ERR_FAILED);
        }

        if (CMAddKey(cop, fromRole, (CMPIValue*)&fromCop, CMPI_ref).rc)
        {
            KReturn(ERR_FAILED);
        }

        if (CMAddKey(cop, toRole, (CMPIValue*)&cd.value.ref, CMPI_ref).rc)
        {
            KReturn(ERR_FAILED);
        }

        /* Create association instance */

        if (!(ci = CMNewInstance(cb, cop, &st)))
        {
            KReturn(ERR_FAILED);
        }

        if (CMSetProperty(ci, fromRole, (CMPIValue*)&fromCop, CMPI_ref).rc)
        {
            KReturn(ERR_FAILED);
        }

        if (CMSetProperty(ci, toRole, (CMPIValue*)&cd.value.ref, CMPI_ref).rc)
        {
            KReturn(ERR_FAILED);
        }

        CMReturnInstance(cr, ci);
    }

    KReturn(OK);
}
