
#pragma once

// All modifications to the document are done by calling 'perform' with an 
// action object. This enables us to implement an undo stack. This method
// will take care of making the actual change to the DTree document, and
// call back on the 'change' methods above to inform the derived class
// that a change has been made. 

#include "DataCell.hh"
#include "DocumentThread.hh"

#include <memory>

namespace cadabra {

	class DocumentThread;
	class GUIBase;

	/// \ingroup clientserver
	///
	/// All actions derive from the ActionBase object, which defines
	/// the interface they need to implement. These objects are used to
	/// pass (user) action instructions around.  They can be stored in
	/// undo/redo stacks. All actions run on the GUI thread. The
	/// update_gui members typically call members of the GUIBase class.

	class ActionBase {
		public:
			/// Execute changes to the DTree document which need to be made
			/// before the GUI is updated. As an example, the 'ActionAddCell'
			/// adds a new cell to the document tree here, after which the
			/// update_gui method makes it visible on screen.

			virtual void pre_execute(DocumentThread&)=0;

			/// Make sure the GUI reflects the change.
			/// FIXME: do we need an exec/revert combo here too?

			virtual void update_gui(const DTree&, GUIBase&)=0;

			/// Execute changes to the DTree document which need to be made
			/// after the GUI has been updated. As an example, the 'ActionRemoveCell'
			/// first removes the cell from the display in 'update_gui', and 
			/// then removes it from the underlying document tree here.

			virtual void post_execute(DocumentThread&)=0;

			/// Revert the change to the DTree document.

			virtual void revert(DocumentThread&)=0;
	};
	
	/// \ingroup clientserver
	///
	/// Add a cell to the notebook.
	
	class ActionAddCell : public ActionBase {
		public:
			enum class Position { before, after, child };
			
			ActionAddCell(DataCell, DTree::iterator ref_, Position pos_);
			
			virtual void pre_execute(DocumentThread&) override;
			virtual void update_gui(const DTree&, GUIBase&) override;
			virtual void post_execute(DocumentThread&) override;

			virtual void revert(DocumentThread&) override;
			
		private:
			// Keep track of the location where this cell is inserted into
			// the notebook. 
			
			DataCell          newcell;
			DTree::iterator   ref, newref;
			Position          pos;
	};


	/// \ingroup clientserver
	///
	/// Position the cursor relative to the indicated cell. If position is 'next' and
   /// there is no input cell following the indicated one, create a new one.

	class ActionPositionCursor : public ActionBase {
		public:
			enum class Position { in, next, previous };

			ActionPositionCursor(DTree::iterator ref_, Position pos_);

			virtual void pre_execute(DocumentThread&) override;
			virtual void update_gui(const DTree&, GUIBase&) override;
			virtual void post_execute(DocumentThread&) override;

			virtual void revert(DocumentThread&) override;

		private:
			bool              needed_new_cell;
			DTree::iterator   ref, newref;
			Position          pos;
	};

	/// \ingroup clientserver
	///
	/// Update the running status of the indicated cell.

	class ActionSetRunStatus : public ActionBase {
		public:
			ActionSetRunStatus(DTree::iterator ref_, bool running);

			virtual void pre_execute(DocumentThread&) override;
			virtual void update_gui(const DTree&, GUIBase&) override;
			virtual void post_execute(DocumentThread&) override;

			virtual void revert(DocumentThread&) override;

		private:
			DTree::iterator this_cell;
			bool            was_running_, new_running_;
	};
	

	/// \ingroup clientserver
	///
	/// Remove a cell and all its child cells from the document.

	class ActionRemoveCell : public ActionBase {
		public:
			ActionRemoveCell(DTree::iterator ref_);
			~ActionRemoveCell();
			
			virtual void pre_execute(DocumentThread&) override;
			virtual void update_gui(const DTree&, GUIBase&) override;
			virtual void post_execute(DocumentThread&) override;

			virtual void revert(DocumentThread&) override;

		private:
			// Keep track of the location where this cell (and its child
			// cells) was in the notebook.  We keep a reference to the
			// parent cell and the index of the current cell as child of
			// that parent.

			DTree             removed_tree;
			DTree::iterator   reference_parent_cell, this_cell;
			size_t            reference_child_index;
	};

	/// \ingroup clientserver
	///
	/// Split a cell into two separate cells, at the point of the cursor.

	class ActionSplitCell : public ActionBase {
		public:
			ActionSplitCell(DTree::iterator ref_);
			~ActionSplitCell();

			virtual void pre_execute(DocumentThread&) override;
			virtual void update_gui(const DTree&, GUIBase&) override;
			virtual void post_execute(DocumentThread&) override;

			virtual void revert(DocumentThread&) override;
			
		private:
			DTree::iterator this_cell, newref; // the newly created cell
	};

}
			
//			class ActionAddText : public ActionBase {
//				public:
//					ActionAddText(tree<DataCell>::iterator, int, const std::string&);
//					
//					virtual void execute(XCadabra&);
//					virtual void revert(XCadabra&);
//					
//					int         insert_pos;
//					std::string text;
//			};
//			
//			class ActionRemoveText : public ActionBase {
//				public:
//					ActionRemoveText(tree<DataCell>::iterator, int, int, const std::string&);
//					
//					virtual void execute(XCadabra&);
//					virtual void revert(XCadabra&);
//					
//					int from_pos, to_pos;
//					std::string removed_text;
//			};
//
//       class ActionMergeCells
			


