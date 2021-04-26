//
// Copyright (c) 2021, Datalogics, Inc. All rights reserved.
//
// For complete copyright information, refer to:
// http://dev.datalogics.com/adobe-pdf-library/license-for-downloaded-pdf-samples/
//
// ConvertColors converts the elements in the PDF to the target ICC profile or to an internal profile
//   -- See the Acrobat -> Tools -> Print Production -> Color Convert menu for related functionality

#include "InitializeLibrary.h"
#include "APDFLDoc.h"

#include "AcroColorExpT.h"
#include "AcroColorCalls.h" 

#define DIR_LOC "./"

#define PDF_FNAME "test.pdf"
#define CM_INPROFILE "colormatch.icc"
#define SRGB_INPROFILE "srgb.icc"
#define OUT_PROFILE "cmyk.icc"
#define OUT_PDF "output.pdf"
#define OUT_EX_PDF "output-Ex.pdf"


/* This function takes in one profile to use as the RGB working space.  
   -- Could also set the CMYK and Gray as desired
*/
void ConvertColors(char *inputProfile,char *outputPDF )
{
	ASFile                  Profile;
	ASTFilePos              ProfileSize;
	ASUns8                 *ProfileBuffer;
	AC_Profile				outputProfile;

	ASPathName pathName = NULL; 
	pathName = ASFileSysCreatePathName(NULL, ASAtomFromString("Cstring"), PDF_FNAME, 0);
	PDDoc pdDoc = PDDocOpen(pathName, NULL, NULL, true);
	ASFileSysReleasePath(NULL, pathName); pathName = NULL;
	
	/* Set default source profile for rgb device space */
	ASPathName Path = ASFileSysCreatePathFromCString (NULL, inputProfile);

	ASFileSysOpenFile (NULL, Path, ASFILE_READ, &Profile);

	ProfileSize = ASFileGetEOF (Profile);
	ProfileBuffer = (ASUns8 *)ASmalloc (ProfileSize);
	ASFileRead (Profile, (char *)ProfileBuffer, ProfileSize);
	ASFileClose (Profile);

	PDPrefSetWorkingRGB(ProfileBuffer,ProfileSize);
	ASfree(ProfileBuffer);

	/* Now setup the output profile
	*  -- Option 1: create a profile from code  -- See ConvertColors()
	*  -- Option 2: use an external ICC profile -- See ConvertColorsEx() below
	*/

	/* Option 1: Create output profile from an internal API */
     if (ACProfileFromCode (&outputProfile, AC_Profile_DotGain15) != AC_Error_None)
           fprintf (stderr, "could not load color profile\n");

		PDColorConvertParamsRec params;
		PDColorConvertActionRec action;
		ASBool changed;

		memset(&params,0,sizeof(PDColorConvertParamsRec));
  
		params.mNumActions = 1;
		params.mActions = &action;
		memset(&action, 0, sizeof(PDColorConvertActionRec));
		params.mActions->mMatchAttributesAll = -1;
		params.mActions->mMatchAttributesAny = kColorConvObj_AnyObject;
		params.mActions->mMatchSpaceTypeAll = -1;
		params.mActions->mMatchIntent = AC_UseProfileIntent;
		params.mActions->mAction = kColorConvConvert;
		params.mActions->mConvertIntent = AC_Perceptual; 

		params.mActions->mEmbed = 1;
		params.mActions->mPreserveBlack = TRUE;
		params.mActions->mUseBlackPointCompensation = 0;
		PDPrefSetBlackPointCompensation(0);

		params.mActions->mMatchSpaceTypeAny = kColorConvAnySpace;
		params.mActions->mConvertProfile = outputProfile;
		PDDocColorConvertPage(pdDoc,&params,0,NULL,NULL,NULL,NULL,&changed);  //third param = page number

		ASFileSys asFileSys = ASGetDefaultFileSys();
		Path = ASFileSysCreatePathFromCString (NULL, outputPDF);
		PDDocSave(pdDoc,PDSaveCompressed | PDSaveFull | PDSaveLinearized | PDSaveCollectGarbage,Path,asFileSys,NULL,NULL);
		PDDocClose(pdDoc);

}

/* This function takes in one profile to use as the RGB working space.
-- Could also set the CMYK and Gray as desired
-- This uses PDDocColorConvertPageEx (recommended) and PDColorConvertParamsRecEx / PDColorConvertActionRecEx
  -- Comparing ActionRec to ActionRecEx adds ...
  -- mMatchMinFontSize, mMatchMaxFontSize
  -- mPromoteGrayToCMYK
  -- mPreserveCMYKPrimaries
*/
void ConvertColorsEx(char* inputProfile, char* outputPDF)
{
	ASFile        Profile;
	ASTFilePos    ProfileSize;
	ASUns8* ProfileBuffer;
	AC_Profile    outputProfile;

	ASPathName pathName = NULL;
	pathName = ASFileSysCreatePathName(NULL, ASAtomFromString("Cstring"), PDF_FNAME, 0);
	PDDoc pdDoc = PDDocOpen(pathName, NULL, NULL, true);
	ASFileSysReleasePath(NULL, pathName); pathName = NULL;

	/* Now setup the output profile */

	/* Option 2: Create output profile from an external ICC */
	ASPathName Path = ASFileSysCreatePathFromCString(NULL, OUT_PROFILE);
	ASFileSysOpenFile(NULL, Path, ASFILE_READ, &Profile);

	ProfileSize = ASFileGetEOF(Profile);
	ProfileBuffer = (ASUns8*)ASmalloc(ProfileSize);
	ASFileRead(Profile, (char*)ProfileBuffer, ProfileSize);
	ASFileClose(Profile);
	ACMakeBufferProfile(&outputProfile, ProfileBuffer, ProfileSize);
	ASfree(ProfileBuffer);

	PDColorConvertParamsRecEx params;
	PDColorConvertActionRecEx action;

	ASBool changed;

	memset(&params, 0, sizeof(PDColorConvertParamsRecEx)); //
	params.mSize = sizeof(PDColorConvertParamsRecEx);
	
	params.mNumActions = 1;
	params.mActions = &action;
	memset(&action, 0, sizeof(PDColorConvertActionRecEx));
	params.mActions->mSize = sizeof(PDColorConvertActionRecEx);

	params.mActions->mMatchAttributesAll = -1;
	params.mActions->mMatchAttributesAny = kColorConvObj_AnyObject;
	params.mActions->mMatchSpaceTypeAll = -1;
	params.mActions->mMatchIntent = AC_UseProfileIntent;
	params.mActions->mAction = kColorConvConvert;
	params.mActions->mConvertIntent = AC_Perceptual;
	params.mActions->mMatchSpaceTypeAny = kColorConvAnySpace;

	params.mActions->mEmbed = 1;
	params.mActions->mPreserveBlack = TRUE;
	params.mActions->mUseBlackPointCompensation = 0;
	PDPrefSetBlackPointCompensation(0);

	ACProfileFromCode(&params.defaultRGB, AC_Profile_sRGB);
	ACProfileFromCode(&params.defaultCMYK, AC_Profile_Acrobat9_CMYK);
	ACProfileFromCode(&params.defaultGray, AC_Profile_DotGain15);
	params.intentCMYK = AC_Perceptual;
	params.intentRGB = AC_Perceptual;
	params.intentGray = AC_Perceptual;

	params.mActions->mConvertProfile = outputProfile;

	// recommended to use the PDDocConvertPageEx() below rather than the older PDDocConvertPage()
	PDDocColorConvertPageEx(pdDoc, &params, 0, NULL, NULL, NULL, NULL, &changed);  //third param = page number

	if (params.defaultCMYK != NULL) {
		ACUnReferenceProfile(params.defaultCMYK);
	}
	if (params.defaultGray != NULL) {
		ACUnReferenceProfile(params.defaultGray);
	}
	if (params.defaultRGB != NULL) {
		ACUnReferenceProfile(params.defaultRGB);
	}

	ASFileSys asFileSys = ASGetDefaultFileSys();
	Path = ASFileSysCreatePathFromCString(NULL, outputPDF);
	PDDocSave(pdDoc, PDSaveCompressed | PDSaveFull | PDSaveLinearized | PDSaveCollectGarbage, Path, asFileSys, NULL, NULL);
	PDDocClose(pdDoc);

}


int main(int argc, char** argv)
{
    char buf[256];

    APDFLib libInit;
    ASErrorCode errCode = 0;
    if (libInit.isValid() == false)
    {
        errCode = libInit.getInitError();
        std::cout << "Initialization failed with code " << errCode << std::endl;
        return libInit.getInitError();
    }

    DURING
        std::cout << "Converting colors in " << PDF_FNAME << std::endl;
        ConvertColorsEx(SRGB_INPROFILE, OUT_EX_PDF);  // use one or the other
    HANDLER
        ASGetErrorString(ERRORCODE, buf, sizeof(buf));
        fprintf(stderr, "Error in conversion using sRGB - code: %ld, Error Message: %s\n", ERRORCODE, buf);
    END_HANDLER

}
