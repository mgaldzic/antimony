#include <cassert>
#include "formula.h"
#include "reactantlist.h"
#include "registry.h"
#include "sbmlx.h"
#include "stringx.h"
#include "module.h"
#include "variable.h"
#include <iostream>

using namespace std;

void ReactantList::AddReactant(Variable* var, double stoichiometry)
{
  m_components.push_back(make_pair(stoichiometry, var->GetName()));
  m_module = var->GetNamespace();
  //var->SetType(varSpeciesUndef); //<-might actually be a formula for -o things
  //cout << "New version: " << ToString() << endl;
}

void ReactantList::SetNewTopName(string newmodname, string newtopname)
{
  m_module = newmodname;
  for (size_t component=0; component<m_components.size(); component++) {
    m_components[component].second.insert(m_components[component].second.begin(), newtopname);
  }
}

void ReactantList::SetComponentCompartments(Variable* compartment, var_type supercomp)
{
  for (size_t component=0; component<m_components.size(); component++) {
    Module* module = g_registry.GetModule(m_module);
    assert(module != NULL);
    Variable* var = module->GetVariable(m_components[component].second);
    if (var != NULL) {
      var->SetSuperCompartment(compartment, supercomp);
    }
  }
}

bool ReactantList::SetComponentTypesTo(var_type vtype)
{
  for (size_t component=0; component<m_components.size(); component++) {
    Module* module = g_registry.GetModule(m_module);
    assert(module != NULL);
    Variable* var = module->GetVariable(m_components[component].second);
    if (var != NULL) {
      if (var->SetType(vtype)) return true;
    }
  }
  return false;
}

bool ReactantList::SetComponentFormulasTo(Formula form)
{
  for (size_t component=0; component<m_components.size(); component++) {
    Module* module = g_registry.GetModule(m_module);
    assert(module != NULL);
    Variable* var = module->GetVariable(m_components[component].second);
    if (var != NULL) {
      if (var->SetFormula(&form)) return true;
    }
  }
  return false;
}

bool ReactantList::ClearReferencesTo(Variable* deletedvar)
{
  if (m_module.empty()) {
    //There are no reactants in this list, so we don't need to clear it.
    return false;
  }
  Module* module = g_registry.GetModule(m_module);
  assert(module != NULL);

  bool ret = false;
  for (vector<pair<double, vector<string> > >::iterator component = m_components.begin(); component != m_components.end(); ) {
    Variable* var = module->GetVariable(component->second);
    if (var->GetIsEquivalentTo(deletedvar)) {
      component = m_components.erase(component);
      ret = true;
    }
    else {
      component++;
    }
  }
  return ret;
}

vector<vector<string> > ReactantList::GetVariableList() const
{
  vector<vector<string> > vlist;
  for (size_t var=0; var<m_components.size(); var++) {
    vlist.push_back(m_components[var].second);
  }
  return vlist;
}

string ReactantList::ToStringDelimitedBy(std::string cc) const
{
  string retval;
  for (size_t component=0; component<m_components.size(); component++) {
    if (component > 0) {
      retval += " + ";
    }
    if (m_components[component].first != 1) {
      char charnum[50];
      sprintf(charnum, "%g", m_components[component].first);
      retval += charnum;
    }
    vector<string> varname = m_components[component].second;
    Variable* var = g_registry.GetModule(m_module)->GetVariable(m_components[component].second);
    if (var->GetIsConst()) {
      retval += "$";
    }
    retval += (var->GetNameDelimitedBy(cc));
  }
  return retval;
}

vector<string> ReactantList::ToStringVecDelimitedBy(string cc) const
{
  vector<string> retval;
  for (size_t component=0; component<m_components.size(); component++) {
    vector<string> varname = m_components[component].second;
    retval.push_back(g_registry.GetModule(m_module)->GetVariable(varname)->GetNameDelimitedBy(cc));
  }
  return retval;
}

vector<double> ReactantList::GetStoichiometries() const
{
  vector<double> retval;
  for (size_t component=0; component<m_components.size(); component++) {
    retval.push_back(m_components[component].first);
  }
  return retval;
}

double ReactantList::GetStoichiometryFor(const Variable* var) const
{
  double stoich = 0;
  for (size_t component=0; component<m_components.size(); component++) {
    Module* module = g_registry.GetModule(m_module);
    assert(module != NULL);
    Variable* compvar = module->GetVariable(m_components[component].second);
    assert(compvar != NULL);
    if (compvar->GetIsEquivalentTo(var)) {
      stoich += m_components[component].first;
    }
  }
  return stoich;
}

double ReactantList::GetStoichiometryFor(size_t n) const
{
  if (n>=m_components.size()) return 0.0;
  return m_components[n].first;
}

const Variable* ReactantList::GetNthReactant(size_t n) const
{
  if (n>=m_components.size()) return NULL;
  Module* module = g_registry.GetModule(m_module);
  assert(module != NULL);
  return module->GetVariable(m_components[n].second);
}

void ReactantList::FixNames()
{
  for (size_t comp=0; comp<m_components.size(); comp++) {
    FixName(m_components[comp].second);
  }
  FixName(m_module);
}

bool ReactantList::Matches(const ReactantList* newrl) const
{
  vector<vector<string> > newvariables = newrl->GetVariableList();
  vector<double> newstoichs = newrl->GetStoichiometries();
  if (m_components.size() != newvariables.size()) return false;
  for (size_t component=0; component<m_components.size(); component++) {
    if (m_components[component].first != newstoichs[component]) return false;
    Module* module = g_registry.GetModule(m_module);
    assert(module != NULL);
    Variable* origvar = module->GetVariable(m_components[component].second);
    assert(origvar != NULL);
    Variable* newvar = module->GetVariable(newvariables[component]);
    assert(newvar != NULL);
    if (origvar->GetSameVariable() != newvar->GetSameVariable()) return false;
  }
  return true;
}
