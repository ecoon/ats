/*
  AdditiveEvaluator is the generic evaluator for adding N other fields.

  Authors: Ethan Coon (ecoon@lanl.gov)
*/

#ifndef AMANZI_RELATIONS_ADDITIVE_EVALUATOR_
#define AMANZI_RELATIONS_ADDITIVE_EVALUATOR_

#include "factory.hh"
#include "secondary_variable_field_evaluator.hh"

namespace Amanzi {
namespace Relations {

class AdditiveEvaluator : public SecondaryVariableFieldEvaluator {

 public:
  // constructor format for all derived classes
  explicit
  AdditiveEvaluator(Teuchos::ParameterList& plist);

  AdditiveEvaluator(const AdditiveEvaluator& other);
  Teuchos::RCP<FieldEvaluator> Clone() const;

  // Required methods from SecondaryVariableFieldEvaluator
  void EvaluateField_(const Teuchos::Ptr<State>& S,
                      const Teuchos::Ptr<CompositeVector>& result);
  void EvaluateFieldPartialDerivative_(const Teuchos::Ptr<State>& S,
          Key wrt_key, const Teuchos::Ptr<CompositeVector>& result);

 protected:
  std::map<Key, double> coefs_;

 private:
  static Utils::RegisteredFactory<FieldEvaluator,AdditiveEvaluator> factory_;
};

} // namespace
} // namespace

#endif

