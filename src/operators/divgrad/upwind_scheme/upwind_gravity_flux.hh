/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */

// -----------------------------------------------------------------------------
// ATS
//
// License: see $ATS_DIR/COPYRIGHT
// Author: Ethan Coon (ecoon@lanl.gov)
//
// Scheme for taking coefficients for div-grad operators from cells to
// faces.
// -----------------------------------------------------------------------------

#ifndef AMANZI_UPWINDING_GRAVITYFLUX_SCHEME_
#define AMANZI_UPWINDING_GRAVITYFLUX_SCHEME_

#include "Epetra_Vector.h"
#include "tensor.hpp"

#include "upwinding.hh"

namespace Amanzi {
namespace Operators {

class UpwindGravityFlux : public Upwinding {

public:

  UpwindGravityFlux(std::string pkname,
                    std::string cell_coef,
                    std::string face_coef,
                    const Teuchos::RCP<std::vector<WhetStone::Tensor> > K);

  void Update(const Teuchos::Ptr<State>& S);

  void CalculateCoefficientsOnFaces(
        const CompositeVector& cell_coef,
        const Epetra_Vector& gravity,
        const Teuchos::Ptr<CompositeVector>& face_coef);

private:

  std::string pkname_;
  std::string cell_coef_;
  std::string face_coef_;

  Teuchos::RCP<std::vector<WhetStone::Tensor> > K_;
};

} // namespace
} // namespace

#endif
