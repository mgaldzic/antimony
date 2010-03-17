#ifndef NCELLML

#include "cellmlx.h"
#include "MathMLInputServices.h"
#include "ICellMLInputServices.h"
#include <nsStringAPI.h>
#include <nsDebug.h>
#include <nsCOMPtr.h>
#include <prtypes.h>
#include <nsServiceManagerUtils.h>

#define CELLML_BOOTSTRAP_CONTRACTID "@cellml.org/cellml-bootstrap;1"

void Module::LoadCellMLModel(nsCOMPtr<cellml_apiIModel> model, vector<nsCOMPtr<cellml_apiICellMLComponent> > top_components)
{
  assert(m_cellmlcomponent==NULL);
  if (m_cellmlmodel != NULL) {
    assert(false); 
  }
  m_cellmlmodel = model;

  //Translate
  nsString cellmltext;
  string cellmlname;

  //Components become sub-modules
  for (size_t comp=0; comp<top_components.size(); comp++) {
    //Load the component as a submodule (which should already have been loaded by the registry in LoadCellML)
    string compname = GetNameAccordingToEncapsulationParent(top_components[comp], model);
    string cellmlmodname = GetModuleNameFrom(top_components[comp]);
    Variable* var = AddOrFindVariable(&compname);
    if(var->SetModule(&cellmlmodname)) {
      assert(false);
      return;
    }
  }

  FixNames();  //In case the name of one of the modules is something like 'time'.
}

bool HasTimeUnits(nsCOMPtr<cellml_apiICellMLVariable> cmlvar) {
  nsresult rv;
  nsString cellmltext;
  rv = cmlvar->GetUnitsName(cellmltext);
  if (cellmltext == ToNSString("second")) return true;
  nsCOMPtr<cellml_apiIUnits> units;
  rv = cmlvar->GetUnitsElement(getter_AddRefs(units));
  if (units==NULL) return false;
  nsCOMPtr<cellml_apiIUnitSet> unitset;
  rv = units->GetUnitCollection(getter_AddRefs(unitset));
  nsCOMPtr<cellml_apiIUnitIterator> uniti;
  rv = unitset->IterateUnits(getter_AddRefs(uniti));
  nsCOMPtr<cellml_apiIUnit> unit;
  rv = uniti->NextUnit(getter_AddRefs(unit));
  while (unit != NULL) {
    rv = unit->GetUnits(cellmltext);
    if (cellmltext == ToNSString("second")) return true;
    rv = uniti->NextUnit(getter_AddRefs(unit));
  }
  return false;
}

void Module::LoadCellMLComponent(nsCOMPtr<cellml_apiICellMLComponent> component)
{
  nsString cellmltext;
  string cellmlname;
  nsresult rv;

  assert(m_cellmlmodel==NULL);
  if (m_cellmlcomponent != NULL) {
    assert(false);
  }
  //Variables
  nsCOMPtr<cellml_apiICellMLVariableSet> varset;
  rv = component->GetVariables(getter_AddRefs(varset));
  nsCOMPtr<cellml_apiICellMLVariableIterator> vsi;
  rv = varset->IterateVariables(getter_AddRefs(vsi));
  nsCOMPtr<cellml_apiICellMLVariable> cmlvar;
  rv = vsi->NextVariable(getter_AddRefs(cmlvar));
  while (cmlvar != NULL) {
    rv = cmlvar->GetName(cellmltext);
    cellmlname = ToThinString(cellmltext.get());
    Variable* antvar = AddOrFindVariable(&cellmlname);
    //antvar->SetIsConst(false); //This forces it to be output in the Antimony script even if it's not otherwise used.
    rv = cmlvar->GetInitialValue(cellmltext);
    cellmlname = ToThinString(cellmltext.get());
    if (cellmlname != "") {
      Formula* formula = g_registry.NewBlankFormula();
      setFormulaWithString(cellmlname, formula);
      antvar->SetFormula(formula);
    }
    //Put it in the module interface if it has one:
    PRUint32 vi;
    rv = cmlvar->GetPrivateInterface(&vi);
    if (vi == 2) {
      rv = cmlvar->GetPublicInterface(&vi);
    }
    if (vi != 2) {
      AddVariableToExportList(antvar);
    }
    
    rv = vsi->NextVariable(getter_AddRefs(cmlvar));
  }

  //Reactions
  nsCOMPtr<cellml_apiIReactionSet> rxnset;
  rv = component->GetReactions(getter_AddRefs(rxnset));
  nsCOMPtr<cellml_apiIReactionIterator> rxni;
  rv = rxnset->IterateReactions(getter_AddRefs(rxni));
  nsCOMPtr<cellml_apiIReaction> rxn;
  rv = rxni->NextReaction(getter_AddRefs(rxn));
  while (rxn != NULL) {
    //parse the reaction;
    //reactions don't have names, so make up a new one for Antimony
    //LS DEBUG:  stopped coding here for now...
    //Variable* rxnvar = AddOrFindVariable(&cellmlname);
    rv = rxni->NextReaction(getter_AddRefs(rxn));
  }
  
  //Math
  nsCOMPtr<cellml_apiIMathContainer> mc(do_QueryInterface(component));
  nsCOMPtr<cellml_apiIMathList> mathlist;
  rv = mc->GetMath(getter_AddRefs(mathlist));
  nsCOMPtr<cellml_apiIMathMLElementIterator> mli;
  rv = mathlist->Iterate(getter_AddRefs(mli));
  nsCOMPtr<mathml_domIMathMLElement> mathel;
  rv = mli->Next(getter_AddRefs(mathel));
  while(mathel != NULL) {
    nsCOMPtr<domINodeList> nodes;
    rv = mathel->GetChildNodes(getter_AddRefs(nodes));
    PRUint32 length;
    rv = nodes->GetLength(&length);
    for (PRUint32 i=0; i<length; i++) {
      nsCOMPtr<domINode> node;
      rv = nodes->Item(i, getter_AddRefs(node));
      nsCOMPtr<mathml_domIMathMLApplyElement> input(do_QueryInterface(node, &rv));
      if (input != NULL) {
        nsCString indent;
        indent = "";
        nsCString cinfix;
        MathMLInputServices mmlis;
        rv = mmlis.MathMLToInputFormat(input, NULL, indent, cinfix);
        string infix;
        infix = cinfix.get();
        string variable, equation;
        variable.assign(infix, 0, infix.find('='));
        equation.assign(infix, infix.find('=')+1, infix.size());
        variable = Trim(variable);
        equation = Trim(equation);
        string origeq = equation;

        //Remove all the '{id: ...}' bits
        size_t idpos;
        while ((idpos = equation.find("{id:")) != string::npos) {
          equation.erase(idpos, equation.find("}", idpos)-idpos+1);
        }
        //Remove '{unit: ...}' bits (for now)
        while ((idpos = equation.find("{unit:")) != string::npos) {
          equation.erase(idpos, equation.find("}", idpos)-idpos+1);
        }
        //Remove '{base: ...}' bits
        while ((idpos = equation.find("{base:")) != string::npos) {
          equation.erase(idpos, equation.find("}", idpos)-idpos+1);
        }
        //Remove '$'--it's apparantly the way that cellml notes variables that are also function names.
        // (We'll fix the name later ourselves with 'FixNames')
        while ((idpos = equation.find("$")) != string::npos) {
          equation.erase(idpos, 1);
        }
        while ((idpos = variable.find("$")) != string::npos) {
          variable.erase(idpos, 1);
        }

        //Handle piecewise equations
        if ((idpos = equation.find("piecewise")) != string::npos) {
          equation = CellMLPiecewiseToSBML(equation);
        }
        Formula* formula = g_registry.NewBlankFormula();
        setFormulaWithString(Trim(equation), formula);

        //Find out what variable we're assigning to, and how we're assigning to it.
        vector<string> fullname;
        fullname.push_back(variable);
        Variable* var = GetVariable(fullname);
        if (var != NULL) {
          //Math is simple assignent rule
          if (var->SetAssignmentRule(formula)) {
            //Something went wrong
            //cout << "Unable to use the formula \"" << formula->ToDelimitedStringWithEllipses('.') << "\" (originally \"" << origeq << "\") to set the assignment rule for " << var->GetNameDelimitedBy('.') << ":  " << getLastError() << endl;
            cout << "Unable to use the formula \"" << formula->ToDelimitedStringWithEllipses('.') << "\" to set the assignment rule for " << var->GetNameDelimitedBy('.') << ":  " << getLastError() << endl;
          }
        }
        else if (variable.find("d(") == 0 && variable.find(")/d(") != string::npos) {
          //Math is of the form dx/dy--if y is time, we can make this a rate rule.
          size_t timepos = variable.find(")/d(")+4;
          string maybetime;
          maybetime.assign(variable, timepos, variable.find(')', timepos)-timepos);
          cellmltext = ToNSString(maybetime);
          rv = varset->GetVariable(cellmltext, getter_AddRefs(cmlvar));
          if (cmlvar && !HasTimeUnits(cmlvar)) {
            rv = cmlvar->GetUnitsName(cellmltext);
            cout << "The units of \"" << maybetime << "\" ('" << ToThinString(cellmltext.get()) << "') do not have 'seconds' as their base unit, so assuming this CellML model is trying to take the derivative of something with respect to some not-time element, we are not translating this derivative." << endl;
            continue;
          }
          variable.assign(variable, 2, timepos-6);
          var = AddOrFindVariable(&variable);
          if (var->SetRateRule(formula)) {
            cout << "Unable to use the formula \"" << formula->ToDelimitedStringWithEllipses('.') << "\" to set the rate rule for " << var->GetNameDelimitedBy('.') << endl;
          }
        }
        else if (variable.find("del(") != string::npos) {
          //It's a partial differential equation.
          cout << "Unable to translate an assignment to \"" << variable << "\" in the Antimony format because Antimony does not handle partial differential equations." << endl;
        }
        else if (variable.find("selector(") != string::npos) {
          //It's vector or matrix math of some sort.
          cout << "Unable to translate an assignment to \"" << variable << "\" in the Antimony format because Antimony does not handle vector or matrix algebra." << endl;
        }
        else {
          //Unable to determine what kind of math we're talking about.
          cout << "Unable to figure out how to translate an assignment to \"" << variable << "\" in the Antimony format." << endl;
          
        }

      }
      else {
        //cout << "Child node " <<  i << " of 'math' element not an 'apply' element" << endl;
      }
    }
    rv = mli->Next(getter_AddRefs(mathel));
  }

  //Containers (?)

  //And finally, fix names.
  FixNames();
}

void Module::SetCellMLChildrenAsSubmodules(nsCOMPtr<cellml_apiICellMLComponent> component) {
  //Iterate over 'encapsulation' children and make them submodules.
  nsString cellmltext;
  string cellmlname;
  nsresult rv;
  nsCOMPtr<cellml_apiICellMLComponentSet> children;
  rv = component->GetEncapsulationChildren(getter_AddRefs(children));
  nsCOMPtr<cellml_apiICellMLComponentIterator> childi;
  nsCOMPtr<cellml_apiICellMLComponent> child;
  
  rv = children->IterateComponents(getter_AddRefs(childi));
  rv = childi->NextComponent(getter_AddRefs(child));
  while (child != NULL) {
    rv = child->GetName(cellmltext);
    cellmlname = ToThinString(cellmltext.get());
    string cellmlmodname = GetModuleNameFrom(child);
    vector<string> fullname;
    fullname.push_back(cellmlname);
    Variable* foundvar = GetVariable(fullname);
    if (foundvar != NULL) {
      cellmlname = cellmlname + "_mod";
    }
    Variable* var = AddOrFindVariable(&cellmlname);
    if(var->SetModule(&cellmlmodname)) {
      assert(false);
      return;
    }
    rv = childi->NextComponent(getter_AddRefs(child));
  }
}


const nsCOMPtr<cellml_apiIModel> Module::GetCellMLModel()
{
  if (m_cellmlmodel==NULL) {
    CreateCellMLModel();
  }
  else {
    nsString cellmltext;
    m_cellmlmodel->GetName(cellmltext);
    if (ToThinString(cellmltext.get()) != m_modulename) {
      CreateCellMLModel();
    }
  }
  return m_cellmlmodel;
}

void Module::CreateCellMLModel()
{
  nsresult rv;
  nsString cellmltext;
  wstring cellmlwstring;

  if (m_cellmlmodel != NULL) {
    if (m_cellmlcomponent != NULL) {
      m_cellmlcomponent = NULL;
    }
  }

  nsCOMPtr<cellml_apiICellMLBootstrap> boot(do_GetService(CELLML_BOOTSTRAP_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, );
  rv = boot->CreateModel(NS_LITERAL_STRING("1.1"), getter_AddRefs(m_cellmlmodel));
  NS_ENSURE_SUCCESS(rv, );
  nsCOMPtr<cellml_apiICellMLComponentSet> components;
  rv = m_cellmlmodel->GetLocalComponents(getter_AddRefs(components));

  cellmltext = ToNSString(m_modulename);
  m_cellmlmodel->SetName(cellmltext);
  //Create units

  //Create a component for all local variables
  m_cellmlcomponent = CreateCellMLComponentFor(m_cellmlmodel);
  m_cellmlmodel->AddElement(m_cellmlcomponent);

  //Create all connections

  //Create groups (?)
}

nsCOMPtr<cellml_apiICellMLComponent> Module::CreateCellMLComponentFor(nsCOMPtr<cellml_apiIModel> model)
{
  nsCOMPtr<cellml_apiICellMLComponent> newc;
  for (size_t var=0; var<m_variables.size(); var++) { 
    Variable* variable = m_variables[var];
    switch(variable->GetType()) {
    case varModule:
      model->AddElement(variable->GetModule()->CreateCellMLComponentFor(model));
      break;
    default:
      break;
      //LS DEBUG:  add stuff here
    }
  }
  return newc;
}

#endif
