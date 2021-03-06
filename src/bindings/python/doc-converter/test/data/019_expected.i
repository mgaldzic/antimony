%feature("docstring") KineticLaw::getFormula "
 Returns the mathematical formula for this KineticLaw object and
 return  it as as a text string.

 This is fundamentally equivalent to  getMath().  This variant is
 provided principally for compatibility compatibility  with SBML Level
 1.

 Returns a string representing the formula of this KineticLaw.

 Note:

 SBML Level 1 uses a text-string format for mathematical formulas.
 SBML  Level 2 uses MathML, an XML format for representing
 mathematical  expressions. LibSBML provides an Abstract Syntax Tree
 API for working  with mathematical expressions; this API is more
 powerful than working  with formulas directly in text form, and ASTs
 can be translated into  either MathML or the text-string syntax. The
 libSBML methods that accept  text-string formulas directly (such as
 this constructor) are provided  for SBML Level 1 compatibility, but
 developers are encouraged to use the  AST mechanisms.

 See also getMath().
";
