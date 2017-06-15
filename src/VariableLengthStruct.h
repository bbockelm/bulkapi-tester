
#include "Rtypes.h"
#include "TObject.h"

/**
 * The VariableLengthStruct has no purpose except to provide
 * inputs to the test cases.
 */

class VariableLengthStruct : public TObject {

public:
   Int_t    myLen{0};
   Float_t *a{nullptr};     //[myLen]
   int      b{0};
   double  *c{nullptr};     //[myLen]

   ClassDef(VariableLengthStruct, 1)
};

