/* ========================================================================
 * Copyright (c) 2005-2009 The OPC Foundation, Inc. All rights reserved.
 *
 * OPC Reciprocal Community License ("RCL") Version 1.00
 * 
 * Unless explicitly acquired and licensed from Licensor under another 
 * license, the contents of this file are subject to the Reciprocal 
 * Community License ("RCL") Version 1.00, or subsequent versions as 
 * allowed by the RCL, and You may not copy or use this file in either 
 * source code or executable form, except in compliance with the terms and 
 * conditions of the RCL.
 * 
 * All software distributed under the RCL is provided strictly on an 
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, 
 * AND LICENSOR HEREBY DISCLAIMS ALL SUCH WARRANTIES, INCLUDING WITHOUT 
 * LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
 * PURPOSE, QUIET ENJOYMENT, OR NON-INFRINGEMENT. See the RCL for specific 
 * language governing rights and limitations under the RCL.
 *
 * The complete license agreement can be found here:
 * http://opcfoundation.org/License/RCL/1.00/
 * ======================================================================*/

/* platform */
#include <opcua.h>

#ifdef OPCUA_HAVE_SERVERAPI

/* core */
#include <opcua_utilities.h>
#include <opcua_datetime.h>

/* types */
#include <opcua_types.h>

/* serializing */
#include <opcua_binaryencoder.h>

/* self */
#include <opcua_endpoint.h>
#include <opcua_servicetable.h>

/*============================================================================
 * OpcUa_ServiceType_Compare
 *===========================================================================*/
static int OpcUa_ServiceType_Compare(const OpcUa_Void* a_pElement1, const OpcUa_Void* a_pElement2)
{
    OpcUa_ServiceType* pType1 = (OpcUa_ServiceType*)a_pElement1;
    OpcUa_ServiceType* pType2 = (OpcUa_ServiceType*)a_pElement2;

    if (pType1 == OpcUa_Null && pType2 != OpcUa_Null)
    {
        return -1;
    }

    if (pType1 == OpcUa_Null)
    {
        return +1;
    }
    
    if (pType1->RequestTypeId < pType2->RequestTypeId)
    {
        return -1;
    }
    
    if (pType1->RequestTypeId > pType2->RequestTypeId)
    {
        return +1;
    }

    return 0;
}

/*============================================================================
 * OpcUa_ServiceTable_Initialize
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServiceTable_AddTypes(   OpcUa_ServiceTable* a_pTable,
                                                OpcUa_ServiceType** a_pTypes)
{
    OpcUa_Int32         ii          = 0;
    OpcUa_UInt32        uCount      = 0;
    OpcUa_ServiceType*  pEntries    = OpcUa_Null;
    OpcUa_DeclareErrorTraceModule(OpcUa_Module_ServiceTable);

    /* check for nulls */
    OpcUa_ReturnErrorIfArgumentNull(a_pTable);
    OpcUa_ReturnErrorIfArgumentNull(a_pTypes);

    /* count the number new definitions */
    for (ii = 0; a_pTypes[ii] != OpcUa_Null; ii++);

    if (ii == 0)
    {
        return OpcUa_Good;
    }

    uCount = a_pTable->Count + ii;

    /* reallocate the table */
    pEntries = (OpcUa_ServiceType *)OpcUa_ReAlloc(a_pTable->Entries, uCount*sizeof(OpcUa_ServiceType));
    OpcUa_ReturnErrorIfAllocFailed(pEntries);
    
    /* copy new definitions */
    for (ii = a_pTable->Count; ii < (OpcUa_Int32)uCount; ii++)
    {
        OpcUa_MemCpy(pEntries+ii, sizeof(OpcUa_ServiceType), a_pTypes[ii], sizeof(OpcUa_ServiceType));
    }

    /* sort the table */

    OpcUa_QSort(    pEntries, 
                    uCount, 
                    sizeof(OpcUa_ServiceType), 
                    OpcUa_ServiceType_Compare, 
                    OpcUa_Null);

    /* save the new table */
    a_pTable->Entries = pEntries;
    a_pTable->Count   = uCount;

    return OpcUa_Good;
}

/*============================================================================
 * OpcUa_ServiceTable_Clear
 *===========================================================================*/
OpcUa_Void OpcUa_ServiceTable_Clear(OpcUa_ServiceTable* a_pTable)
{
    if (a_pTable != OpcUa_Null)
    {
        OpcUa_Free(a_pTable->Entries);
        a_pTable->Entries = OpcUa_Null;
        a_pTable->Count   = 0;
    }
}

/*============================================================================
 * OpcUa_ServiceTable_FindService
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServiceTable_FindService(
    OpcUa_ServiceTable* a_pTable,
    OpcUa_UInt32        a_uTypeId,
    OpcUa_ServiceType*  a_pType)
{       
    OpcUa_ServiceType   cKey;
    OpcUa_ServiceType*  pType   = OpcUa_Null;
    OpcUa_DeclareErrorTraceModule(OpcUa_Module_ServiceTable);

    /* check for nulls */   
    OpcUa_ReturnErrorIfArgumentNull(a_pTable);
    OpcUa_ReturnErrorIfArgumentNull(a_pType);

    OpcUa_MemSet(a_pType, 0, sizeof(OpcUa_ServiceType));

    /* check for empty table */
    if (a_pTable->Entries == OpcUa_Null)
    {
        return OpcUa_BadServiceUnsupported;
    }

    cKey.RequestTypeId = a_uTypeId;

    /* search for type id */
    pType = (OpcUa_ServiceType *)OpcUa_BSearch(  &cKey, 
                            a_pTable->Entries,
                            a_pTable->Count, 
                            sizeof(OpcUa_ServiceType), 
                            OpcUa_ServiceType_Compare, 
                            OpcUa_Null);

    if (pType == OpcUa_Null)
    {
        return OpcUa_BadServiceUnsupported;
    }

    /* copy the definition */
    OpcUa_MemCpy(a_pType, sizeof(OpcUa_ServiceType), pType, sizeof(OpcUa_ServiceType));

    return OpcUa_Good;
}

/*============================================================================
 * OpcUa_ServerApi_CreateFault
 *===========================================================================*/
OpcUa_StatusCode OpcUa_ServerApi_CreateFault(
    OpcUa_RequestHeader*   a_pRequestHeader,
    OpcUa_StatusCode       a_uServiceResult,
    OpcUa_DiagnosticInfo*  a_pServiceDiagnostics,
    OpcUa_Int32*           a_pNoOfStringTable,
    OpcUa_String**         a_ppStringTable,
    OpcUa_Void**           a_ppFault,
    OpcUa_EncodeableType** a_ppFaultType)
{
    /* create a fault object */
    OpcUa_ServiceFault* pFault = OpcUa_Null;
    
    OpcUa_InitializeStatus(OpcUa_Module_ProxyStub, "OpcUa_ServerApi_CreateFault");

    /* check arguments */
    OpcUa_ReturnErrorIfArgumentNull(a_pRequestHeader);
    OpcUa_ReturnErrorIfArgumentNull(a_ppFault);
    OpcUa_ReturnErrorIfArgumentNull(a_ppFaultType);

    *a_ppFault     = OpcUa_Null;
    *a_ppFaultType = OpcUa_Null;

    uStatus = OpcUa_EncodeableObject_Create(&OpcUa_ServiceFault_EncodeableType, (OpcUa_Void**)&pFault);
    OpcUa_GotoErrorIfBad(uStatus);

    /* fill in fault header */
    pFault->ResponseHeader.Timestamp      = OPCUA_P_DATETIME_UTCNOW();
    pFault->ResponseHeader.RequestHandle  = a_pRequestHeader->RequestHandle;
    pFault->ResponseHeader.ServiceResult  = a_uServiceResult;

    /* copy diagnostics */
    if (a_pServiceDiagnostics != OpcUa_Null)
    {
        pFault->ResponseHeader.ServiceDiagnostics  = *a_pServiceDiagnostics;
        OpcUa_MemSet(a_pServiceDiagnostics, 0, sizeof(OpcUa_DiagnosticInfo));
    }

    /* copy strings */
    if (a_pNoOfStringTable != OpcUa_Null && *a_pNoOfStringTable > 0)
    {
        pFault->ResponseHeader.NoOfStringTable     = *a_pNoOfStringTable;
        pFault->ResponseHeader.StringTable         = *a_ppStringTable;

        *a_pNoOfStringTable = 0;
        *a_ppStringTable = OpcUa_Null;
    }

    *a_ppFault     = pFault;
    *a_ppFaultType = &OpcUa_ServiceFault_EncodeableType;

    OpcUa_ReturnStatusCode;
    OpcUa_BeginErrorHandling;

    OpcUa_EncodeableObject_Delete(&OpcUa_ServiceFault_EncodeableType, (OpcUa_Void**)&pFault);

    *a_ppFault     = OpcUa_Null;
    *a_ppFaultType = OpcUa_Null;

    OpcUa_FinishErrorHandling;
}

#endif /* OPCUA_HAVE_SERVERAPI */

