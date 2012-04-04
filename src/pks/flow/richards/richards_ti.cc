/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */

/*
A base two-phase, thermal Richard's equation with water vapor.

Authors: Ethan Coon (ATS version) (ecoon@lanl.gov)
*/

#include "richards.hh"

namespace Amanzi {
namespace Flow {

/* ******************************************************************
 * Calculate f(u, du/dt) = d(s u)/dt + A*u - g.
 ****************************************************************** */
void Richards::fun(double t_old, double t_new, Teuchos::RCP<TreeVector> u_old,
                       Teuchos::RCP<TreeVector> u_new, Teuchos::RCP<TreeVector> g) {
  S_inter_->set_time(t_old);
  S_next_->set_time(t_new);

  // pointer-copy temperature into states and update any auxilary data
  solution_to_state(u_old, S_inter_);
  solution_to_state(u_new, S_next_);
  UpdateSecondaryVariables_(S_inter_);
  UpdateSecondaryVariables_(S_next_);

  // update boundary conditions
  bc_pressure_->Compute(t_new);
  //bc_head_->Compute(t_new);
  bc_flux_->Compute(t_new);
  UpdateBoundaryConditions_();

  // zero out residual
  Teuchos::RCP<CompositeVector> res = g->data();
  res->PutScalar(0.0);

  // diffusion term, treated implicitly
  ApplyDiffusion_(S_next_, res);

  // accumulation term
  AddAccumulation_(res);
};

/* ******************************************************************
* Apply preconditioner to u and return the result in Pu.
****************************************************************** */
void Richards::precon(Teuchos::RCP<const TreeVector> u, Teuchos::RCP<TreeVector> Pu) {
  std::cout << "Precon application:" << std::endl;
  std::cout << "  u: " << (*u->data())("cell",0,0) << " " << (*u->data())("face",0,0) << std::endl;
  preconditioner_->ApplyInverse(*u->data(), Pu->data());
  std::cout << "  Pu: " << (*Pu->data())("cell",0,0) << " " << (*Pu->data())("face",0,0) << std::endl;
};


/* ******************************************************************
 * computes a norm on u-du and returns the result
 ****************************************************************** */
double Richards::enorm(Teuchos::RCP<const TreeVector> u,
                       Teuchos::RCP<const TreeVector> du) {
  double enorm_val = 0.0;
  Teuchos::RCP<const Epetra_MultiVector> pres_vec = u->data()->ViewComponent("cell", false);
  Teuchos::RCP<const Epetra_MultiVector> dpres_vec = du->data()->ViewComponent("cell", false);

  for (int lcv=0; lcv!=pres_vec->MyLength(); ++lcv) {
    double tmp = abs((*(*dpres_vec)(0))[lcv])/(atol_ + rtol_*abs((*(*pres_vec)(0))[lcv]));
    enorm_val = std::max<double>(enorm_val, tmp);
  }

  Teuchos::RCP<const Epetra_MultiVector> fpres_vec = u->data()->ViewComponent("face", false);
  Teuchos::RCP<const Epetra_MultiVector> fdpres_vec = du->data()->ViewComponent("face", false);

  for (int lcv=0; lcv!=fpres_vec->MyLength(); ++lcv) {
    double tmp = abs((*(*fdpres_vec)(0))[lcv])/(atol_ + rtol_*abs((*(*fpres_vec)(0))[lcv]));
    enorm_val = std::max<double>(enorm_val, tmp);
  }

#ifdef HAVE_MPI
  double buf = enorm_val;
  MPI_Allreduce(&buf, &enorm_val, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
#endif

  return enorm_val;
};

/* ******************************************************************
* Compute new preconditioner B(p, dT_prec).
****************************************************************** */
void Richards::update_precon(double t, Teuchos::RCP<const TreeVector> up, double h) {
  // update state with the solution up.
  S_next_->set_time(t);
  PK::solution_to_state(up, S_next_);
  UpdateSecondaryVariables_(S_next_);

  // update the rel perm according to the scheme of choice
  UpdatePermeabilityData_(S_next_);
  Teuchos::RCP<const CompositeVector> rel_perm_faces =
    S_next_->GetFieldData("rel_perm_faces");

  // update boundary conditions
  bc_pressure_->Compute(S_next_->time());
  //bc_head_->Compute(S_next_->time());
  bc_flux_->Compute(S_next_->time());
  UpdateBoundaryConditions_();

  preconditioner_->CreateMFDstiffnessMatrices(K_, rel_perm_faces);
  preconditioner_->CreateMFDrhsVectors();
  AddGravityFluxesToOperator_(S_next_, preconditioner_);

  // update with accumulation terms
  Teuchos::RCP<const CompositeVector> pres = S_next_->GetFieldData("pressure");
  Teuchos::RCP<const double> p_atm = S_next_->GetScalarData("atmospheric_pressure");
  Teuchos::RCP<const CompositeVector> temp = S_next_->GetFieldData("temperature");
  Teuchos::RCP<const CompositeVector> poro = S_next_->GetFieldData("porosity");

  Teuchos::RCP<const CompositeVector> mol_frac_gas =
    S_next_->GetFieldData("mol_frac_gas");
  Teuchos::RCP<const CompositeVector> n_gas = S_next_->GetFieldData("molar_density_gas");
  Teuchos::RCP<const CompositeVector> sat_gas = S_next_->GetFieldData("saturation_gas");

  Teuchos::RCP<const CompositeVector> n_liq = S_next_->GetFieldData("molar_density_liquid");
  Teuchos::RCP<const CompositeVector> sat_liq = S_next_->GetFieldData("saturation_liquid");

  Teuchos::RCP<const CompositeVector> cell_volume = S_next_->GetFieldData("cell_volume");

  std::vector<double>& Acc_cells = preconditioner_->Acc_cells();
  std::vector<double>& Fc_cells = preconditioner_->Fc_cells();
  int ncells = S_next_->mesh()->num_entities(AmanziMesh::CELL, AmanziMesh::OWNED);
  for (int c=0; c!=ncells; ++c) {
    // many terms here...
    // accumulation term is d/dt ( phi * (omega_g*n_g*s_g + n_l*s_l) )
    // note: s_g = (1 - s_l)
    // note: mol_frac of vapor is not a function of pressure
    // note: assumes phi does not depend on pressure
    double p = (*pres)("cell",0,c);
    double T = (*temp)("cell",0,c);
    double phi = (*poro)("cell",0,c);

    //  omega_g * sat_g * d(n_g)/dp
    double result = (*mol_frac_gas)(c) * (*sat_gas)(c) * eos_gas_->DMolarDensityDp(T,p);

    // + sat_l * d(n_l)/dp
    result += (*sat_liq)(c) * eos_liquid_->DMolarDensityDp(T,p);

    // + (n_l - omega_g * n_g) * d(sat_l)/d(p_c) * d(p_c)/dp
    result += -((*n_liq)(c) - (*mol_frac_gas)(c)*(*n_gas)(c))
      * wrm_->d_saturation((*p_atm) - p);

    Acc_cells[c] += result * (*cell_volume)(c) / h;
    Fc_cells[c] += result * (*cell_volume)(c) / h * p;
  }

  preconditioner_->ApplyBoundaryConditions(bc_markers_, bc_values_);
  preconditioner_->AssembleGlobalMatrices();
  preconditioner_->ComputeSchurComplement(bc_markers_, bc_values_);
  preconditioner_->UpdateMLPreconditioner();
};

}  // namespace Flow
}  // namespace Amanzi



