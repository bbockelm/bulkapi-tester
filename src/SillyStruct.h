
#include "Rtypes.h"
#include "TObject.h"

/**
 * The SillyStruct has no purpose except to provide
 * inputs to the test cases.
 */

class SillyStruct : public TObject {

public:
   float  a;
   int    b;
   double c;

   ClassDef(SillyStruct, 1)
};

