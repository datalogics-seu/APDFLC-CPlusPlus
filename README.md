# APDFLC-CPlusPlus

APDFL C++ samples -- Derived from demos, blogs or other sources

** ConvertColors -- converts the elements in the PDF to the target ICC profile or to an internal profile. 
  -- Demonstrates PDDocColorConvertPage() and PDDocColorConvertPageEx()
  -- See the Acrobat -> Tools -> Print Production -> Color Convert menu for related functionality
  -- v15.0.4

** CosStreamSetDataSnippet -- modifies a cos object with CosStreamSetData(). CosID stays the same. 
  -- old API that Adobe depracated but DL ressurected
  -- v10.1
  
** ExtractText-FromRegion -- 
  -- Modified version of the ExtractText sample that demonstrates extracting text from a specific location by 
  -- comparing the Word quads to a user specified target area. See the //DLADD comments
  -- v15.0.4
  
** ReplaceFont -- replaces a reference to Helvetica to a reference to Courier. Includes a search through a Form XOject.
  -- v15.0.4
  -- Superceded by PDDocReplaceSimpleFonts() API added in v18.0.3

** Missing: Extract Text XY order (bc)
** Replace font in Resources (bc)
