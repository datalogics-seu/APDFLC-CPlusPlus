//
// Copyright (c) 2017, Datalogics, Inc. All rights reserved.
//
// For complete copyright information, refer to:
// http://dev.datalogics.com/adobe-pdf-library/license-for-downloaded-pdf-samples/
//
// This sample demonstrates using the PDWordFinder to find examples of a specific word in a PDF
// document.  When the sample finds the text it highlights each example and saves the file as an
// output document.
//
// Command-line:   <input-file> <output-file> <search-word>    (All optional)
//
// For more detail see the description of the TextSearch sample program on our Developer’s site,
// http://dev.datalogics.com/adobe-pdf-library/sample-program-descriptions/c1samples#textsearch

#include <iostream>
#include <algorithm>
#include <vector>
#include "CosCalls.h"
#include "InitializeLibrary.h"
#include "APDFLDoc.h"

// DL Add
#include "PSFCalls.h"
#include "PERCalls.h"
#include "PEWCalls.h"
#include "PagePDECntCalls.h"

#define DIR_LOC "../../../../Resources/Sample_Input/"
#define DEF_INPUT "TextSearch.pdf"
#define DEF_OUTPUT "TextSearch-out.pdf"
#define DEF_SEARCH_WORD "pirate"


static void ApplyQuadsToAnnot(PDAnnot, ASFixedQuad *, ASArraySize);
static bool DoesWordMatch(PDWord, const char *);
static void AnnotateWord(PDWord, PDPage, PDColorValue, PDDoc);

int main(int argc, char *argv[]) {
    APDFLib libInit;
    ASErrorCode errCode = 0;

    if (libInit.isValid() == false) {
        errCode = libInit.getInitError();
        std::cout << "Initialization failed with code " << errCode << std::endl;
        return libInit.getInitError();
    }

    std::string csInputFileName(argc > 1 ? argv[1] : DIR_LOC DEF_INPUT);
    std::string csOutputFileName(argc > 2 ? argv[2] : DEF_OUTPUT);
    std::string csSearchWord(argc > 3 ? argv[3] : DEF_SEARCH_WORD);
    std::cout << "Will search " << csInputFileName.c_str() << " for all occurrences of "
              << "the word \"" << csSearchWord.c_str() << "\"," << std::endl
              << "highlight them all, "
              << " and save to " << csOutputFileName.c_str() << std::endl;



    DURING

        APDFLDoc document(csInputFileName.c_str(), true);
        // PDDoc pdDoc = document.getPDDoc();

        // Step 1) Set up the word finder configuration record

        PDWordFinderConfigRec wfConfig;

        memset(&wfConfig, 0, sizeof(wfConfig));           // Always do this!
        wfConfig.recSize = sizeof(PDWordFinderConfigRec); //...and this!

        wfConfig.disableTaggedPDF = true; // Treat this as a non-tagged PDF document.
        wfConfig.noXYSort = true;         // Don't generate an XY-ordered word list.
        wfConfig.preserveSpaces = false;  // Don't preserve spaces during word breaking.
        wfConfig.noLigatureExp = false; // Enable expansion of ligatures using the default ligatures.
        wfConfig.noEncodingGuess = true; // Disable guessing encoding of fonts with unknown/custom encoding.
        wfConfig.unknownToStdEnc = false; // Don't assume all fonts are Standard Roman.
                                          //   Setting to true overrides noEncodingGuess.
        wfConfig.ignoreCharGaps = true;     // Disable converting large character gaps to spaces.
        wfConfig.ignoreLineGaps = false;    // Treat vertical movements as line breaks.
        wfConfig.noAnnots = true;           // Don't extract from annotations.
        wfConfig.noHyphenDetection = false; // Don't differentiate between hard and soft hyphens.
        wfConfig.trustNBSpace = false; // Don't differentiate between breaking and non-breaking spaces.
        wfConfig.noExtCharOffset = false; // If client doesn't have a need for detailed character
                                          //   offset information set to true for improvement in efficiency.
        wfConfig.noStyleInfo = false; // Set to true if client doesn't have a need for style
                                      //   information for improvement in efficiency.
        wfConfig.decomposeTbl = NULL; // Table may be used to expand unicode ligatures
                                      //   not in the default list.
        wfConfig.decomposeTblSize = 0;           // Not using decomposeTbl, so the size is 0.
        wfConfig.charTypeTbl = NULL;             // Custom table to enhance word breaking quality.
        wfConfig.charTypeTblSize = 0;            // Unused, so the size will be 0.
        wfConfig.preserveRedundantChars = false; // May be used to preserve overlapping redundant
                                                 //   characters in some PDF documents.
        wfConfig.disableCharReordering = false; // Used in cases where the PDF page has heavily
                                                //   overlapped character bounding boxes.

        // Step 2) Fill in color information for highlighting text. In this case our color will be set to orange.

        PDColorValueRec colorValRec;
        PDColorValue pdColorValue(&colorValRec);
        pdColorValue->space = PDDeviceRGB;             // Colors are set using the RGB color space.
        pdColorValue->value[0] = ASFloatToFixed(1.0);  // red level
        pdColorValue->value[1] = ASFloatToFixed(0.65); // green level
        pdColorValue->value[2] = ASFloatToFixed(0.0);  // blue level

        // Step 3) Search for the quarry, and highlight all occurrences

        PDWordFinder wordFinder =
            PDDocCreateWordFinderEx(document.getPDDoc(), WF_LATEST_VERSION, true, &wfConfig);

        // Iterate over all pages in thedocument
        ASInt32 countPages = PDDocGetNumPages(document.getPDDoc());
        for (ASInt32 pageNum = 0; pageNum < countPages; ++pageNum) {
            // Get the words for this page
            PDWord pdfWordArray;
            ASInt32 numberOfWords = 0;
            PDWordFinderAcquireWordList(wordFinder, pageNum, &pdfWordArray, NULL, NULL, &numberOfWords);

            // The PDPage object will be needed for adding the highlight annotation.
            PDPage pdPage = document.getPage(pageNum);

            // Examine each word
            for (ASInt32 index = 0; index < numberOfWords; ++index) {
                PDWord pdWord = PDWordFinderGetNthWord(wordFinder, index);
                if (DoesWordMatch(pdWord, csSearchWord.c_str())) {
                    AnnotateWord(pdWord, pdPage, pdColorValue, document.getPDDoc());  //                     AnnotateWord(pdWord, pdPage, pdColorValue);
                }
            }

            PDPageRelease(pdPage);
            // Release the word list before acquiring the next one.
            PDWordFinderReleaseWordList(wordFinder, pageNum);
        }

        PDWordFinderDestroy(wordFinder);

        document.saveDoc(csOutputFileName.c_str());

    HANDLER
        errCode = ERRORCODE;
        libInit.displayError(errCode);
    END_HANDLER

    return errCode;
}

// Add quads to an annotation's CosObj.
// NOTE: There is Adobe documentation that specifies quads be added in the order - BL, BR, TR, TL.
//    However, currently the order needs to be BL, BR, TL, TR to get correct output.
void ApplyQuadsToAnnot(PDAnnot annot, ASFixedQuad *quads, ASArraySize numQuads) {
    CosObj coAnnot = PDAnnotGetCosObj(annot);
    CosDoc coDoc = CosObjGetDoc(coAnnot);
    CosObj coQuads = CosNewArray(coDoc, false, numQuads * 8);
    static ASAtom atQP = ASAtomFromString("QuadPoints");

    for (ASUns32 i = 0, n = 0; i < numQuads; ++i) {
        CosArrayPut(coQuads, n++, CosNewFixed(coDoc, false, quads[i].bl.h));
        CosArrayPut(coQuads, n++, CosNewFixed(coDoc, false, quads[i].bl.v));
        CosArrayPut(coQuads, n++, CosNewFixed(coDoc, false, quads[i].br.h));
        CosArrayPut(coQuads, n++, CosNewFixed(coDoc, false, quads[i].br.v));
        CosArrayPut(coQuads, n++, CosNewFixed(coDoc, false, quads[i].tl.h)); // See note above
        CosArrayPut(coQuads, n++, CosNewFixed(coDoc, false, quads[i].tl.v));
        CosArrayPut(coQuads, n++, CosNewFixed(coDoc, false, quads[i].tr.h)); // See note above
        CosArrayPut(coQuads, n++, CosNewFixed(coDoc, false, quads[i].tr.v));
    }

    CosDictPut(coAnnot, atQP, coQuads);
}

// Determines if our word "matches" the quarry.  You control the logic of this decision.
//     In this sample, we will match on a case INsensitive containment of the search word
/* static */ bool DoesWordMatch(PDWord w, const char *q) {
    ASText asTextWord = ASTextNew();
    PDWordGetASText(w, 0, asTextWord);

    ASInt32 wordLen = 0;
    char *pdWordChar = ASTextGetPDTextCopy(asTextWord, &wordLen);
    std::string sWord(pdWordChar);
    std::transform(sWord.begin(), sWord.end(), sWord.begin(), ::tolower);
    bool rc = std::string::npos != sWord.find(q);

    ASTextDestroy(asTextWord);
    ASfree(pdWordChar);

    return rc;
}

    // /* static */ void AnnotateWord(PDWord w, PDPage p, PDColorValue c) {
    /* static */ void AnnotateWord(PDWord w, PDPage p, PDColorValue c, PDDoc pdDoc) {

    CosDoc inputDocCosDoc = PDDocGetCosDoc(pdDoc); //DL

    static ASAtom atH = ASAtomFromString("Highlight");

    // A value of -2 adds the annotation to the end of the page's annotation array
    ASInt32 addAfterCode = -2;

    // Coordinates of the annotation
    ASFixedQuad tempQuad;
    PDWordGetNthQuad(w, 0, &tempQuad);

    ASFixedRect annotationRect;
    annotationRect.left = tempQuad.bl.h-fixedHalf;
	annotationRect.top = tempQuad.tr.v + fixedHalf;
	annotationRect.right = tempQuad.tr.h + fixedHalf;
	annotationRect.bottom = tempQuad.bl.v - fixedHalf;

    // Create the annotation
    PDAnnot highlight = PDPageCreateAnnot(p, atH, &annotationRect);

    // Set its coordinates and color
    ApplyQuadsToAnnot(highlight, &tempQuad, 1);
    PDAnnotSetColor(highlight, c);


    // Add an AP dictionary to the annot

    // Create the Form
    PDEForm pdForm = 0;
    PDEContentAttrs attrs;
    std::memset(&attrs, 0, sizeof(attrs));

    attrs.flags = kPDEContentToForm;
    attrs.formType = 1;
    attrs.bbox = annotationRect;

    ASFixedMatrix matrix = {
        fixedOne, fixedZero, fixedZero, fixedOne, 
		-annotationRect.left, -annotationRect.bottom // NOTE: Does not seem to be having an effect...
    };
    attrs.matrix = matrix;

    PDEFilterArray filter;
    filter.numFilters = 0;

    CosDoc cosDoc = PDDocGetCosDoc(pdDoc);
    CosObj cosContent, cosResource;

    PDEContent pdeCont = PDEContentCreate();
	PDEPath pdePath = PDEPathCreate();

	// Can make the PDEPath  filled or stroked. 
	// PDEPathSetPaintOp(pdePath, kPDEStroke);
    PDEPathSetPaintOp(pdePath, kPDEFill);

	//Struct that will hold display attributes.
	PDEGraphicState gState;
    
    PDEExtGState extGState = PDEExtGStateCreateNew(inputDocCosDoc);
    PDEExtGStateSetOpacityFill(extGState, fixedThreeQuarters);
    PDEExtGStateSetBlendMode(extGState, ASAtomFromString("Darken")); //
    

	//Set graphics state to default values.
	PDEDefaultGState(&gState, sizeof(PDEGraphicState));
	//Release the stroke color space before modifiying it.
	PDERelease(reinterpret_cast<PDEObject>(gState.strokeColorSpec.space));

	if (c->space == PDDeviceRGB)
	{
		gState.strokeColorSpec.space = PDEColorSpaceCreateFromName(ASAtomFromString("DeviceRGB"));
        gState.fillColorSpec.space = PDEColorSpaceCreateFromName(ASAtomFromString("DeviceRGB"));
		gState.strokeColorSpec.value.color[0] = c->value[0];
		gState.strokeColorSpec.value.color[1] = c->value[1];
		gState.strokeColorSpec.value.color[2] = c->value[2];
        gState.fillColorSpec.value.color[0] = c->value[0];
        gState.fillColorSpec.value.color[1] = c->value[1];
        gState.fillColorSpec.value.color[2] = c->value[2];
        gState.extGState = extGState;
	}
	else
	{
		gState.strokeColorSpec.space = PDEColorSpaceCreateFromName(ASAtomFromString("DeviceGray"));
        gState.fillColorSpec.space = PDEColorSpaceCreateFromName(ASAtomFromString("DeviceGray"));
		gState.strokeColorSpec.value.color[0] = c->value[0];
        gState.fillColorSpec.value.color[0] = c->value[1];  
        gState.extGState = extGState;
	}

	gState.lineWidth = fixedHalf;               //Line width is set to a thickness of 3.
	//gState.lineCap = 1;                               //Using rounded line caps.
	//gState.lineJoin = 1;                              //Using rounded line joins.

	//Set the PDEPath's graphic state.
	PDEElementSetGState(reinterpret_cast<PDEElement>(pdePath), &gState, sizeof(PDEGraphicState));

	//Release the PDEColorSpace objects.
	//PDERelease(reinterpret_cast<PDEObject>(gState.strokeColorSpec.space));
	PDERelease(reinterpret_cast<PDEObject>(gState.fillColorSpec.space));
	ASFixed curvOffset = FloatToASFixed(2.3792); // NOTE: this value determined empirically does not seem to be sensitive to the Rect box for heights between 8.45 and 8.975; I can't say if it changes for greater heights.

	/* // this code bulges in at the sides rather than bulges out.
	PDEPathAddSegment(pdePath, kPDEMoveTo, annotationRect.left + fixedHalf, annotationRect.top - fixedHalf,
		fixedZero, fixedZero, fixedZero, fixedZero);
	PDEPathAddSegment(pdePath, kPDECurveTo, annotationRect.left + curvOffset, annotationRect.top - curvOffset,
		annotationRect.left + curvOffset, annotationRect.bottom + curvOffset, 
		annotationRect.left + fixedHalf, annotationRect.bottom + fixedHalf);
	PDEPathAddSegment(pdePath, kPDELineTo, annotationRect.right - fixedHalf, annotationRect.bottom + fixedHalf,
		fixedZero, fixedZero, fixedZero, fixedZero);
	PDEPathAddSegment(pdePath, kPDECurveTo, annotationRect.right - curvOffset, annotationRect.bottom + curvOffset,
		annotationRect.right - curvOffset, annotationRect.top - curvOffset,
		annotationRect.right - fixedHalf, annotationRect.top - fixedHalf);
	PDEPathAddSegment(pdePath, kPDEClosePath, fixedZero, fixedZero,	fixedZero, fixedZero, fixedZero, fixedZero);
	*/
	// this code bulges out at the sides.
	PDEPathAddSegment(pdePath, kPDEMoveTo, annotationRect.left + curvOffset, annotationRect.top - fixedHalf,
		fixedZero, fixedZero, fixedZero, fixedZero);
	PDEPathAddSegment(pdePath, kPDECurveTo, annotationRect.left + fixedHalf, annotationRect.top - curvOffset,
		annotationRect.left + fixedHalf, annotationRect.bottom + curvOffset,
		annotationRect.left + curvOffset, annotationRect.bottom + fixedHalf);
	PDEPathAddSegment(pdePath, kPDELineTo, annotationRect.right - curvOffset, annotationRect.bottom + fixedHalf,
		fixedZero, fixedZero, fixedZero, fixedZero);
	PDEPathAddSegment(pdePath, kPDECurveTo, annotationRect.right - fixedHalf, annotationRect.bottom + curvOffset,
		annotationRect.right - fixedHalf, annotationRect.top - curvOffset,
		annotationRect.right - curvOffset, annotationRect.top - fixedHalf);
	PDEPathAddSegment(pdePath, kPDEClosePath, fixedZero, fixedZero, fixedZero, fixedZero, fixedZero, fixedZero);


	PDEContentAddElem(pdeCont, kPDEAfterLast, reinterpret_cast<PDEElement>(pdePath));

    PDEContentToCosObj(pdeCont, kPDEContentToForm, &attrs, sizeof(attrs), cosDoc, &filter, &cosContent, &cosResource);

    PDEForm nForm = PDEFormCreateFromCosObj(&cosContent, &cosResource, &matrix);
    
	CosObj cosForm;
    PDEFormGetCosObj(nForm, &cosForm);
	

    // Get the cos obj of the annotation
    CosObj annotObj = PDAnnotGetCosObj(highlight);

    CosObj apDict;
    apDict = CosNewDict(cosDoc, false, 1);
    CosDictPutKeyString(apDict, "N", cosForm); //

    // Finally, add the AP dictionary containing the Form
    CosDictPutKeyString(annotObj, "AP", apDict); //  

  
    PDPageAddAnnot(p, addAfterCode, highlight);
}
