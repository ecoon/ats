/* -*-  mode: c++; indent-tabs-mode: nil -*- */

/*
  A collection of WRMs along with a Mesh Partition.
  Authors: Ethan Coon (ecoon@lanl.gov)
*/

#include "dbc.hh"
#include "wrm_factory.hh"
#include "wrm_permafrost_factory.hh"
#include "wrm_partition.hh"


namespace Amanzi {
namespace Flow {

// Non-member factory
Teuchos::RCP<WRMPartition>
createWRMPartition(Teuchos::ParameterList& plist) {
  WRMFactory fac;
  std::vector<Teuchos::RCP<WRM> > wrm_list;
  std::vector<std::string> region_list;

  for (Teuchos::ParameterList::ConstIterator lcv=plist.begin();
       lcv!=plist.end(); ++lcv) {
    std::string name = lcv->first;
    if (plist.isSublist(name)) {
      Teuchos::ParameterList sublist = plist.sublist(name);
      region_list.push_back(sublist.get<std::string>("region"));
      wrm_list.push_back(fac.createWRM(sublist));
    } else {
      ASSERT(0);
    }
  }

  Teuchos::RCP<Functions::MeshPartition> part =
      Teuchos::rcp(new Functions::MeshPartition(AmanziMesh::CELL,region_list));

  return Teuchos::rcp(new WRMPartition(part , wrm_list));
}


// Non-member factory
Teuchos::RCP<WRMPermafrostModelPartition>
createWRMPermafrostModelPartition(Teuchos::ParameterList& plist,
        Teuchos::RCP<WRMPartition>& wrms) {

  // for each WRM create a permfrost_model
  WRMPermafrostFactory fac;
  std::vector<Teuchos::RCP<WRMPermafrostModel> > pm_list;

  for (WRMList::const_iterator wrm=wrms->second.begin();
       wrm!=wrms->second.end(); ++wrm) {
    pm_list.push_back(fac.createWRMPermafrostModel(plist, *wrm));
  }

  return Teuchos::rcp(new WRMPermafrostModelPartition(wrms->first, pm_list));
}

} // namespace
} // namespace
