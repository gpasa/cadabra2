
#include <boost/python.hpp>
#include "Functional.hh"
#include "SympyCdb.hh"
#include "PreClean.hh"
#include "Cleanup.hh"
#include "Parser.hh"
#include "Kernel.hh"
#include "DisplaySympy.hh"
#include "algorithms/substitute.hh"

Ex::iterator sympy::apply(const Kernel& kernel, Ex& ex, Ex::iterator& it, const std::string& head, const std::string& args, 
								  const std::string& method)
	{
	// We first need to print the sub-expression using DisplaySympy,
	// optionally with the head wrapped around it and the args added
	// (if present).

	std::ostringstream str;

	if(head.size()>0)
		str << head << "(";

	DisplaySympy ds(kernel, ex);
	ds.output(str, it);

	if(head.size()>0)
		if(args.size()>0) 
			str << ", " << args << ")";
	str << method;

	if(head.size()>0)
		str << ")";

	// We then execute the expression in Python.

	//ex.print_recursive_treeform(std::cerr, it);
	std::cerr << "feeding " << str.str() << std::endl;

	auto module = boost::python::import("sympy.parsing.sympy_parser");
	auto parse  = module.attr("parse_expr");
	boost::python::object obj = parse(str.str());
	//std::cerr << "converting result to string" << std::endl;
	auto __str__ = obj.attr("__str__");
	boost::python::object res = __str__();
	std::string result = boost::python::extract<std::string>(res);
	std::cerr << result << std::endl;
	

   // After that, we construct a new sub-expression from this string by using our
   // own parser, and replace the original.

	auto ptr = std::make_shared<Ex>();
	Parser parser(ptr);
	std::stringstream istr(result);
	istr >> parser;

	pre_clean_dispatch_deep(kernel, *parser.tree);
   cleanup_dispatch_deep(kernel, *parser.tree);

	//parser.tree->print_recursive_treeform(std::cerr, parser.tree->begin());

	ds.import(*parser.tree);

	Ex::iterator first=parser.tree->begin();
   it = ex.move_ontop(it, first);

	return it;
	}

Ex sympy::invert_matrix(const Kernel& kernel, Ex& ex, Ex& rules)
	{
	// check that object has two children only.
	if(ex.number_of_children(ex.begin())!=2) {
		throw ConsistencyException("Object should have exactly two indices.");
		}

	Ex::iterator ind1=ex.child(ex.begin(), 0);
	Ex::iterator ind2=ex.child(ex.begin(), 1);

	str_node::parent_rel_t prel;
	if(ind1->fl.parent_rel==str_node::p_sub)   prel=str_node::p_super;
	if(ind1->fl.parent_rel==str_node::p_super) prel=str_node::p_sub;

	Ex ret;

	// Get Indices property and from there Coordinates.

	const Indices *prop1 = kernel.properties.get<Indices>(ind1);
	const Indices *prop2 = kernel.properties.get<Indices>(ind2);
	
	if(prop1!=prop2 || prop1==0) 
		throw ConsistencyException("Need the indices of object to be declared with Indices property.");

	// Run over all values of Coordinates, construct matrix.

	Ex matrix("\\matrix");
	auto cols=matrix.append_child(matrix.begin(), str_node("\\comma"));
	for(unsigned c1=0; c1<prop1->values.size(); ++c1) {
		auto row=matrix.append_child(cols, str_node("\\comma"));
		for(unsigned c2=0; c2<prop1->values.size(); ++c2) {
			// Generate an expression with this component, apply substitution, then stick 
			// the result into the string that will go to sympy.

			Ex c(ex.begin());
			Ex::iterator cit1=c.child(c.begin(), 0);
			Ex::iterator cit2=c.child(c.begin(), 1);
			cit1=c.replace(cit1, prop1->values[c1].begin());
			cit2=c.replace(cit2, prop1->values[c2].begin());
			cit1->fl.parent_rel=prel;
			cit2->fl.parent_rel=prel;

			Ex::iterator cit=c.begin();
			substitute subs(kernel, c, rules);
			if(subs.can_apply(cit)) {
				subs.apply(cit);
				matrix.append_child(row, cit);
				}
			else {
				zero( matrix.append_child(row, str_node("1"))->multiplier );
				}
			}
		}
	
	auto top=matrix.begin();
	sympy::apply(kernel, matrix, top, "", "", ".inv()");
	//matrix.print_recursive_treeform(std::cerr, top);

	Ex::iterator ruleslist=rules.begin();

	// Now we need to iterate over the components again and construct sparse rules.
	cols=matrix.begin(matrix.begin()); // outer comma
	auto row=matrix.begin(cols); // first inner comma
	for(unsigned c1=0; c1<prop1->values.size(); ++c1) {
		auto el =matrix.begin(row);  // first element of first inner comma
		for(unsigned c2=0; c2<prop2->values.size(); ++c2) {
			if(el->is_zero()==false) {
				Ex rule("\\equals");
				auto rit  = rule.append_child(rule.begin(), ex.begin());
				auto cvit = rule.append_child(rule.begin(), Ex::iterator(el));
				auto i = rule.begin(rit);
				//std::cerr << c1 << ", " << c2 << std::endl;
				i = rule.replace(i, prop1->values[c1].begin());
				i->fl.parent_rel=ind1->fl.parent_rel;
				++i;
				i = rule.replace(i, prop1->values[c2].begin());
				i->fl.parent_rel=ind1->fl.parent_rel;
				rules.append_child(ruleslist, rule.begin());
				//rule.print_recursive_treeform(std::cerr, rule.begin());
				}
			++el;
			}
		++row;
		}

	return ret;
	}
