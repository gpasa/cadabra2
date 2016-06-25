/* 

	Cadabra: a field-theory motivated computer algebra system.
	Copyright (C) 2001-2014  Kasper Peeters <kasper.peeters@phi-sci.com>

   This program is free software: you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
*/

#include "Props.hh"
#include "Exceptions.hh"
#include "Compare.hh"
#include <stdlib.h>
#include <typeinfo>
#include <iostream>
#include <sstream>
#include "properties/Indices.hh"

pattern::pattern()
	{
	}

pattern::pattern(const Ex& o)
	: obj(o)
	{
	}

bool pattern::match(const Properties& properties, const Ex::iterator& it, bool ignore_parent_rel) const
	{
	// Special case for range wildcards.
	// FIXME: move this to storage.cc (see the FIXME there)

	if(it->name==obj.begin()->name && children_wildcard()) {
		Ex::iterator hm=obj.begin(obj.begin());
		if(Ex::number_of_children(hm)==0) {
			return true; // # without arguments
			}
		Ex::iterator hmarg=hm.begin();
		Ex::iterator seqarg=hm; 
		const Indices *ind=0;

		if(*hmarg->name=="\\comma" || *hmarg->name!="\\sequence") {
			Ex::iterator stt=hmarg;
			if(*hmarg->name=="\\comma") {
				stt=hmarg.begin();
				seqarg=hmarg.begin();
				seqarg.skip_children();
				++seqarg;
				}
			ind=properties.get<Indices>(stt, true);
			}
		else seqarg=hmarg;

		if(seqarg!=hm) {
			Ex::sibling_iterator seqit=seqarg.begin();
			unsigned int from=to_long(*seqit->multiplier);
			++seqit;
			unsigned int to  =to_long(*seqit->multiplier);

			if(Ex::number_of_children(it)<from ||
				Ex::number_of_children(it)>to ) 
				return false;
			}
		
		if(ind!=0) {
			Ex::sibling_iterator indit=it.begin();
			while(indit!=it.end()) {
				const Indices *gi=properties.get<Indices>(indit, true);
				if(gi!=ind) {
					return false;
					}
				++indit;
				}
			}
		
		return true;
		}

	// Cases without range wildcard.
//	if(obj.number_of_children(obj.begin())>0)
//		std::cerr << "comparing: " << ignore_parent_rel << "\n" << Ex(it) << " " << obj << std::endl;
//	Ex::print_recursive_treeform(txtout, it);
//	Ex::print_recursive_treeform(txtout, obj.begin());


	// This does not work, because it does not flip parent rels (disabled for a reason?)
	// FIXME: fix logic for subtree/Ex_compare usage.
//	std::cerr << "Comparing " << Ex(it) <<  " with " << obj << " " << ignore_parent_rel << std::endl;
	int res=subtree_compare(&properties, it, obj.begin(), ignore_parent_rel?0:-3, false /* was true; but that leads to infinite recurion */, 0);
	//std::cerr << res << std::endl;

	// This should work better, but this is _not_ allowed (and crashes in an infinite recursion)
	// because Ex_comparator tries to fetch property information which then gets back here.
	// Ex_comparator comp(properties);
	// int res=comp.equal_subtree(it, obj.begin());

	//txtout << res << std::endl;
	if(abs(res)<=1) {
//		if(obj.number_of_children(obj.begin())>0)
//			std::cerr << "match!" << std::endl;
		return true;
		}

	return false;
	}

bool pattern::children_wildcard() const	
	{
	if(Ex::number_of_children(obj.begin())==1) 
		if(obj.begin(obj.begin())->is_range_wildcard())
			return true;
	return false;
	}

bool Properties::has(const property *pb, Ex::iterator it) 
	{
	std::pair<property_map_t::iterator, property_map_t::iterator> pit=props.equal_range(it->name);
	while(pit.first!=pit.second) {
//		txtout << *it->name << std::endl;
//		txtout << typeid(pit.first->second.second).name() << " versus " 
//				 << typeid(pb).name() << std::endl;
		if(typeid(*(pit.first->second.second))==typeid(*pb) && 
			pit.first->second.first->match(*this, it))  // match found
			return true;
		++pit.first;
		}
	return false;
	}

void Properties::clear() 
	{
	// Clear and free the property lists. Since pointers to properties can
	// be shared, we use the pats map and make sure that we only free each
	// property* pointer once.
	pattern_map_t::const_iterator it=pats.begin();
	const property *previous=0;
	while(it!=pats.end()) {
		 if(previous!=it->first) {
			  previous=it->first;
			  delete it->first;
			  }
		 delete it->second;
		 ++it;
		 }
	props.clear();
	pats.clear();
	}

Properties::registered_property_map_t::~registered_property_map_t()
	{
	// FIXME: V2
	}

void Properties::register_property(property* (*fun)(), const std::string& name)
	{
	registered_properties.store[name]=fun;
	}

keyval_t::const_iterator keyval_t::find(const std::string& key) const
	{
	keyval_t::const_iterator it=keyvals.begin();
	while(it!=keyvals.end()) {
		 if(it->first==key)
			  break;
		 ++it;
		 }
	return it;
	}

keyval_t::iterator keyval_t::find(const std::string& key) 
	{
	keyval_t::iterator it=keyvals.begin();
	while(it!=keyvals.end()) {
		 if(it->first==key)
			  break;
		 ++it;
		 }
	return it;
	}

keyval_t::const_iterator keyval_t::begin() const
	{
	return keyvals.begin();
	}

keyval_t::const_iterator keyval_t::end() const
	{
	return keyvals.end();
	}

void keyval_t::push_back(const kvpair_t& kv) 
	{
	keyvals.push_back(kv);
	}

void keyval_t::erase(iterator it)
	{
	keyvals.erase(it);
	}


bool property::parse(const Kernel&, keyval_t& keyvals)
	{
	return true;

//	if(tr.number_of_children(arg)==0) return true;
//	txtout << name() << ": should not have any arguments." << std::endl;
//	return false;
	}

void property::validate(const Kernel&, const Ex&) const
	{
	}

bool property::parse_one_argument(Ex::iterator arg, keyval_t& keyvals)
	{
	if(*arg->name=="\\equals") {
		Ex::sibling_iterator key=arg.begin();
		if(key==arg.end()) return false;
		Ex::sibling_iterator val=key;
		++val;
		if(val==arg.end()) return false;
		keyvals.push_back(keyval_t::value_type(*arg.begin()->name, val));
		}
	else {
		if(unnamed_argument()!="") {
			// std::cerr << unnamed_argument() << " unnamed " << *arg->name << std::endl;
			keyvals.push_back(keyval_t::value_type(unnamed_argument(), arg));
			}
		else return false;
		}
	return true;
	}

bool property::parse_to_keyvals(const Ex& tr, keyval_t& keyvals) 
	{
	if(tr.begin()==tr.end()) return true;

	auto it=tr.begin();

//	std::cout << "parsing to keyvals" << std::endl;
	if(*(it)->name!="\\comma") { // one argument
		if(parse_one_argument(it, keyvals)==false)
			return false;
		}
	else {
		Ex::sibling_iterator sib=tr.begin(it);
		while(sib!=tr.end(it)) {
			if(parse_one_argument(sib, keyvals)==false)
				return false;
			++sib;
			}
		}

	// for(auto it=keyvals.begin(); it!=keyvals.end(); ++it)
	//   	std::cerr << (*it).first << " = " << *(*it).second->name << std::endl;
	return true;
	}


void property::latex(std::ostream& str) const
	{ 
	str << name();
	}

std::string property::unnamed_argument() const
	{
	return "";
	}

property::match_t property::equals(const property *) const
	{
	return exact_match;
	}

bool labelled_property::parse(const Kernel&, keyval_t& keyvals)
	{
	keyval_t::const_iterator lit=keyvals.find("label");
	if(lit!=keyvals.end()) {
		label=*lit->second->name;
		return true;
		}
	else {
		// FIXME: not all labelled properties have an actual label, e.g.
		// Derivative derives from WeightBase but not all derivatives need
		// a label. If we throw an exception here, those properties fail
		// to run.
//		throw ArgumentException("Need a 'label' parameter for property.");
		return false;
		}
	}
	
//bool operator<(const pattern& one, const pattern& two)
//	{
//	return tree_less(one.obj, two.obj);
////	if(*(one.obj.begin()->name)<*(two.obj.begin()->name)) return true;
////	return false;
//	}

//bool operator==(const pattern& one, const pattern& two)
//	  {
//	  return one.obj==two.obj; // FIXME: handle dummy indices
//	  }

void Properties::insert_prop(const Ex& et, const property *pr)
	{
//	assert(pats.find(pr)==pats.end()); // identical properties have to be assigned through insert_list_prop

	// FIXME: is it really necessary to store this by pointer? We are in any case
	// not cleaning this up correctly yet.
	pattern *pat=new pattern(et);

	std::pair<property_map_t::iterator, property_map_t::iterator> pit=
		props.equal_range(pat->obj.begin()->name_only());

	property_map_t::iterator first_nonpattern=pit.first;

	while(pit.first!=pit.second) {
		// keep track of the first non-pattern element
		if(Ex::number_of_children((*pit.first).second.first->obj.begin())==1) 
			if((*pit.first).second.first->obj.begin().begin()->is_range_wildcard()) 
				++first_nonpattern;
			
		// A given pattern can only have one property of any given type. The following
		// triggers on entries in the props map which match the pattern to be inserted.
		if((*pit.first).second.first->match(*this, et.begin())) {

			// Does this entry in props give a property of the same type as the one we
			// are trying to insert?
			if(typeid(*pr)==typeid(*(*pit.first).second.second)) {

				// If this is a labelled property, is the label different from the one on the
				// property we are trying to insert?
				const labelled_property *lp   =dynamic_cast<const labelled_property *>(pr);
				const labelled_property *lpold=dynamic_cast<const labelled_property *>(pit.first->second.second);

				if(!lp || !lpold || lp->label==lpold->label) {

					// The to-be-inserted property cannot co-exist on this pattern with the
					// one that is currently associated to the pattern. Remove it.

					pattern        *oldpat =pit.first->second.first;
					const property *oldprop=pit.first->second.second;
					
					// If the new property instance is the same as the old one, we can stop
					// (this happens if a pattern is accidentally repeated in a property assignment).
					if(oldprop==pr) {
						delete pat;
						return;
						}

					// Erase the pattern->property entry, and delete the pattern. 
					// FIXME: store pattern by value.
					props.erase(pit.first);
					delete oldpat;

					// Remove the property->pattern entry. Only delete the property 
					// if it is no longer associated to any other pattern.
					// FIXME: 
					//   {A, B}::SelfAntiCommuting.
					//   {A}::SelfAntiCommuting.
					//   {B}::SelfAntiCommuting.
					// leads to two properties SelfAntiCommuting, which are identical.
					// We need a way to compare properties and decide when they are
					// identical, or when they can coexist, or something like that.
					for(auto pi=pats.begin(); pi!=pats.end(); ++pi) {
						if((*pi).first==oldprop && (*pi).second==oldpat) {
//							std::cerr << "found old entry, deleting" << std::endl;
							pats.erase(pi);
							break;
							}
						}
					if(pats.find(oldprop)==pats.end()) {
//						std::cerr << "no other references" << std::endl;
						delete oldprop;
						}

					break;
					}
				}
			}
		++pit.first;
		}

	pats.insert(pattern_map_t::value_type(pr, pat));
	std::cout << "inserting for " << *(pat->obj.begin()->name_only()) << std::endl;
	props.insert(property_map_t::value_type(pat->obj.begin()->name_only(), pat_prop_pair_t(pat,pr)));
	}

void Properties::insert_list_prop(const std::vector<Ex>& its, const list_property *pr)
	{
	assert(pats.find(pr)==pats.end()); // identical properties have to be assigned through insert_list_prop
	assert(its.size()>0);

	// If 'pr' is exactly equal to an existing property, we should use that one instead of 
	// introducing a duplicate.
	pattern_map_t::iterator fit=pats.begin();
	while(fit!=pats.end()) {
		if(typeid(*(*fit).first)==typeid(*pr))
			if(pr->equals((*fit).first)==property::exact_match) {
				pr=static_cast<const list_property *>( (*fit).first );
				break;
				}
		++fit;
		}
	
	// If 'pr' has id_match with an existing property, we need to remove all property assignments
	// for the existing one, except when there is an exact_match.
	const property *to_delete_property=0;
	pattern_map_t::iterator pit=pats.begin();
	while(pit!=pats.end()) {
		if(typeid(*(*pit).first)==typeid(*pr))
			if(pr->equals((*pit).first)==property::id_match) {
				to_delete_property = (*pit).first;
				break;
				}
		++pit;
		}
	if(to_delete_property) {
		pats.erase(to_delete_property);
		property_map_t::iterator it=props.begin();
		while(it!=props.end()) {
			property_map_t::iterator nxt=it;
			++nxt;
			if((*it).second.second==to_delete_property) props.erase(it);
			it=nxt;
			}
		}
	
	
	// Now register the list property.

	for(unsigned int i=0; i<its.size(); ++i) {
		pattern *pat=new pattern(its[i]);

		// Removing properties causes more problems than it solves (the only reason
		// for overwriting a list property is to change the SortOrder, which is 
		// rarely useful). So we just insert the new property regardless.

//		// Pointers to properties are shared, so we need to delete them only once.
//
//		std::pair<property_map_t::iterator, property_map_t::iterator> pit=
//			props.equal_range(its[i]->name);
//
//		while(pit.first!=pit.second) {
//			if((*pit.first).second.first->match(its[i])) { // found the pattern 'its[i]' in the property list
//				if(typeid(*pr)==typeid(*(*pit.first).second.second)) {
////						txtout << "found a property for " << *(its[i]->name) << std::endl;
////						Ex::print_recursive_treeform(txtout, its[i]);
//					
//					pattern  *oldpat=pit.first->second.first;
//					const property *oldprop=pit.first->second.second;
//					
////					props.erase(pit.first); THIS
//					
//					// Delete only those entries in the pattern map which are related to
//					// this particular pattern _and_ this particular property
//					std::pair<pattern_map_t::iterator, pattern_map_t::iterator> patrange=
//						pats.equal_range(oldprop);
//					while(patrange.first!=patrange.second) {
//						if(patrange.first->first==oldprop && patrange.first->second==oldpat) {
////								  txtout << "erasing property for " << *(oldpat->headnode) << std::endl;
////							pats.erase(patrange.first); // THIS  
//							break;
//							}
//						++patrange.first;
//						}
////					delete oldpat; THIS
//					break;
//					}
//				}
//			++pit.first;
//			}
		
		// Now register the property.
//		txtout << "registering " << *(pat->headnode) << std::endl;
		pats.insert(pattern_map_t::value_type(pr, pat));
		props.insert(property_map_t::value_type(pat->obj.begin()->name_only(), pat_prop_pair_t(pat,pr)));
		}
	}


int Properties::serial_number(const property *listprop, const pattern *pat) const
	{
	int serialnum=0;

	std::pair<pattern_map_t::const_iterator, pattern_map_t::const_iterator> 
		pm=pats.equal_range(listprop);
	serialnum=0;
	while(pm.first!=pm.second) { 
		if((*pm.first).second==pat)
			break;
		++serialnum;
		++pm.first;
		}
	return serialnum;
	}


/*

  {a,b,c,d,e}::Indices(vector).
  {a,b,c}::Indices(spinor).

  This should make a,b,c spinor indices, and keep d,e as vector indices.


  {a,b,c}::Indices(vector).
  {d,e}::Indices(vector).

  This should make all of a,b,c,d,e vector indices.


  {a,b,c}::Indices(vector).
  {a,b,c,d,e,f}::Indices(spinor).

  This should make all indices spinor indices.


  {a,b,c,d,e}::Indices(vector, position=free).
  {a,b,c}::Indices(vector, position=fixed).

  You can only have one type of index for each name, so this declaration implies that
  d,e should have their property removed.


 */


// Insert a property for the given pattern Ex. Determines whether the property
// is a list property or a normal one, and dispatches accordingly.

std::string Properties::master_insert(Ex proptree, property *thepropbase)
	{
	std::ostringstream str;

	Ex::sibling_iterator st=proptree.begin();

	list_property *thelistprop=dynamic_cast<list_property *>(thepropbase);
	if(thelistprop) { // a list property
		std::vector<Ex> objs;
		if(*st->name=="\\comma") {
			Ex::sibling_iterator sib=proptree.begin(st);
			while(sib!=proptree.end(st)) {
				if(sib->fl.parent_rel!=str_node::p_property) {
					objs.push_back(Ex(sib));
					}
				++sib;
				}
			}
		if(objs.size()<2) 
			throw ConsistencyException("A list property cannot be assigned to a single object.");
		
		// FIXME: we special-case Indices, as those pass a list of objects with parent_rel==p_none,
		// but we need the patterns to have parent_rel set to p_sub and p_super in order to avoid
		// special cases in the pattern matcher later. 
		// DOCME: the above
		if(dynamic_cast<Indices *>(thelistprop)) {
			std::vector<Ex> objs2;
			for(auto& obj: objs) {
				Ex obj2(obj);
				obj2.begin()->fl.parent_rel=str_node::p_super;
				objs2.push_back(obj2);
				}
			for(auto& obj: objs) {
				Ex obj2(obj);
				obj2.begin()->fl.parent_rel=str_node::p_sub;
				objs2.push_back(obj2);
				}
			insert_list_prop(objs2, thelistprop);
			}
		else {
			insert_list_prop(objs, thelistprop);
			}
		}
	else { // a normal property
		property *theprop=thepropbase;
		assert(theprop);
		if(*st->name=="\\comma") {
			Ex::sibling_iterator sib=proptree.begin(st);
			while(sib!=proptree.end(st)) {
				if(sib->fl.parent_rel!=str_node::p_property) {
               //	std::cerr << "inserting property for " << Ex(sib) << std::endl;
					insert_prop(Ex(sib), theprop);
					}
				++sib;
				}				
			}
		else {
			insert_prop(Ex(st), theprop);
			}
		}
	return str.str();
	}
