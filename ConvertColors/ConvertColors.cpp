//
// Copyright (c) 2019, Datalogics, Inc. All rights reserved.
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
	*  -- Option 2: use an external ICC profile -- See ConvertColorsEx()
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
-- This uses PDDocColorConvertPageEx() and	PDColorConvertParamsRecEx / PDColorConvertActionRecEx
  -- Comparing ActionRec to ActionRecEx adds ...
  -- mMatchMinFontSize, mMatchMaxFontSize
  -- mPromoteGrayToCMYK
  -- mPreserveCMYKPrimaries
*/
void ConvertColorsEx(char *inputProfile, char *outputPDF)
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
	ASPathName Path = ASFileSysCreatePathFromCString(NULL, inputProfile);

	ASFileSysOpenFile(NULL, Path, ASFILE_READ, &Profile);

	ProfileSize = ASFileGetEOF(Profile);
	ProfileBuffer = (ASUns8 *)ASmalloc(ProfileSize);
	ASFileRead(Profile, (char *)ProfileBuffer, ProfileSize);
	ASFileClose(Profile);

	PDPrefSetWorkingRGB(ProfileBuffer, ProfileSize);
	ASfree(ProfileBuffer);

	/* Now setup the output profile 
	*  -- Option 1: create a profile from code  -- See ConvertColors()
	*  -- Option 2: use an external ICC profile -- See ConvertColorsEx()
	*/
	/* Option 2: Create output profile from an external ICC */
	Path = ASFileSysCreatePathFromCString (NULL, OUT_PROFILE);
	ASFileSysOpenFile (NULL, Path, ASFILE_READ, &Profile);

	ProfileSize = ASFileGetEOF (Profile);
	ProfileBuffer = (ASUns8 *)ASmalloc (ProfileSize);
	ASFileRead (Profile, (char *)ProfileBuffer, ProfileSize);
	ASFileClose (Profile);
	ACMakeBufferProfile(&outputProfile,ProfileBuffer, ProfileSize);
	ASfree(ProfileBuffer);

		PDColorConvertParamsRecEx params;
		PDColorConvertActionRecEx action;

		ASBool changed;

		memset(&params, 0, sizeof(PDColorConvertParamsRec));
		params.mNumActions = 1;
		params.mActions = &action;
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
		PDDocColorConvertPageEx(pdDoc, &params, 0, NULL, NULL, NULL, NULL, &changed);  //third param = page number

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

	// ConvertColors(SRGB_INPROFILE, OUT_PDF);     // use one or the other
	ConvertColorsEx(SRGB_INPROFILE, OUT_EX_PDF);  // use one or the other
	HANDLER
		ASGetErrorString(ERRORCODE, buf, sizeof(buf));
	fprintf(stderr, "Error in conversion using sRGB - code: %ld, Error Message: %s\n", ERRORCODE, buf);
	END_HANDLER

}








