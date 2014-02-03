/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */
/* -------------------------------------------------------------------------
ATS

License: see $ATS_DIR/COPYRIGHT
Author: Ethan Coon

Test PK for testing snow distribution

------------------------------------------------------------------------- */

#ifndef PKS_TEST_SNOW_DIST_HH_
#define PKS_TEST_SNOW_DIST_HH_

#include "pk_factory.hh"
#include "pk_physical_base.hh"

namespace Amanzi {

class TestSnowDist : public PKPhysicalBase {

public:

  TestSnowDist(const Teuchos::RCP<Teuchos::ParameterList>& plist,
                      Teuchos::ParameterList& FElist,
                      const Teuchos::RCP<TreeVector>& solution) :
      PKDefaultBase(plist, FElist, solution),
      PKPhysicalBase(plist, FElist, solution) {
    plist_->set("solution key", "snow_depth");
  }

  // ConstantTemperature is a PK
  // -- Setup data
  virtual void setup(const Teuchos::Ptr<State>& S);

  // -- Commit any secondary (dependent) variables.
  virtual void commit_state(double dt, const Teuchos::RCP<State>& S) {}

  // -- Update diagnostics for vis.
  virtual void calculate_diagnostics(const Teuchos::RCP<State>& S) {}

  // -- advance via one of a few methods
  virtual bool advance(double dt);

  double get_dt() {
    return 24*60*60.0; // 1 day
  }

private:
  // factory registration
  static RegisteredPKFactory<TestSnowDist> reg_;
};

} // namespace

#endif