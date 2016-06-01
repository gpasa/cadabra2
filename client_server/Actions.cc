

#include "Actions.hh"
#include "DataCell.hh"
#include "DocumentThread.hh"
#include "GUIBase.hh"

#include <iostream>

using namespace cadabra;

ActionAddCell::ActionAddCell(DataCell cell, DTree::iterator ref_, Position pos_) 
	: newcell(cell), ref(ref_), pos(pos_)
	{
	}

void ActionAddCell::pre_execute(DocumentThread& cl)  
	{
	// std::cout << "ActionAddCell::execute" << std::endl;

//	std::lock_guard<std::mutex> guard(cl.dtree_mutex);

	// Insert this DataCell into the DTree document.
	switch(pos) {
		case Position::before:
			newref = cl.doc.insert(ref, newcell);
			break;
		case Position::after:
			newref = cl.doc.insert_after(ref, newcell);
			break;
		case Position::child:
			newref = cl.doc.append_child(ref, newcell);
			break;
		}
	}

void ActionAddCell::post_execute(DocumentThread& cl)  
	{
	}

void ActionAddCell::revert(DocumentThread& cl)
	{
	// FIXME: implement
	}

void ActionAddCell::update_gui(const DTree& tr, GUIBase& gb)
	{
	// std::cout << "updating gui for ActionAddCell" << std::endl;
	gb.add_cell(tr, newref, true);
	}


ActionPositionCursor::ActionPositionCursor(DTree::iterator ref_, Position pos_)
	: needed_new_cell(false), ref(ref_), pos(pos_)
	{
	}

void ActionPositionCursor::pre_execute(DocumentThread& cl)  
	{
	// std::cout << "ActionPositionCursor::execute" << std::endl;

//	std::lock_guard<std::mutex> guard(cl.dtree_mutex);

	switch(pos) {
		case Position::in:
			// std::cerr << "in" << std::endl;
			newref = ref;
			break;
		case Position::next: {
			DTree::sibling_iterator sib=ref;
			while(cl.doc.is_valid(++sib)) {
				if(sib->cell_type==DataCell::CellType::python || sib->cell_type==DataCell::CellType::latex) {
					if(!sib->hidden) {
						// std::cout << "found input cell " << sib->textbuf << std::endl;
						newref=sib;
						return;
						}
					}
				}
			// std::cout << "no input cell available, adding new" << std::endl;
			if(ref->textbuf!="") { // If the last cell is empty, stay where we are.
				DataCell newcell(DataCell::CellType::python, "");
				newref = cl.doc.insert(sib, newcell);
				needed_new_cell=true;
				}
			break;
			}
		case Position::previous: {
			DTree::sibling_iterator sib=ref;
			while(cl.doc.is_valid(--sib)) {
				if(sib->cell_type==DataCell::CellType::python || sib->cell_type==DataCell::CellType::latex) {
					if(!sib->hidden) {
						newref=sib;
						return;
						}
					}
				}
			newref=ref; // No previous sibling cell. FIXME: walk tree structure
			break;
			}
		}
	}

void ActionPositionCursor::post_execute(DocumentThread& cl)  
	{
	}

void ActionPositionCursor::revert(DocumentThread& cl)  
	{
	// FIXME: implement
	}

void ActionPositionCursor::update_gui(const DTree& tr, GUIBase& gb)
	{
	if(needed_new_cell) {
		// std::cerr << "cadabra-client: adding new visual cell before positioning cursor" << std::endl;
		gb.add_cell(tr, newref, true);
		}
	// std::cerr << "cadabra-client: positioning cursor" << std::endl;
	gb.position_cursor(tr, newref);
	}


ActionRemoveCell::ActionRemoveCell(DTree::iterator ref_) 
	: this_cell(ref_)
	{
	}

ActionRemoveCell::~ActionRemoveCell()
	{
	}

void ActionRemoveCell::pre_execute(DocumentThread& cl)  
	{
	}

void ActionRemoveCell::post_execute(DocumentThread& cl)  
	{
//	std::lock_guard<std::mutex> guard(cl.dtree_mutex);

	reference_parent_cell = cl.doc.parent(this_cell);
	reference_child_index = cl.doc.index(this_cell);
	cl.doc.erase(this_cell);
	}

void ActionRemoveCell::revert(DocumentThread& cl)
	{
	// FIXME: implement
	}

void ActionRemoveCell::update_gui(const DTree& tr, GUIBase& gb)
	{
	// std::cout << "updating gui for ActionRemoveCell" << std::endl;
	gb.remove_cell(tr, this_cell);
	}


ActionSplitCell::ActionSplitCell(DTree::iterator ref_) 
	: this_cell(ref_)
	{
	}

ActionSplitCell::~ActionSplitCell()
	{
	}

void ActionSplitCell::pre_execute(DocumentThread& cl)  
	{
//	std::lock_guard<std::mutex> guard(cl.dtree_mutex);

//	size_t pos = gb.get_cursor_position(tr, this_cell);
//	std::cerr << "cursor position = " << pos << std::endl;

//	std::string segment1=
//		this_cell->textbuf->get_slice(dc->textbuf->begin(), 
//									  dc->textbuf->get_iter_at_mark(dc->textbuf->get_insert())));


	DataCell newcell(this_cell->cell_type, "hi!");

	newref = cl.doc.insert(this_cell, newcell);
	}

void ActionSplitCell::post_execute(DocumentThread& cl)  
	{
	}

void ActionSplitCell::revert(DocumentThread& cl)
	{
	// FIXME: implement
	}

void ActionSplitCell::update_gui(const DTree& tr, GUIBase& gb)
	{
	// std::cout << "updating gui for ActionSplitCell" << std::endl;
	gb.add_cell(tr, newref, true);
	}



ActionSetRunStatus::ActionSetRunStatus(DTree::iterator ref_, bool running) 
	: this_cell(ref_), new_running_(running)
	{
	}

void ActionSetRunStatus::pre_execute(DocumentThread& cl)  
	{
	}

void ActionSetRunStatus::post_execute(DocumentThread& cl)  
	{
	was_running_=this_cell->running;
	this_cell->running=new_running_;
	}

void ActionSetRunStatus::revert(DocumentThread& cl)
	{
	}

void ActionSetRunStatus::update_gui(const DTree& tr, GUIBase& gb)
	{
	gb.update_cell(tr, this_cell);
	}

