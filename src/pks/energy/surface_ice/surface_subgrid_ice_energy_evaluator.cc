/* -*-  mode: c++; c-default-style: "google"; indent-tabs-mode: nil -*- */

/* -----------------------------------------------------------------------------
ATS

Authors: Ethan Coon (ecoon@lanl.gov)

FieldEvaluator for water content.

Wrapping this conserved quantity as a field evaluator makes it easier to take
derivatives, keep updated, and the like.  The equation for this is simply:

WC = phi * (s_liquid * n_liquid + omega_gas * s_gas * n_gas)

This is simply the conserved quantity in Richards equation.
----------------------------------------------------------------------------- */


#include "surface_subgrid_ice_energy_evaluator.hh"

namespace Amanzi {
namespace Energy {

SurfaceSubgridIceEnergyEvaluator::SurfaceSubgridIceEnergyEvaluator(Teuchos::ParameterList& plist) :
    SecondaryVariableFieldEvaluator(plist) {
 
  
  sg_model_ = plist.get<bool>("subgrid model",true);
  assert(sg_model_);

  if(my_key_.empty())
    my_key_ = plist_.get<std::string>("energy key", "surface-energy");
 
 
  std::string domain = Keys::getDomain(my_key_);

  // densities
  dens_key_ = plist.get<std::string>("molar density liquid key",
          Keys::getKey(domain, "molar_density_liquid"));
  dependencies_.insert(dens_key_);

  dens_ice_key_ = plist.get<std::string>("molar density ice key",
          Keys::getKey(domain, "molar_density_ice"));
  dependencies_.insert(dens_ice_key_);

  // internal energies
  ie_key_ = plist.get<std::string>("internal energy liquid key",
          Keys::getKey(domain, "internal_energy_liquid"));
  dependencies_.insert(ie_key_);

  ie_ice_key_ = plist.get<std::string>("internal energy ice key",
          Keys::getKey(domain, "internal_energy_ice"));
  dependencies_.insert(ie_ice_key_);

  // unfrozen fraction

  uf_key_ = plist.get<std::string>("unfrozen fraction key", Keys::getKey(domain,"unfrozen_fraction"));
  dependencies_.insert(uf_key_);

  // ponded depth
  height_key_ = plist.get<std::string>("height key", Keys::getKey(domain,"ponded_depth"));

  dependencies_.insert(height_key_);

  vpd_key_ = plist.get<std::string>("volumetric height key", Keys::getKey(domain,"volumetric_ponded_depth"));
  
  dependencies_.insert(vpd_key_);
  
  delta_max_key_ = plist_.get<std::string>("maximum ponded depth key", Keys::getKey(domain,"maximum_ponded_depth"));
  dependencies_.insert(delta_max_key_);

  delta_ex_key_ = plist_.get<std::string>("excluded volume key", Keys::getKey(domain,"excluded_volume"));
  dependencies_.insert(delta_ex_key_);
  //delta_max_ = plist_.get<double>("maximum ponded depth");
  //delta_ex_ = plist_.get<double>("excluded volume");

  cv_key_ = plist.get<std::string>("cell volume key",
          Keys::getKey(domain, "cell_volume"));
  //assert(delta_max_ > 0);

};

SurfaceSubgridIceEnergyEvaluator::SurfaceSubgridIceEnergyEvaluator(const SurfaceSubgridIceEnergyEvaluator& other) :
    SecondaryVariableFieldEvaluator(other),
    dens_key_(other.dens_key_),
    dens_ice_key_(other.dens_ice_key_),
    ie_key_(other.ie_key_),
    ie_ice_key_(other.ie_ice_key_),
    uf_key_(other.uf_key_),
    height_key_(other.height_key_),
    cv_key_(other.cv_key_),
    vpd_key_(other.vpd_key_),
    sg_model_(other.sg_model_),
    delta_max_key_(other.delta_max_key_),
    delta_ex_key_(other.delta_ex_key_){};
    
Teuchos::RCP<FieldEvaluator>
SurfaceSubgridIceEnergyEvaluator::Clone() const {
  return Teuchos::rcp(new SurfaceSubgridIceEnergyEvaluator(*this));
};


void SurfaceSubgridIceEnergyEvaluator::EvaluateField_(const Teuchos::Ptr<State>& S,
        const Teuchos::Ptr<CompositeVector>& result) {
  const Epetra_MultiVector& n_l = *S->GetFieldData(dens_key_)
      ->ViewComponent("cell",false);
  const Epetra_MultiVector& n_i = *S->GetFieldData(dens_ice_key_)
      ->ViewComponent("cell",false);

  const Epetra_MultiVector& u_l = *S->GetFieldData(ie_key_)
      ->ViewComponent("cell",false);
  const Epetra_MultiVector& u_i = *S->GetFieldData(ie_ice_key_)
      ->ViewComponent("cell",false);

  const Epetra_MultiVector& eta = *S->GetFieldData(uf_key_)
      ->ViewComponent("cell",false);
  const Epetra_MultiVector& height = *S->GetFieldData(height_key_)
      ->ViewComponent("cell",false);

  const Epetra_MultiVector& cv = *S->GetFieldData(cv_key_)
      ->ViewComponent("cell",false);

  const Epetra_MultiVector& vpd = *S->GetFieldData(vpd_key_)
      ->ViewComponent("cell",false);
 

  Epetra_MultiVector& result_v = *result->ViewComponent("cell",false);
  
  int ncells = result_v.MyLength();
  
  for (int c=0; c!=ncells; ++c) {
    result_v[0][c] = vpd[0][c] * ( eta[0][c] * n_l[0][c] * u_l[0][c]
                                   + (1. - eta[0][c]) * n_i[0][c] * u_i[0][c]);
      result_v[0][c] *= cv[0][c];
  }
  
};


void SurfaceSubgridIceEnergyEvaluator::EvaluateFieldPartialDerivative_(
    const Teuchos::Ptr<State>& S, Key wrt_key,
    const Teuchos::Ptr<CompositeVector>& result) {

  const Epetra_MultiVector& n_l = *S->GetFieldData(dens_key_)
      ->ViewComponent("cell",false);
  const Epetra_MultiVector& n_i = *S->GetFieldData(dens_ice_key_)
      ->ViewComponent("cell",false);

  const Epetra_MultiVector& u_l = *S->GetFieldData(ie_key_)
      ->ViewComponent("cell",false);
  const Epetra_MultiVector& u_i = *S->GetFieldData(ie_ice_key_)
      ->ViewComponent("cell",false);

  const Epetra_MultiVector& eta = *S->GetFieldData(uf_key_)
      ->ViewComponent("cell",false);
  const Epetra_MultiVector& height = *S->GetFieldData(height_key_)
      ->ViewComponent("cell",false);

  const Epetra_MultiVector& cv = *S->GetFieldData(cv_key_)
      ->ViewComponent("cell",false);

  const Epetra_MultiVector& max_pd_v = *S->GetFieldData(delta_max_key_)
    ->ViewComponent("cell",false);
  
  const Epetra_MultiVector& ex_vol_v = *S->GetFieldData(delta_ex_key_)
    ->ViewComponent("cell",false);

  Epetra_MultiVector& result_v = *result->ViewComponent("cell",false);

  int ncells = result_v.MyLength();

  
  //assert(delta_max_ > 0);
  assert (max_pd_v[0][3] > 0.);
  assert (ex_vol_v[0][3] > 0.);
  
  double wc1 = 0;

  if (wrt_key == height_key_) {
    double dh;
    for (int c=0; c!=ncells; ++c) {
      double delta_max = max_pd_v[0][c];
      double delta_ex = ex_vol_v[0][c];
      if (height[0][c] <= delta_max)
        dh = 2*height[0][c]*(2*delta_max - 3*delta_ex )/ std::pow(delta_max,2) 
          + 3*height[0][c]*height[0][c]*(2*delta_ex - delta_max)/std::pow(delta_max,3);
      else
        dh = 1.0;
      
      result_v[0][c] = dh * ( eta[0][c] * n_l[0][c] * u_l[0][c]
                              + (1. - eta[0][c]) * n_i[0][c] * u_i[0][c]);
      result_v[0][c] *= cv[0][c];
    } 
  }
  else if (wrt_key == vpd_key_) {
    for (int c=0; c!=ncells; ++c) {
      result_v[0][c] = 1.*(eta[0][c] * n_l[0][c] * u_l[0][c]
                          + (1. - eta[0][c]) * n_i[0][c] * u_i[0][c]);
      result_v[0][c] *= cv[0][c];
    }
  }
  else if (wrt_key == uf_key_) {
      //const Epetra_MultiVector& vpd = *S->GetFieldData(vpd_key_)->ViewComponent("cell",false);
      for (int c=0; c!=ncells; ++c) {
        double delta_max = max_pd_v[0][c];
        double delta_ex = ex_vol_v[0][c];
        if (height[0][c] <= delta_max) 
          wc1 = std::pow(height[0][c],2)*(2*delta_max - 3*delta_ex)/std::pow(delta_max,2) + std::pow(height[0][c],3)*(2*delta_ex - delta_max)/std::pow(delta_max,3);
        else
          wc1 = height[0][c] - delta_ex;
        
        result_v[0][c] = wc1 * (n_l[0][c] * u_l[0][c] - n_i[0][c] * u_i[0][c]);
        result_v[0][c] *= cv[0][c];
      }
  } else if (wrt_key == dens_key_) {
    //const Epetra_MultiVector& vpd = *S->GetFieldData(vpd_key_)->ViewComponent("cell",false);
    for (int c=0; c!=ncells; ++c) {
      double delta_max = max_pd_v[0][c];
      double delta_ex = ex_vol_v[0][c];
      if (height[0][c] <= delta_max) 
        wc1 = std::pow(height[0][c],2)*(2*delta_max - 3*delta_ex)/std::pow(delta_max,2) + std::pow(height[0][c],3)*(2*delta_ex - delta_max)/std::pow(delta_max,3);
      else
        wc1 = height[0][c] - delta_ex;
        
      result_v[0][c] = wc1 * eta[0][c] * u_l[0][c];
      result_v[0][c] *= cv[0][c];
    }

  } else if (wrt_key == dens_ice_key_) {
     //const Epetra_MultiVector& vpd = *S->GetFieldData(vpd_key_)->ViewComponent("cell",false);
    for (int c=0; c!=ncells; ++c) {
      double delta_max = max_pd_v[0][c];
      double delta_ex = ex_vol_v[0][c];
      if (height[0][c] <= delta_max) 
        wc1 = std::pow(height[0][c],2)*(2*delta_max - 3*delta_ex)/std::pow(delta_max,2) + std::pow(height[0][c],3)*(2*delta_ex - delta_max)/std::pow(delta_max,3);
      else
        wc1 = height[0][c] - delta_ex;
      
      result_v[0][c] = wc1 * (1. - eta[0][c]) * u_i[0][c];
      result_v[0][c] *= cv[0][c];
    }
  } else if (wrt_key == ie_key_) {
    //const Epetra_MultiVector& vpd = *S->GetFieldData(vpd_key_)->ViewComponent("cell",false);
    for (int c=0; c!=ncells; ++c) {
      double delta_max = max_pd_v[0][c];
      double delta_ex = ex_vol_v[0][c];
      if (height[0][c] <= delta_max) 
        wc1 = std::pow(height[0][c],2)*(2*delta_max - 3*delta_ex)/std::pow(delta_max,2) + std::pow(height[0][c],3)*(2*delta_ex - delta_max)/std::pow(delta_max,3);
      else
        wc1 = height[0][c] - delta_ex;
      
      result_v[0][c] = wc1 * eta[0][c] * n_l[0][c];
      result_v[0][c] *= cv[0][c];
    }
  } else if (wrt_key == ie_ice_key_) {
    // const Epetra_MultiVector& vpd = *S->GetFieldData(vpd_key_)->ViewComponent("cell",false);
    for (int c=0; c!=ncells; ++c) {
      double delta_max = max_pd_v[0][c];
      double delta_ex = ex_vol_v[0][c];
      if (height[0][c] <= delta_max) 
        wc1 = std::pow(height[0][c],2)*(2*delta_max - 3*delta_ex)/std::pow(delta_max,2) + std::pow(height[0][c],3)*(2*delta_ex - delta_max)/std::pow(delta_max,3);
      else
        wc1 = height[0][c] - delta_ex;
      
      result_v[0][c] = wc1 * (1. - eta[0][c]) * n_i[0][c];
      result_v[0][c] *= cv[0][c];
    }
    
  } else {
    std::cout<<"SURFACE_SUBGRID_ICE_ENERGY: NO DERIVATIVE EXITS: "<<wrt_key<<"\n";
    ASSERT(0);
  }
};


} //namespace
} //namespace
