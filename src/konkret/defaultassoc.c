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

#include <strings.h>

#define MAX_REFS 32

typedef void (*FindCallback)(
    const CMPIBroker* mb,
    const CMPIContext* cc,
    const CMPIResult* cr,
    CMPIObjectPath* cop,
    void* client_data);

static CMPIStatus _find_associators(
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
    FindCallback callback,
    void* client_data)
{
    CMPIEnumeration* en;
    CMPIObjectPath* ccop; /* class cop */
    CMPIStatus st;

    /* Reject null thisClass */

    if (!thisClass)
        CMReturn(CMPI_RC_ERR_FAILED);

    /* Create a "class" object path to represent just the class */

    ccop = CMNewObjectPath(mb, KNameSpace(cop), thisClass, &st);

    if (!ccop || st.rc)
        return st;

    /* Return now if thisClass is not an assocClass */

    if (assocClass && !CMClassPathIsA(mb, ccop, assocClass, NULL))
        CMReturn(CMPI_RC_OK);

    /* Enumerate all instance names of the association class */

    en = mb->bft->enumerateInstanceNames(mb, cc, ccop, &st);

    if (!en || st.rc)
        return st;

    while (CMHasNext(en, &st))
    {
        CMPIData cd;
        CMPIObjectPath* acop; /* association cop */
        CMPICount count;
        CMPICount i;
        CMPIObjectPath* refs[MAX_REFS];
        CMPICount r = 0;
        CMPIBoolean found = 0;

        /* Get the next instance name */

        cd = CMGetNext(en, &st);

        if (st.rc)
        {
            return st;
        }

        if (cd.type != CMPI_ref)
        {
            continue;
        }

        if (!cd.value.ref)
        {
            continue;
        }

        acop = cd.value.ref;

        /* Build a list of references in this object path */

        count = CMGetKeyCount(acop, &st);

        if (st.rc)
        {
            return st;
        }

        for (i = 0; i < count; i++)
        {
            CMPIString* pn = NULL;
            cd = CMGetKeyAt(acop, i, &pn, &st);

            if (st.rc || cd.type != CMPI_ref || !cd.value.ref ||
                CMIsNullValue(cd))
            {
                continue;
            }

            if (r == MAX_REFS)
            {
                CMReturn(CMPI_RC_ERR_FAILED);
            }

            /* Match the two object paths */

            if (!found && KMatch(cop, cd.value.ref))
            {
                if (role)
                {
                    const char* tmp = KChars(pn);

                    if (tmp && strcasecmp(tmp, role) == 0)
                        found = 1;
                }
                else
                    found = 1;
                continue;
            }

            /* Check result class */

            if (resultClass)
            {
                const char* tmp = KClassName(cd.value.ref);

                if (!tmp || strcasecmp(tmp, resultClass) != 0)
                {
                    continue;
                }
            }

            /* Check result role */

            if (resultRole)
            {
                const char* tmp = KChars(pn);

                if (!tmp || strcasecmp(tmp, resultRole) != 0)
                {
                    continue;
                }
            }

            /* Save object path */

            refs[r++] = cd.value.ref;
        }

        /* If "from" name, then deliver "to" names. */

        if (found)
        {
            for (i = 0; i < r; i++)
            {
                (*callback)(mb, cc, cr, refs[i], NULL);
            }
        }
    }

    CMReturn(CMPI_RC_OK);
}

static void _deliver_object_path_callback(
    const CMPIBroker* mb,
    const CMPIContext* cc,
    const CMPIResult* cr,
    CMPIObjectPath* cop,
    void* client_data)
{
    CMReturnObjectPath(cr, cop);
}

CMPIStatus KDefaultAssociatorNames(
    const CMPIBroker* mb,
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* thisClass,
    const char* assocClass,
    const char* resultClass,
    const char* role,
    const char* resultRole)
{
    return _find_associators(mb, mi, cc, cr, cop, thisClass, assocClass, 
        resultClass, role, resultRole, _deliver_object_path_callback, NULL);
}

static void _deliver_instance_callback(
    const CMPIBroker* mb,
    const CMPIContext* cc,
    const CMPIResult* cr,
    CMPIObjectPath* cop,
    void* client_data)
{
    const char** properties = (const char**)client_data;
    CMPIInstance* ci;
    CMPIStatus st;

    ci = mb->bft->getInstance(mb, cc, cop, properties, &st);

    if (ci && KOkay(st))
    {
        CMReturnInstance(cr, ci);
    }
}

CMPIStatus KDefaultAssociators(
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
    const char** properties)
{
    return _find_associators(mb, mi, cc, cr, cop, thisClass, assocClass, 
        resultClass, role, resultRole, _deliver_instance_callback, properties);
}

static CMPIStatus _find_references(
    const CMPIBroker* mb,
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* thisClass,
    const char* assocClass,
    const char* role,
    FindCallback callback,
    void* client_data)
{
    CMPIEnumeration* en;
    CMPIObjectPath* ccop; /* class cop */
    CMPIStatus st;

    /* Reject null thisClass */

    if (!thisClass)
        CMReturn(CMPI_RC_ERR_FAILED);

    /* Create a "class" object path to represent just the class */

    ccop = CMNewObjectPath(mb, KNameSpace(cop), thisClass, &st);

    if (!ccop || st.rc)
        return st;

    /* Return now if thisClass is not an assocClass */

    if (assocClass && !CMClassPathIsA(mb, ccop, assocClass, NULL))
        CMReturn(CMPI_RC_OK);

    /* Enumerate all instance names of the association class */

    en = mb->bft->enumerateInstanceNames(mb, cc, ccop, &st);

    if (!en || st.rc)
        return st;

    while (CMHasNext(en, &st)) /* for each association instance */
    {
        CMPIData cd;
        CMPIObjectPath* acop; /* association cop */
        CMPICount count;
        CMPICount i;

        /* Get the next association instance name */

        cd = CMGetNext(en, &st);

        if (st.rc)
            return st;

        if (cd.type != CMPI_ref)
            continue;

        if (!cd.value.ref)
            continue;

        acop = cd.value.ref;

        /* Build a list of assocation instance names that refer to cop */

        count = CMGetKeyCount(acop, &st);

        if (st.rc)
            return st;

        for (i = 0; i < count; i++) /* for each */
        {
            CMPIString* pn = NULL;
            cd = CMGetKeyAt(acop, i, &pn, &st);

            if (st.rc || cd.type != CMPI_ref || !cd.value.ref)
            {
                continue;
            }

            // ATTN: CMIsNullValue(cd) is broken.

            /* Match the two object paths */

            if (KMatch(cop, cd.value.ref))
            {
                CMPIBoolean found = 0;

                if (role)
                {
                    const char* tmp = KChars(pn);

                    if (tmp && strcasecmp(tmp, role) == 0)
                        found = 1;
                }
                else
                    found = 1;

                if (found)
                {
                    (*callback)(mb, cc, cr, acop, NULL);
                    break;
                }
            }
        }
    }

    CMReturn(CMPI_RC_OK);
}

CMPIStatus KDefaultReferenceNames(
    const CMPIBroker* mb,
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* thisClass,
    const char* assocClass,
    const char* role)
{
    return _find_references(mb, mi, cc, cr, cop, thisClass, assocClass, role, 
        _deliver_object_path_callback, NULL);
}

CMPIStatus KDefaultReferences(
    const CMPIBroker* mb,
    CMPIAssociationMI* mi,
    const CMPIContext* cc,
    const CMPIResult* cr,
    const CMPIObjectPath* cop,
    const char* thisClass,
    const char* assocClass,
    const char* role,
    const char** properties)
{
    return _find_references(mb, mi, cc, cr, cop, thisClass, assocClass, role, 
        _deliver_instance_callback, properties);
}
