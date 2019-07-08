//
// Copyright (c) 2019, Datalogics, Inc. All rights reserved.
//
// For complete copyright information, refer to:
// http://dev.datalogics.com/adobe-pdf-library/license-for-downloaded-pdf-samples/
//
// This sample demonstrates replacing a reference to a font 
//
// Command-line:  <output-file>  <output-file>     (Both optional)
//

#include <iostream>

#include "PSFCalls.h"
#include "PERCalls.h"
#include "PEWCalls.h"
#include "PagePDECntCalls.h"
#include "ASExtraCalls.h"

#include "APDFLDoc.h"
#include "InitializeLibrary.h"
#include "DLExtrasCalls.h"

#define DIR_LOC "../../../../Resources/Sample_Input/"
#define DEF_INPUT "HelvReferenced.pdf"
#define DEF_OUTPUT "HelvReferenced-out.pdf"

// This define turns on the facility to replace the font reference in the PDEForm object
#define ReplaceInPDE 1

/* Keep the replacement fonts names global, to simplify */
PDEFont Courier, CourierBold;

/* Validate that Helvetica and Helvetica-Bold are not among the
** documents font resources
*/
ASBool  validateHelvAbsent(CosObj obj, CosObj value, void *clientData)
{
	/* Get pointer to result. Set to false if Helvetica is seen
	*/
	ASBool *helvNotSeen = (ASBool *)clientData;

	/* Get a PDEFont from the cos font passed in
	*/
	PDEFont font = PDEFontCreateFromCosObj(&obj);

	/* Acquire the font attributes */
	PDEFontAttrs attrs;
	PDEFontGetAttrs(font, &attrs, sizeof(PDEFontAttrs));

	/* Check for Helvetica */
	if (attrs.name == ASAtomFromString("Helvetica"))
	{
		*helvNotSeen = false;
		printf("Helvetica IS present in the document!\n");
	}
	if (attrs.name == ASAtomFromString("Helvetica-Bold"))
	{
		*helvNotSeen = false;
		printf("Helvetica-Bold IS present in the document!\n");
	}

	/* Release the PDEFont */
	PDERelease((PDEObject)font);

	/* return "true", so the enumeration will proceed */
	return (true);
}

void ReplaceFontReferences(PDEContent content)
{
	/* For each element in this content */
	for (ASInt32 elem = 0; elem < PDEContentGetNumElems(content); elem++)
	{
		/* Get the element (no need to release, does not increment) */
		PDEElement element = PDEContentGetElem(content, elem);

		/* Get it's type */
		PDEType objType = (PDEType)PDEObjectGetType((PDEObject)element);

		/* Switch by type */
		switch (objType)
		{
		case kPDEContent:
			/* recurse through content elements */
			ReplaceFontReferences((PDEContent)element);
			break;

		case kPDEForm:
		{
			/* Recurse through forms contents
			** And note that PDEFormGetContent DOES
			** increment the content, and hence must release
			*/
			PDEContent formContent = PDEFormGetContent((PDEForm)element);
			ReplaceFontReferences(formContent);
			PDEFormSetContent((PDEForm)element, formContent);

#if ReplaceInPDE
			/* Test and replace the font stored in the
			** PDEForm.
			*/
			PDEFont font = PDEFormGetFont((PDEForm)element);
			PDEFontAttrs attrs;
			memset(&attrs, 0, sizeof(PDEFontAttrs));

			/* If it is not a null pointer, get it's attributes */
			if (font != NULL)
				PDEFontGetAttrs(font, &attrs, sizeof(attrs));

			/* If the font is Helveritca, change it to Courier */
			if (attrs.name == ASAtomFromString("Helvetica"))
				PDEFormSetFont((PDEForm)element, Courier);

			/* If the font is Helvetica-Bold, Change it to Courier Bold */
			if (attrs.name == ASAtomFromString("Helvetica-Bold"))
				PDEFormSetFont((PDEForm)element, CourierBold);
#endif

			PDERelease((PDEObject)formContent);
		}
		break;

		case kPDEContainer:
		{
			/* Recurse through container content.
			*/
			PDEContent containerContent = PDEContainerGetContent((PDEContainer)element);
			ReplaceFontReferences(containerContent);
		}
		break;

		case kPDEText:
		{
			/* Replace the font reference in each run
			*/
			for (ASInt32 run = 0; run < PDETextGetNumRuns((PDEText)element); run++)
			{
				PDETextItem item = PDETextGetItem((PDEText)element, run);

				/* Acquire the runs font */
				PDEFont font = PDETextItemGetFont(item);

				/* Aquire the fonts Attributes */
				PDEFontAttrs attrs;
				PDEFontGetAttrs(font, &attrs, sizeof(PDEFontAttrs));

				/* Note if nfont changed */
				ASBool fontChanged = false;

				/* If the font is Helvetica, change the name to CourierStd */
				if (attrs.name == ASAtomFromString("Helvetica"))
				{
					fontChanged = true;
					font = Courier;
				}

				/* If the font is Helvetica-Bold, change the name to CourierSrd-Bold */
				if (attrs.name == ASAtomFromString("Helvetica-Bold"))
				{
					fontChanged = true;
					font = CourierBold;
				}

				/* If the font name changed, replace the font in the run */
				if (fontChanged)
					PDETextItemSetFont(item, font);
			}
		}
		break;

		default:
			break;
		}
	}
}

int main(int argc, char** argv)
{

    ASErrorCode errCode = 0;
    APDFLib libInit;

    if (libInit.isValid() == false)
    {
        errCode = libInit.getInitError();
        std::cout << "Initialization failed with code " << errCode << std::endl;
        return errCode;
    }
     
	std::string csInputFileName(argc > 1 ? argv[1] : DIR_LOC DEF_INPUT);
	std::string csOutputFileName(argc > 2 ? argv[2] : DEF_OUTPUT);
	std::cout << "Creating output document " << csOutputFileName.c_str()
		<< " with Helvetica removed.\n"; 
DURING
    APDFLDoc apdflDoc(csInputFileName.c_str(), true);
    PDDoc doc = apdflDoc.getPDDoc();

	/* Create the PDEFonts we are going to replace
	** Helvetica with
	*/
	PDEFontAttrs attrs;
	memset((char *)&attrs, 0, sizeof(PDEFontAttrs));
	attrs.name = ASAtomFromString("CourierStd");
	attrs.type = ASAtomFromString("Type1");
	attrs.charSet = ASAtomFromString("Roman");
	attrs.encoding = ASAtomFromString("WinAnsiEncoding");
	PDSysFont sysFont = PDFindSysFont(&attrs, sizeof(PDEFontAttrs), kPDSysFontMatchNameAndCharSet);
	Courier = PDEFontCreateFromSysFont(sysFont, kPDEFontCreateEmbedded);

	attrs.name = ASAtomFromString("CourierStd-Bold");
	sysFont = PDFindSysFont(&attrs, sizeof(PDEFontAttrs), kPDSysFontMatchNameAndCharSet);
	CourierBold = PDEFontCreateFromSysFont(sysFont, kPDEFontCreateEmbedded);


	/* Walk through each page of the document */
	for (ASInt32 pageNumb = 0; pageNumb < PDDocGetNumPages(doc); pageNumb++)
	{
		/* Acquire the page */
		PDPage page = PDDocAcquirePage(doc, pageNumb);

		/* Acquire the page content */
		PDEContent content = PDPageAcquirePDEContent(page, 0);

		/* Replace all references to the fonts Helvetica or Helvetica-Bold
		** with references to CourierStd and CourierStd-Bold
		*/
		ReplaceFontReferences(content);

		/* Set the modified content into the page */
		PDPageSetPDEContent(page, 0);

		/* Release the page content */
		PDPageReleasePDEContent(page, 0);

		/* Release the page */
		PDPageRelease(page);
	}

	/* Subset and embed the courier fonts */
	PDEFontEmbedNow(Courier, PDDocGetCosDoc(doc));
	PDEFontEmbedNow(CourierBold, PDDocGetCosDoc(doc));

	/* Then release them both */
	PDERelease((PDEObject)Courier);
	PDERelease((PDEObject)CourierBold);

	/* Save the modified document for later examination */
	ASPathName outPath = ASFileSysCreatePathFromDIPath(NULL, csOutputFileName.c_str(), NULL);
	PDDocSave(doc, PDSaveFull | PDSaveCollectGarbage, outPath, NULL, NULL, NULL);

HANDLER
    errCode = ERRORCODE;           
    libInit.displayError(errCode);
END_HANDLER

    return errCode;                 //APDFLib's destructor terminates the library.

}
