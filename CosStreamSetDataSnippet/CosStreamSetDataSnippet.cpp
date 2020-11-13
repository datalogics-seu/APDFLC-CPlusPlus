
//#include "PDFLPrivCalls.h"  // for CosStreamSetData below
#include "DLExtrasCalls.h"  // for CosStreamSetData below
#include "PagePDECntCalls.h" // PDPageAcquire/Set/ReleasePDEContent


	pdDoc = MyPDDocOpen("out.pdf");
/* Added code to modify document */
    CosDoc tCosDoc = PDDocGetCosDoc(pdDoc);
    CosObj objToMod = CosDocGetObjByID(tCosDoc, 8);
    ASInt32 objT = CosObjGetType(objToMod);

    if ( (CosObjGetType(objToMod) != CosStream) )
    {
        printf("Object not a stream.\n");
    }
    else
    {
        ASPathName outPath = ASFileSysCreatePathName(NULL, ASAtomFromString("Cstring"), "out-2.pdf", 0);
        char *newData = "This is some new, and invalid, data.";
        ASStm newASStm = ASMemStmRdOpen(newData, strlen(newData));
// UNDOCUMENTED: include PDFLPrivCalls.h (APDFL v7.0.1)
        DURING
            CosStreamSetData(objToMod,
                newASStm,
                -1, // seekable
                true,   // ASStm data decoded?
                CosNewNull(),   // new attrs dict
                CosNewNull(),   // encode params
                -1  // decode length
                );
            ASStmClose(newASStm);
        HANDLER
            printf("Bimminy!\n");
        END_HANDLER

		PDDocSave(pdDoc, PDSaveFull |  PDSaveCollectGarbage, outPath, 0, 0, 0);
        PDDocClose(pdDoc);


